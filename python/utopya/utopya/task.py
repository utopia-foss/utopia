"""The Task class supplies a container for all information needed for a task.

The WorkerTask and ProcessTask classes specialize on tasks for the
WorkerManager that work on subprocesses or multiprocessing processes.
"""

import os
import copy
import uuid
import time
import queue
import io
import subprocess
import threading
import multiprocessing
import signal
import re
import warnings
import logging
from functools import partial
from typing import Callable, Union, Dict, List, Sequence, Set, Tuple, Generator
from typing.io import TextIO

import numpy as np

from .tools import yaml

# Local variables
log = logging.getLogger(__name__)

# A map from signal names to corresponding integer exit codes
SIGMAP = {a: int(getattr(signal, a)) for a in dir(signal) if a[:3] == "SIG"}

# A regex pattern to remove ANSI escape characters, needed for stream saving
# From: https://stackoverflow.com/a/14693789/1827608
_ANSI_ESCAPE = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')


# -----------------------------------------------------------------------------
# Helper methods
# These solely relate to the WorkerTask and similar classes, and thus are not
# implemented in the tools module.

def _follow(f: io.TextIOWrapper,
            delay: float=0.05,
            should_stop: Callable=lambda: False,
            ) -> Generator[str, None, None]:
    """Generator that follows the output written to the given stream object
    and yields each new line written to it. If no output is retrieved, there
    will be a delay to reduce processor load.

    The ``should_stop`` argument may be a callable that will lead to breaking
    out of the waiting loop. If it is not given, the loop will only break if
    reading from the stream ``f`` is no longer possible, e.g. because it was
    closed.
    """
    while not should_stop():
        try:
            line = f.readline()
        except:
            # Stream was closed or is otherwise not readable, end generator
            return

        if not line:
            time.sleep(delay)
            continue
        yield line

def enqueue_lines(*, queue: queue.Queue, stream: TextIO, follow: bool=False,
                  parse_func: Callable=None) -> None:
    """From the given text stream, read line-buffered lines and add them to the
    provided queue as 2-tuples, (line, parsed object).

    This function is meant to be passed to an individual thread in which it can
    read individual lines separately from the main thread. Before exiting this
    function, the stream is closed.

    Args:
        queue (queue.Queue): The queue object to put the read line and parsed
            objects into.
        stream (TextIO): The stream identifier. If this is not a text stream,
            be aware that the elements added to the queue might need decoding.
        follow (bool, optional): If instead of ``iter(stream.readline)``, the
            :py:func:`~utopya.task._follow` function should be used instead.
            This should be selected if the stream is file-like instead of
            ``sys.stdout``-like.
        parse_func (Callable, optional): A parse function that the read line
            is passed through. This should be a unary function that either
            returns a successfully parsed line or None.
    """
    # Define a pass-through parse function, if none was given
    parse_func = parse_func if parse_func else lambda _: None

    # If this is a buffered stream (like subprocess.Popen.stdout), we can use a
    # simple iterator that will not hang up. If it is a file-based stream (e.g.
    # when reading from a file), we need to follow the file similar to how
    # `tail -f` does it ...
    if follow:
        # Get the current thread to allow stopping to follow
        ct = threading.currentThread()
        should_stop = lambda: getattr(ct, "stop_follow", False)

        it = _follow(stream, should_stop=should_stop)
    else:
        it = iter(stream.readline, '')

    # Read the lines and put them into the queue
    for line in it: # <-- thread waits here for a new line, w/o idle looping
        # Got a new line
        # Strip the whitespace on the right (e.g. the new-line character)
        line = line.rstrip()

        # Add it to the queue as tuples: (string, parsed object), where the
        # parsed object can also be None
        queue.put_nowait((line, parse_func(line)))

    # Thread dies here.

# Custom parse methods ........................................................


def parse_yaml_dict(line: str, *, start_str: str="!!map") -> Union[None, dict]:
    """A yaml parse function that can be passed to enqueue_lines. It only tries
    parsing the line if it starts with the provided start string.

    It tries to decode the line, and parse it as a yaml. If that fails, it
    will still try to decode the string. If that fails yet again, the
    unchanged line will be returned.

    Args:
        line (str): The line to decode, assumed byte-string, utf8-encoded
        start_str (str, optional): Description

    Returns:
        Union[None, dict]: either the decoded dict, or, if that failed:
    """
    # Check if it should be attempted to parse this line
    if not line.startswith(start_str):
        # Nope, return None
        return None

    # Try to load the object, ensuring it is a dict
    try:
        obj = dict(yaml.load(line))

    except Exception as err:
        # Failed to do that, regardless why; be verbose about it
        log.warning("Got %s while trying to parse line '%s': %s",
                    err.__class__.__name__, line, err)

        return None

    # Was able to parse it. Return the parsed object.
    return obj


# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------

class Task:
    """The Task is a container for a task handled by the WorkerManager.

    It aims to provide the necessary interfaces for the WorkerManager to easily
    associate tasks with the corresponding workers and vice versa.
    """

    __slots__ = ('_name', '_priority', '_uid', '_progress_func',
                 '_stop_conditions', 'callbacks')

    def __init__(self, *,
                 name: str=None,
                 priority: float=None,
                 callbacks: Dict[str, Callable]=None,
                 progress_func: Callable=None):
        """Initialize a Task object.

        Args:
            name (str, optional): The task's name. If none is given, the
                generated uuid will be used.
            priority (float, optional): The priority of this task; if None,
                default is +np.inf, i.e. the lowest priority. If two priority
                values are the same, the task created earlier has a higher
                priority.
            callbacks (Dict[str, Callable], optional): A dict of callback funcs
                that are called at different points of the life of this task.
                The function gets passed as only argument this task object.
            progress_func (Callable, optional): Invoked by the ``progress``
                property and used to calculate the progress given the current
                task object as argument
        """
        # Carry over arguments attributes
        self._name = str(name) if name is not None else None
        self._priority = priority if priority is not None else np.inf

        # Create a unique ID
        self._uid = uuid.uuid1()

        # Save the callbacks and the progress function
        self.callbacks = callbacks
        self._progress_func = progress_func

        # The to-be-populated set of _fulfilled_ stop conditions
        self._stop_conditions = set()

        log.debug("Initialized Task '%s'.\n  Priority: %s,  UID: %s.",
                  self.name, self.priority, self.uid)


    # Properties ..............................................................

    @property
    def name(self) -> str:
        """The task's name, if given; else the uid."""
        if self._name is not None:
            return self._name
        return str(self.uid)

    @property
    def uid(self) -> int:
        """The task's unique ID"""
        return self._uid

    @property
    def priority(self) -> float:
        """The task's priority. Default is +inf, which is the lowest priority
        """
        return self._priority

    @property
    def order_tuple(self) -> tuple:
        """Returns the ordering tuple (priority, uid.time)"""
        return (self.priority, self.uid.time)

    @property
    def progress(self) -> float:
        """If a progress function is given, invokes it; otherwise returns 0

        This also performs checks that the progress is in [0, 1]
        """
        if self._progress_func is None:
            return 0.

        # Invoke it to get the progress and check interval
        progress = self._progress_func(self)

        if progress >= 0 and progress <= 1:
            return progress

        raise ValueError("The progres function {} of task '{}' returned a "
                         "value outside of the allowed range [0, 1]!"
                         "".format(self._progress_func.__name__, self.name))

    @property
    def fulfilled_stop_conditions(self) -> Set['StopCondition']:
        """The set of *fulfilled* stop conditions for this task. Typically,
        this is set by the StopCondition itself as part of its evaluation in
        :py:meth:`utopya.stopcond.StopCondition.fulfilled`.
        """
        return self._stop_conditions


    # Magic methods ...........................................................

    def __hash__(self) -> int:
        return hash(self.uid)

    def __str__(self) -> str:
        return "Task<uid: {}, priority: {}>".format(self.uid, self.priority)

    # Rich comparisons, needed in PriorityQueue
    # NOTE only need to implement __lt__, __le__, and __eq__, the others are
    # created by calling the methods with swapped arguments.

    def __lt__(self, other):
        return bool(self.order_tuple < other.order_tuple)

    def __le__(self, other):
        return bool(self.order_tuple <= other.order_tuple)

    def __eq__(self, other):
        return bool(self is other)
        # NOTE we trust 'uuid' that the IDs are unique therefore different
        #      tasks can not get the same ID --> are different in ordering


    # Private methods .........................................................

    def _invoke_callback(self, name: str):
        """If given, invokes the callback function with the name `name`.

        NOTE In order to have higher flexibility, this will _not_ raise errors
             or warnings if there was no callback function specified with the
             give name.
        """
        if self.callbacks and name in self.callbacks:
            self.callbacks[name](self)


# -----------------------------------------------------------------------------
# ... working with subprocess

class WorkerTask(Task):
    """A specialisation of the Task class that is aimed at use in the
    WorkerManager.

    It is able to spawn a worker process, executing the task. Task execution
    is non-blocking. At the same time, the worker's stream can be read in via
    another non-blocking thread.

    Attributes:
        profiling (dict): Profiling information of this WorkerTask
        setup_func (Callable): The setup function to use before this task is
            being worked on
        setup_kwargs (dict): The kwargs to use to call setup_func
        streams (dict): the associated streams of this WorkerTask
        worker (subprocess.Popen): The worker process, if spawned
        worker_kwargs (dict): The kwargs to use to spawn a worker process
    """

    # Extend the slots of the Task class with some WorkerTask-specific slots
    __slots__ = ('setup_func', 'setup_kwargs', 'worker_kwargs',
                 '_worker', '_worker_pid', '_worker_status',
                 'streams', 'profiling')

    # A mapping of functions that are used in parsing the streams
    STREAM_PARSE_FUNCS = dict(default=None,
                              yaml_dict=parse_yaml_dict)

    def __init__(self, *,
                 setup_func: Callable=None,
                 setup_kwargs: dict=None,
                 worker_kwargs: dict=None,
                 **task_kwargs):
        """Initialize a WorkerTask object, a specialization of a task for use
        in the WorkerManager.

        Args:
            setup_func (Callable, optional): The function to call before the
                worker process is spawned
            setup_kwargs (dict, optional): The kwargs to call setup_func with
            worker_kwargs (dict, optional): The kwargs needed to spawn the
                worker. Note that these are also passed to setup_func and, if a
                setup_func is given, the return value of that function will be
                used for the worker_kwargs.
            **task_kwargs: Arguments to be passed to Task.__init__, including
                the callbacks dictionary.

        Raises:
            ValueError: If neither `setup_func` nor `worker_kwargs` were given
        """

        super().__init__(**task_kwargs)

        # Check the argument values
        if setup_func:
            setup_kwargs = setup_kwargs if setup_kwargs else dict()

        elif worker_kwargs:
            if setup_kwargs:
                warnings.warn("`worker_kwargs` given but also `setup_kwargs` "
                              "specified; the latter will be ignored. Did "
                              "you mean to call a setup function? If yes, "
                              "pass it via the `setup_func` argument.",
                              UserWarning)
        else:
            raise ValueError("Need either argument `setup_func` or "
                             "`worker_kwargs`, got none of those.")

        # Save the arguments
        self.setup_func = setup_func
        self.setup_kwargs = copy.deepcopy(setup_kwargs)
        self.worker_kwargs = copy.deepcopy(worker_kwargs)

        # Create empty attributes to be filled with worker information
        self._worker = None
        self._worker_pid = None
        self._worker_status = None
        self.streams = dict()
        self.profiling = dict()

        log.debug("Finished setting up task '%s' as a %s.",
                  self.name, self.__class__.__name__)
        log.debug("  With setup function?  %s", bool(setup_func))

    # Properties ..............................................................

    @property
    def worker(self) -> subprocess.Popen:
        """The associated worker process object or None, if not yet created."""
        return self._worker

    @worker.setter
    def worker(self, proc: subprocess.Popen):
        """Set the associated worker process of this task.

        This can only be done once.

        Args:
            proc (subprocess.Popen): The process to associate with this task.

        Raises:
            RuntimeError: If a process was already associated.
        """
        if self.worker is not None:
            raise RuntimeError("A worker process was already associated with "
                               "this task; cannot change it!")

        # Save the object and its process id
        self._worker = proc
        self._worker_pid = proc.pid

        log.debug("Task %s: associated with worker process %d.",
                  self.name, self.worker_pid)

    @property
    def worker_pid(self) -> int:
        """The process ID of the associated worker process"""
        return self._worker_pid

    @property
    def worker_status(self) -> Union[int, None]:
        """The worker processe's current status or False, if there is no
        worker spawned yet.

        Note that this invokes a poll to the worker process if one was spawned.

        Returns:
            Union[int, None]: Current worker status. False, if there was no
                worker associated yet.
        """
        if not self.worker:
            return False

        if self._worker_status is None:
            # No cached value yet; poll the worker
            poll_res = self.worker.poll()

            if poll_res is not None:
                # The worker finished. Save the exit status and finish up ...
                self._worker_status = poll_res
                self._finished()

            return poll_res

        # Return the cached exit status
        return self._worker_status

    @property
    def outstream_objs(self) -> list:
        """Returns the list of objects parsed from the 'out' stream"""
        return self.streams['out']['log_parsed']


    # Magic methods ...........................................................

    def __str__(self) -> str:
        """Return basic WorkerTask information."""
        return ("{}<uid: {}, priority: {}, worker_status: {}>"
                "".format(self.__class__.__name__, self.uid,
                          self.priority, self.worker_status))


    # Public API ..............................................................

    def spawn_worker(self) -> subprocess.Popen:
        """Spawn a worker process using subprocess.Popen and manage the
        corresponding queue and thread for reading the stdout stream.

        If there is a setup_func, this function will be called first.

        Afterwards, from the worker_kwargs returned by that function or from
        the ones given during initialisation (if not setup_func was given),
        the worker process is spawned and associated with this task.

        Returns:
            subprocess.Popen: The created process object

        Raises:
            RuntimeError: If a worker was already spawned for this task.
            TypeError: For invalid `args` argument
        """
        if self.worker:
            raise RuntimeError("Can only spawn one worker per task!")

        # If a setup function is available, call it with the given kwargs
        if self.setup_func:
            log.debug("Calling a setup function ...")
            worker_kwargs = self.setup_func(
                worker_kwargs=self.worker_kwargs, **self.setup_kwargs
            )
        else:
            log.debug("No setup function given; using given `worker_kwargs`")
            worker_kwargs = self.worker_kwargs

        # Start the subprocess and associate it with this WorkerTask
        self.worker = self._spawn_worker(**worker_kwargs)

        # ... and take care of stdout stream reading.
        if worker_kwargs.get('read_stdout', True):
            self._setup_stream_reader(
                'out',
                stream=self.worker.stdout,
                parser=worker_kwargs.pop('stdout_parser', 'default'),
                **worker_kwargs,
            )

        # Done with spawning.
        self._invoke_callback('spawn')
        return self.worker

    def read_streams(self, stream_names: list='all', *,
                     max_num_reads: int=10,
                     forward_directly: bool=False) -> None:
        """Read the streams associated with this task's worker.

        Args:
            stream_names (list, optional): The list of stream names to read.
                If 'all' (default), will read all streams.
            max_num_reads (int, optional): How many lines should be read from
                the buffer. For -1, reads the whole buffer.
                WARNING: Do not make this value too large as it could block the
                whole reader thread of this worker.
            forward_directly (bool, optional): Whether to call the
                `forward_streams` method; this is done before the callback and
                can be useful if the callback should not happen before the
                streams are forwarded.

        Returns:
            None
        """
        def read_single_stream(stream: dict, stream_name: str,
                               max_num_reads=max_num_reads) -> bool:
            """A function to read a single stream

            Returns true, if a parsed object was among the read stream entries
            """
            log.debug("Reading stream '%s' ...", stream_name)

            q = stream['queue']

            # The flag that is set if there was a parsed object in the queue
            contained_parsed_obj = False

            # In certain cases, read as many as queue reports to have
            if max_num_reads == -1:
                max_num_reads = q.qsize()
                # NOTE this value is approximate; thus, this should only be
                #      called if it is reasonably certain that the queue size
                #      will not change

            # Perform the read operations
            for _ in range(max_num_reads):
                # Try to read a single entry, i.e.: the tuple enqueued by
                # enqueue_lines, being: (decoded string, parsed object)
                try:
                    line, obj = q.get_nowait()

                except queue.Empty:
                    break

                else:
                    # Got entries, write it to the stream's raw log
                    stream['log_raw'].append(line)

                    # Check if there was a parsed object
                    if obj is not None:
                        # Add object to the parsed log and set the flag
                        stream['log_parsed'].append(obj)
                        contained_parsed_obj = True

                    else:
                        # Write line to the regular log. This way, the regular
                        # log only contains this entry if no object could be
                        # parsed.
                        stream['log'].append(line)

            return contained_parsed_obj

        if not self.streams:
            # There are no streams to read
            log.debug("No streams to read for WorkerTask '%s' (uid: %s).",
                      self.name, self.uid)
            return

        elif stream_names == 'all':
            # Gather list of stream names
            stream_names = list(self.streams.keys())

        # Now have the stream names set properly
        # Set the flag that determines whether there will be a callback
        got_parsed_obj = False

        # Loop over stream names and call the function to read a single stream
        for stream_name in stream_names:
            # Get the corresponding stream dict
            stream = self.streams[stream_name]
            # NOTE This way, a non-existent stream_name will not pass silently
            #      put raise a KeyError

            # Read the stream, saving its return value (needed for flag)
            rv = read_single_stream(stream, stream_name)

            if rv:
                got_parsed_obj = True

        # May want to forward
        if forward_directly:
            self.forward_streams()

        # Invoke a callback, if there was a parsed object
        if got_parsed_obj:
            self._invoke_callback('parsed_object_in_stream')

        return

    def save_streams(self, stream_names: list='all', *, final: bool=False):
        """For each stream, checks if it is to be saved, and if yes: saves it.

        The saving location is stored in the streams dict. The relevant keys
        are the `save` flag and the `save_path` string.

        Note that this function does not save the whole stream log, but only
        those part of the stream log that have not already been saved. The
        position up to which the stream was saved is stored under the
        `lines_saved` key in the stream dict.

        Args:
            stream_names (list, optional): The list of stream names to _check_.
                If 'all' (default), will check all streams whether the `save`
                flag is set.
            save_raw (bool, optional): If True, stores the raw log; otherwise
                stores the regular log, i.e. the lines that were parseable not
                included.
            final (bool, optional): If True, this is regarded as the final
                save operation for the stream, which will lead to additional
                information being saved to the end of the log.
            remove_ansi (bool, optional): If True, will remove ANSI escape
                characters (e.g. from colored logging) from the log before
                saving to file.

        Returns:
            None
        """
        if not self.streams:
            # There are no streams to save
            log.debug("No streams to save for WorkerTask '%s' (uid: %s).",
                      self.name, self.uid)
            return

        elif stream_names == 'all':
            # Gather list of stream names
            stream_names = list(self.streams.keys())

        # Go over all streams and check if they were configured to be saved
        for stream_name in stream_names:
            # Get the corresponding stream dict
            stream = self.streams[stream_name]

            # Determine if to save this one
            if not stream.get('save'):
                log.debug("Not saving stream '%s' ...", stream_name)
                continue
            # else: this stream is to be saved

            # Determine the lines to save
            save_raw = stream['save_raw']
            stream_log = stream['log_raw'] if save_raw else stream['log']
            lines_to_save = stream_log[slice(stream['lines_saved'], None)]

            if not lines_to_save:
                log.debug("No lines to save for stream '%s'. Lines already "
                          "saved: %d / %d.", stream_name,
                          stream['lines_saved'], len(stream['log']))
                continue

            log.debug("Saving the log of stream '%s' to %s, starting from "
                      "line %d ...",
                      stream_name, stream['save_path'], stream['lines_saved'])

            # Open the file and append the not yet saved lines
            with open(stream['save_path'], 'a') as f:
                # Write header, if not already done
                if stream['lines_saved'] == 0:
                    f.write(
                        "Log of '{}' stream of {} '{}'\n---\n\n"
                        "".format(stream_name, type(self).__name__, self.name)
                    )

                # Prepare the string that is to be saved, potentially removing
                # ANSI escape characters (e.g. from regex logging) ...
                s = "\n".join(lines_to_save)
                if stream['remove_ansi']:
                    s = _ANSI_ESCAPE.sub('', s)

                # ... and write it.
                f.write(s)

                # If this is the final save call, add information on the exit
                # status to the end
                if final:
                    f.write(
                        "\n"
                        "\n---"
                        f"\nend of log. exit code: {self.worker_status}\n"
                    )

                    if self.fulfilled_stop_conditions:
                        _fsc = "\n  - ".join([str(sc) for sc in
                                              self.fulfilled_stop_conditions])
                        f.write("\nFulfilled stop condition(s):\n"
                                f"  - {_fsc}\n")

                # Ensure new line at the end
                f.write("\n")

            # Update counter
            stream['lines_saved'] += len(lines_to_save)

            log.debug("Saved %d lines of stream '%s'.",
                      len(lines_to_save), stream_name)

        # All done.

    def forward_streams(self, stream_names: list='all',
                        forward_raw: bool=False) -> bool:
        """Forwards the streams to stdout, either via logging module or print

        This function can be periodically called to forward the part of the
        stream logs that was not already forwarded to stdout.

        The information for that is stored in the stream dict. The log_level
        entry is used to determine whether the logging module should be used
        or (in case of None) the print method.

        Args:
            stream_names (list, optional): The list of streams to print

        Returns:
            bool: whether there was any output
        """

        def print_lines(lines: List[str], *, log_level: int):
            """Prints the lines to stdout via print or log.log"""
            prefix = "  {} {}: ".format(self.name, stream_name)

            if log_level is None:
                # print it to the parent process's stdout
                for line in lines:
                    print(prefix, line)

            else:
                # use the logging module
                for line in lines:
                    log.log(log_level, "%s %s", prefix, line)

        # Check whether there are streams that could be printed
        if not self.streams:
            # There are no streams to print
            log.debug("No streams to print for WorkerTask '%s' (uid: %s).",
                      self.name, self.uid)
            return

        elif stream_names == 'all':
            # Gather list of stream names
            stream_names = list(self.streams.keys())

        # Keep track whether there was output, which will be the return value
        rv = False

        # Go over all streams in the list
        for stream_name in stream_names:
            # Get stream; will raise KeyError if invalid
            stream = self.streams[stream_name]

            # Determine if to forward this one
            if not stream.get('forward'):
                log.debug("Not forwarding stream '%s' ...", stream_name)
                continue
            # else: this stream is to be forwarded

            # Determine lines to write
            forward_raw = stream.get('forward_raw', True)
            stream_log = stream['log_raw'] if forward_raw else stream['log']
            lines = stream_log[stream['lines_forwarded']:]
            if not lines:
                # Nothing to print
                continue

            # Write and increment counter
            print_lines(lines, log_level=stream.get('log_level'))
            stream['lines_forwarded'] += len(lines)

            # There was output -> set flag
            rv = True
            log.debug("Forwarded %d lines for stream '%s' of WorkerTask '%s'.",
                      len(lines), stream_name, self.name)

        # Done.
        return rv

    def signal_worker(self, signal: str) -> tuple:
        """Sends a signal to this WorkerTask's worker.

        Args:
            signal (str): The signal to send. Needs to be a valid signal name,
                i.e.: available in python signal module.

        Raises:
            ValueError: When an invalid `signal` argument was given

        Returns:
            tuple: (signal: str, signum: int) sent to the worker
        """
        # Determine the signal number
        try:
            signum = SIGMAP[signal]

        except KeyError as err:
            raise ValueError("No signal named '{}' available! Valid signals "
                             "are: {}".format(signal, ", ".join(SIGMAP.keys()))
                             ) from err

        # Handle some specific cases, then all the other signals ...
        if signal == 'SIGTERM':
            log.debug("Terminating worker of task %s ...", self.name)
            self.worker.terminate()

        elif signal == 'SIGKILL':
            log.debug("Killing worker of task %s ...", self.name)
            self.worker.kill()

        elif signal == 'SIGINT':
            log.debug("Interrupting worker of task %s ...", self.name)
            self.worker.send_signal(SIGMAP['SIGINT'])

        else:
            log.debug("Sending %s (%d) to worker of task %s ...",
                      signal, signum, self.name)
            self.worker.send_signal(SIGMAP[signal] if isinstance(signal, str)
                                    else signal)

        self._invoke_callback('after_signal')
        return signal, signum


    # Private API .............................................................

    def _prepare_process_args(self, *, args: tuple, read_stdout: bool,
                              **kwargs) -> Tuple[tuple, dict]:
        """Prepares the arguments that will be passed to subprocess.Popen"""
        # Set encoding such that stream reading is in text mode; provides
        # backwards-compatibilibty to cases where popen_kwargs is empty.
        kwargs['encoding'] = kwargs.get('encoding', 'utf8')

        # Set the buffer size
        kwargs['bufsize'] = kwargs.get('bufsize', 1)
        # NOTE bufsize = 1 is important here as default, as we usually want
        #      lines to not be interrupted. As this only works in text mode,
        #      the encoding specified via popen_kwargs is crucial here.

        # Depending on whether stdout should be read, set up the pipe objects
        if read_stdout:
            # Create new pipes for STDOUT and _forwarding_ STDERR into that
            # same pipe. For the specification of that syntax, see the
            # subprocess.Popen docs:
            #   docs.python.org/3/library/subprocess.html#subprocess.Popen
            kwargs['stdout'] = subprocess.PIPE
            kwargs['stderr'] = subprocess.STDOUT

        else:
            # No stream-reading is taking place; forward all streams to devnull
            kwargs['stdout'] = kwargs['stderr'] = subprocess.DEVNULL

        return args, kwargs

    def _spawn_process(self, args, **popen_kwargs):
        """This helper takes care *only* of spawning the actual process and
        potential error handling.

        It can be subclassed to spawn a different kind of process
        """
        try:
            return subprocess.Popen(args, **popen_kwargs)

        except FileNotFoundError as err:
            raise FileNotFoundError(
                f"No executable found for task '{self.name}'! "
                f"Process arguments:  {repr(args)}"
            ) from err

    def _spawn_worker(self, *, args: tuple, popen_kwargs: dict = None,
                      read_stdout: bool = True, **_) -> subprocess.Popen:
        """Helper function to spawn the worker subprocess"""
        # Prepare arguments
        args, popen_kwargs = self._prepare_process_args(
            args=args,
            read_stdout=read_stdout,
            **(popen_kwargs if popen_kwargs else {})
        )

        if not isinstance(args, tuple):
            raise TypeError(
                f"Need argument `args` to be of type tuple, got {type(args)} "
                f"with value {args}. Refusing to even try to spawn a worker "
                "process."
            )

        # Spawn the child process with the given arguments
        log.debug("Spawning worker process with args:\n  %s", args)
        proc = self._spawn_process(args, **popen_kwargs)

        # ... it is running now.
        # Save the approximate creation time (as soon as possible)
        self.profiling['create_time'] = time.time()
        log.debug("Spawned worker process with PID %s.", proc.pid)

        return proc

    def _setup_stream_reader(self, stream_name: str, *,
                             stream,
                             parser: str = 'default',
                             follow: bool = False,
                             save_streams: bool = False,
                             save_streams_to: str = None,
                             save_raw: bool = True,
                             remove_ansi: bool = False,
                             forward_streams: bool = False,
                             forward_raw: bool = True,
                             streams_log_lvl: int = None,
                             **_):
        """Sets up the stream reader thread"""
        # Create the queue that will contain the stream
        q = queue.Queue()

        # Resolve and assemble the enqueue function, passing the parser
        # function to it ... (parse_func=None is a valid argument)
        log.debug("Using stream parse function: %s", parser)
        parse_func = self.STREAM_PARSE_FUNCS[parser]
        enqueue_func = partial(enqueue_lines, parse_func=parse_func,
                               follow=follow)

        # Generate the thread that reads the stream and populates the queue
        t = threading.Thread(target=enqueue_func,
                             kwargs=dict(queue=q, stream=stream))
        # Set to be a daemon thread => will die with the parent thread
        t.daemon = True

        # Start the thread; this will lead to enqueue_func being called
        t.start()

        # Save the stream information in the WorkerTask object
        # This includes two counters for the number of lines saved and
        # forwarded, which are used by the save_/forward_streams methods
        self.streams[stream_name] = dict(
            queue=q, thread=t,
            log=[], log_raw=[], log_parsed=[],
            save=save_streams, save_path=None,
            save_raw=save_raw, remove_ansi=remove_ansi,
            forward=forward_streams, forward_raw=forward_raw,
            log_level=streams_log_lvl,
            lines_saved=0, lines_forwarded=0
        )

        log.debug("Added thread to read worker %s's %s stream",
                  self.name, stream_name)

        # If configured to save, save the
        if save_streams:
            if not save_streams_to:
                raise ValueError(
                    "Was told to `save_streams` but did not find a "
                    "`save_streams_to` argument in `worker_kwargs`!"
                )

            # Perform a format operation to generate the path
            save_path = save_streams_to.format(name=stream_name)
            self.streams[stream_name]['save_path'] = save_path

    def _stop_stream_reader(self, name: str):
        """Stops the stream reader with the given name."""
        pass

    def _finished(self) -> None:
        """Is called once the worker has finished working on this task.

        It takes care that a profiling time is saved and that the remaining
        stream information is logged.
        """
        # Update profiling info
        self.profiling['end_time'] = time.time()
        self.profiling['run_time'] = (self.profiling['end_time']
                                      - self.profiling['create_time'])
        # NOTE these are both approximate values as the worker process must
        # have ended prior to the call to this method

        # Read all remaining stream lines, then forward remaining and save all
        self.read_streams(max_num_reads=-1, forward_directly=True)
        self.save_streams(final=True)

        # Stop the stream-reading threads
        for stream_name in self.streams:
            self._stop_stream_reader(stream_name)

        # If given, call the callback function
        self._invoke_callback('finished')

        log.debug("Task %s: worker finished with status %s.",
                  self.name, self.worker_status)


# -----------------------------------------------------------------------------
# ... working with the multiprocessing module


def _target_wrapper(target, streams: dict, *args, **kwargs):
    """A wrapper around the multiprocessing.Process target function which
    takes care of stream handling.
    """
    import os
    import sys
    import logging
    log = logging.getLogger(__name__)

    # Stream handling . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
    # For stdout, there are the following options:
    #   - Leave as it is, which may lead to forwarding to the parent process
    #   - Redirect to file (which may be os.devnull)
    if streams['stdout'] is not None:
        sys.stdout = open(streams['stdout'], mode="w+")
        log.debug("Using file-based custom stdout:  %s", sys.stdout.name)

    # For stderr, there is one additional option: redirecting to stdout
    if streams['stderr'] is not None:
        if streams['stderr'] == streams['stdout']:
            sys.stderr = sys.stdout
            log.debug("Redirecting stderr to stdout now.")

        else:
            sys.stderr = open(streams['stderr'], mode="w+")
            log.debug("Using file-based custom stderr:  %s", sys.stderr.name)

    # Target invocation . . . . . . . . . . . . . . . . . . . . . . . . . . . .
    log.debug("Now invoking target ...")

    try:
        target(*args, **kwargs)
        log.debug("Target returned successfully.")

    # TODO Error handling
    finally:
        log.debug("Flushing and closing streams ...")
        sys.stdout.flush()
        sys.stderr.flush()

        sys.stdout.close()
        sys.stderr.close()

        # For good measure, reset the streams
        sys.stdout = sys.__stdout__
        sys.stderr = sys.__stderr__


class PopenMPProcess:
    """A wrapper around multiprocessing.Process that replicates (wide parts of)
    the interface of subprocess.Popen.
    """

    def __init__(self, args: tuple, kwargs: dict={},
                 stdin=None, stdout=None, stderr=None,
                 bufsize: int=-1, encoding: str='utf8'):
        """Creates a ``multiprocessing.Process`` and starts it.

        The interface here is a subset of ``subprocess.Popen`` that makes those
        features available that make sense for a ``multiprocessing.Process``,
        mainly: stream reading.

        Subsequently, the interface is quite a bit different to that of the
        ``multiprocessing.Process``. The most important arguments of that
        interface are ``target``, ``args``, and ``kwargs``, which can be set
        as follows:

            - ``target`` will be ``args[0]``
            - ``args`` will be ``args[1:]``
            - ``kwargs`` is an additional keyword argument that is not part of
                the ``subprocess.Popen`` interface typically.

        Regarding the stream arguments, the following steps are done to attach
        custom pipes: If any argument is a ``subprocess.PIPE`` or another
        stream specifier that is *not* ``subprocess.DEVNULL``, a new
        ``multiprocessing.Pipe`` and a reader thread will be established.

        Args:
            args (tuple): The ``target`` callable (``args[0]``) and subsequent
                positional arguments.
            kwargs (dict, optional): Keyword arguments for the ``target``.
            stdin (None, optional): The stdin stream
            stdout (None, optional): The stdout stream
            stderr (None, optional): The stderr stream
            bufsize (int, optional): The buffersize to use.
            encoding (str, optional): The encoding to use for the streams;
                should typically remain ``utf8``, using other values is not
                encouraged!
        """
        self._args = args
        self._kwargs = copy.deepcopy(kwargs)
        self._bufsize = bufsize
        self._encoding = encoding
        self._stdin = None
        self._stdout = None
        self._stderr = None

        # Prepare target and positional arguments, then spawn the process in a
        # custom context that always uses `spawn` (instead of `fork` on Linux).
        target, args = self._prepare_target_args(
            args, stdin=stdin, stdout=stdout, stderr=stderr,
        )
        _ctx = multiprocessing.get_context('spawn')
        self._proc = _ctx.Process(
            target=_target_wrapper, args=args, kwargs=self.kwargs, daemon=True
        )

        log.debug("Starting multiprocessing.Process for target %s ...", target)
        self._proc.start()

    def _prepare_target_args(self, args: tuple, *,
                             stdin, stdout, stderr,
                             ) -> Tuple[Callable, tuple]:
        """Prepares the target callable and stream objects"""
        # Extract target and the actual positional arguments
        target, args = args[0], args[1:]

        if not callable(target):
            raise TypeError(f"Given target {target} is not callable!")

        # Prepare the streams . . . . . . . . . . . . . . . . . . . . . . . . .
        if stdin is not None:
            raise NotImplementedError("stdin is not supported!")

        # Create lambdas that create file descriptors for the streams
        import tempfile
        File = tempfile.NamedTemporaryFile
        get_tempfile = lambda: File(mode='x+',  # exclusive creation
                                    buffering=self._bufsize,
                                    encoding=self._encoding,
                                    delete=False)

        # Need to map certain subprocess module flags to the stream creators
        get_stream = {
            None:               lambda: None,
            subprocess.DEVNULL: lambda: open(os.devnull, mode='w'),
            True:               get_tempfile,
            subprocess.PIPE:    get_tempfile,
            subprocess.STDOUT:  get_tempfile,
        }

        # Depending on the setting for stdout, let it create a file descriptor,
        # which is saved here in the parent process. For the child process, we
        # can only pass None or a string, which denotes the path to the file
        # (i.e. `File.name`) that should be used for this stream ...
        self._stdout = get_stream[stdout]()
        stdout = getattr(self._stdout, "name", self._stdout)

        # Same for stderr, but need to allow a shared pipe with stdout
        if stderr is subprocess.STDOUT and stdout is not None:
            self._stderr = self._stdout
            stderr = stdout
        else:
            self._stderr = get_stream[stderr]()
            stderr = getattr(self._stderr, "name", self._stderr)

        # Prepare arguments . . . . . . . . . . . . . . . . . . . . . . . . . .
        # Add those positional arguments that are used up by the wrapper
        wrapper_args = (
            target,
            dict(stdin=stdin, stdout=stdout, stderr=stderr),
        )
        return target, (wrapper_args + args)

    def __del__(self):
        """Custom destructor that closes the process and file descriptors"""
        try: self._proc.close()
        except: pass

        try: self._stdout.close()
        except: pass

        try: self._stderr.close()
        except: pass

    def __str__(self) -> str:
        return f"<PopenMPProcess for process: {self._proc}>"

    # .. subprocess.Popen interface ...........................................

    def poll(self) -> Union[int, None]:
        """Check if child process has terminated. Set and return ``returncode``
        attribute. Otherwise, returns None.

        With the underlying process being a multiprocessing.Process, this
        method is equivalent to the ``returncode`` property.
        """
        return self.returncode

    def wait(self, timeout=None):
        """Wait for the process to finish; blocking call.

        This method is not yet implemented, but will be!
        """
        raise NotImplementedError("PopenMPProcess.wait")

    def communicate(self, input=None, timeout=None):
        """Communicate with the process.

        This method is not yet implemented! Not sure if it will be ...
        """
        raise NotImplementedError("PopenMPProcess.communicate")

    def send_signal(self, signal: int):
        """Send a signal to the process. Only works for SIGKILL and SIGTERM."""
        if signal == SIGMAP['SIGTERM']:
            return self.terminate()

        elif signal == SIGMAP['SIGKILL']:
            return self.kill()

        raise NotImplementedError(
            f"Cannot send signal {signal} to multiprocessing.Process! "
            "The only supported signals are SIGTERM and SIGKILL."
        )

    def terminate(self):
        """Sends ``SIGTERM`` to the process"""
        self._proc.terminate()

    def kill(self):
        """Sends ``SIGKILL`` to the process"""
        self._proc.kill()

    @property
    def args(self) -> tuple:
        """The ``args`` argument to this process. Note that the returned tuple
        *includes* the target callable as its first entry.

        Note that these have already been passed to the process; changing them
        has no effect.
        """
        return self._args

    @property
    def kwargs(self):
        """Keyword arguments passed to the target callable.

        Note that these have already been passed to the process; changing them
        has no effect.
        """
        return self._kwargs

    @property
    def stdin(self):
        """The attached ``stdin`` stream"""
        return self._stdin

    @property
    def stdout(self):
        """The attached ``stdout`` stream"""
        return self._stdout

    @property
    def stderr(self):
        """The attached ``stderr`` stream"""
        return self._stderr

    @property
    def pid(self):
        """Process ID of the child process"""
        return self._proc.pid

    @property
    def returncode(self) -> Union[int, None]:
        """The child return code, set by ``poll()`` and ``wait()`` (and
        indirectly by ``communicate()``). A None value indicates that the
        process hasn’t terminated yet.

        A negative value ``-N`` indicates that the child was terminated by
        signal ``N`` (POSIX only).
        """
        return self._proc.exitcode


# .............................................................................

class MPProcessTask(WorkerTask):
    """A WorkerTask specialization that uses multiprocessing.Process instead
    of subprocess.Popen.

    It is mostly equivalent to :py:class:`~utopya.task.WorkerTask` but adjusts
    the private methods that take care of spawning the actual process and
    setting up the stream readers, such that the particularities of the
    :py:class:`~utopya.task.PopenMPProcess` wrapper are accounted for.
    """

    def _spawn_process(self, args, **popen_kwargs) -> PopenMPProcess:
        """This helper takes care *only* of spawning the actual process and
        potential error handling. It returns an PopenMPProcess instance, which
        has the same interface as subprocess.Popen.
        """
        return PopenMPProcess(args, **popen_kwargs)

    def _setup_stream_reader(self, *args, **kwargs):
        """Sets up the stream reader with ``follow=True``, such that the file-
        like streams that PopenMPProcess uses can be read properly."""
        return super()._setup_stream_reader(*args, follow=True, **kwargs)

    def _stop_stream_reader(self, name: str):
        """Stops the stream reader thread with the given name by telling its
        follow function to stop, thus ending iteration."""
        self.streams[name].get('thread').stop_follow = True


# -----------------------------------------------------------------------------

class TaskList:
    """The TaskList stores Task objects in it, ensuring that none is in there
    twice and allows to lock it to prevent adding new tasks.
    """

    def __init__(self):
        """Initialize an empty TaskList."""
        self._l = []
        self._locked = False

    def __len__(self) -> int:
        """The length of the TaskList."""
        return len(self._l)

    def __contains__(self, val: Task) -> bool:
        """Checks if the given object is contained in this TaskList."""
        if not isinstance(val, Task):
            # Cannot be part of this TaskList
            return False
        return val in self._l

    def __getitem__(self, idx: int) -> Task:
        """Returns the item at the given index in the TaskList."""
        return self._l[idx]

    def __iter__(self):
        """Iterate over the TaskList"""
        return iter(self._l)

    def __eq__(self, other) -> bool:
        """Tests for equality of the task list by forwarding to _l attribute"""
        return bool(self._l == other)

    def lock(self):
        """If called, the TaskList becomes locked and allows no further calls
        to the append method.
        """
        self._locked = True

    def append(self, val: Task):
        """Append a Task object to this TaskList

        Args:
            val (Task): The task to add

        Raises:
            RuntimeError: If TaskList object was locked
            TypeError: Tried to add a non-Task type object
            ValueError: Task already added to this TaskList
        """

        if self._locked:
            raise RuntimeError("TaskList locked! Cannot append further tasks.")

        elif not isinstance(val, Task):
            raise TypeError("TaskList can only be filled with "
                            "Task objects, got object of type {}, "
                            "value {}".format(type(val), val))
        elif val in self:
            raise ValueError("Task '{}' (uid: {}) was already added "
                             "to this TaskList, cannot be added "
                             "again.".format(val.name, val.uid))

        # else: Everything ok, append the object
        self._l.append(val)

    def __add__(self, tasks: Sequence[Task]):
        """Appends all the tasks in the given iterable to the task list"""
        for t in tasks:
            self.append(t)
        return self
