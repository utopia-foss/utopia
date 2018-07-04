"""The Task class supplies a container for all information needed for a task.

The WorkerTask specialises on tasks for the WorkerManager."""

import uuid
import time
import json
import queue
import threading
import subprocess
import warnings
import logging
from typing import Callable, Union, Dict, List
from typing.io import BinaryIO

import numpy as np

# Initialise logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

class Task:
    """The Task is a container for a task handled by the WorkerManager.
    
    It aims to provide the necessary interfaces for the WorkerManager to easily
    associate tasks with the corresponding workers and vice versa.
    """

    __slots__ = ('_name', '_priority', '_uid', 'callbacks')

    def __init__(self, *, name: str=None, priority: float=None, callbacks: Dict[str, Callable]=None):
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
        """
        # Carry over arguments attributes
        self._name = str(name) if name else None
        self._priority = priority if priority is not None else np.inf
        
        # Create a unique ID
        self._uid = uuid.uuid1()

        # Save the callbacks
        self.callbacks = callbacks

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
        """The task's priority. Default is +inf, which is the lowest priority."""
        return self._priority

    @property
    def order_tuple(self) -> tuple:
        """Returns the ordering tuple (priority, uid.time)"""
        return (self.priority, self.uid.time)

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
        # NOTE we trust 'uuid' that the IDs are unique therefore different tasks
        # can not get the same ID --> are different in ordering

    # Private methods .........................................................

    def _invoke_callback(self, name: str):
        """If given, invokes the callback function with the name `name`."""
        if self.callbacks and name in self.callbacks:
            self.callbacks[name](self)

# -----------------------------------------------------------------------------

class WorkerTask(Task):
    """A specialisation of the Task class that is aimed at use in the WorkerManager.
    
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

    def __init__(self, *, setup_func: Callable=None, setup_kwargs: dict=None, worker_kwargs: dict=None, callbacks: Dict[str, Callable]=None, **task_kwargs):
        """Initialize a WorkerTask object, a specialization of a task for use in the WorkerManager.
        
        Args:
            setup_func (Callable, optional): The function to call before the
                worker process is spawned
            setup_kwargs (dict, optional): The kwargs to call setup_func with
            worker_kwargs (dict, optional): The kwargs needed to spawn the
                worker. Note that these are also passed to setup_func and, if a
                setup_func is given, the return value of that function will be
                used for the worker_kwargs.
            write_stream_to (sstr, optional): Description
            callbacks (Dict[str, Callable], optional): Callbacks available in
                the WorkerTask follow the life of a process; available keys
                are: 'spawn', 'finished', 'after_signal'.
            **task_kwargs: Arguments to be passed to Task.__init__
        
        Raises:
            ValueError: If neither `setup_func` nor `worker_kwargs` were given
        """

        super().__init__(callbacks=callbacks, **task_kwargs)

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
        self.setup_kwargs = setup_kwargs
        self.worker_kwargs = worker_kwargs

        # Create empty attributes to be filled with worker information
        self._worker = None
        self._worker_pid = None
        self._worker_status = None
        self.streams = dict()
        self.profiling = dict()

        log.debug("Finished setting up task '%s' as a WorkerTask.", self.name)
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
        """The worker processe's current status or False, if there is no worker spawned yet.

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
        
        else:
            return self._worker_status

    # Magic methods ...........................................................

    def __str__(self) -> str:
        """Return basic WorkerTask information."""
        return "WorkerTask<uid: {}, priority: {}, worker_status: {}>".format(self.uid, self.priority, self.worker_status)

    # Public API ..............................................................

    def spawn_worker(self) -> subprocess.Popen:
        """Spawn a worker process using subprocess.Popen and manage the corresponding queue and thread for reading the stdout stream.
        
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
            worker_kwargs = self.setup_func(worker_kwargs=self.worker_kwargs,
                                            **self.setup_kwargs)
        else:
            log.debug("No setup function given; using the `worker_kwargs` "
                      "directly.")
            worker_kwargs = self.worker_kwargs

        # Extract information from the worker_kwargs
        args = worker_kwargs['args']
        popen_kwargs = worker_kwargs.get('popen_kwargs', {})

        read_stdout = worker_kwargs.get('read_stdout', True)
        line_read_func = worker_kwargs.get('line_read_func')

        save_streams = worker_kwargs.get('save_streams', False)
        save_streams_to = worker_kwargs.get('save_streams_to')

        forward_streams = worker_kwargs.get('forward_streams', False)
        streams_log_lvl = worker_kwargs.get('streams_log_lvl', None)

        # Perform some checks
        if not isinstance(args, tuple):
            raise TypeError("Need argument `args` to be of type tuple, "
                            "got {} with value {}. Refusing to even try to "
                            "spawn a worker process.".format(type(args), args))

        if read_stdout:
            # If no `line_read_func` was given, read the default
            if not line_read_func:
                log.debug("No `line_read_func` given; will use "
                          "`enqueue_lines` instead.")
                line_read_func = enqueue_lines

            # Establish queue for stream reading, creating a new pipe for STDOUT and forwarding STDERR into that same pipe
            q = queue.Queue()
            stdout = subprocess.PIPE
            stderr = subprocess.STDOUT
        else:
            # No stream-reading is taking place; forward all streams to devnull
            stdout = stderr = subprocess.DEVNULL

        # Done with the checks now.
        # Spawn the child process with the given arguments
        log.debug("Spawning worker process with args:\n  %s", args)
        try:
            proc = subprocess.Popen(args,
                                    bufsize=1, # line buffered
                                    stdout=stdout, stderr=stderr,
                                    **popen_kwargs)
        except FileNotFoundError as err:
            raise FileNotFoundError("Could not find command to execute! Did "
                                    "you build your binary?") from err

        # Save the approximate creation time (as soon as possible)
        self.profiling['create_time'] = time.time()
        log.debug("Spawned worker process with PID %s.", proc.pid)
        # ... it is running now.

        # Associate the process with the task
        self.worker = proc

        # If enabled, prepare for reading the output
        if read_stdout:
            # Generate the thread that reads the stream and populates the queue
            t = threading.Thread(target=line_read_func,
                                 kwargs=dict(queue=q, stream=proc.stdout))
            # Set to be a daemon thread => will die with the parent thread
            t.daemon = True

            # Start the thread; this will lead to line_read_func being called
            t.start()

            # Save the stream information in the WorkerTask object
            # This includes two counters for the number of lines saved and
            # forwarded, which are used by the save_/forward_streams methods
            self.streams['out'] = dict(queue=q, thread=t, log=[],
                                       save=save_streams, save_path=None, 
                                       forward=forward_streams,
                                       log_level=streams_log_lvl,
                                       lines_saved=0, lines_forwarded=0)
            # NOTE could have more streams here, but focus on stdout right now

            log.debug("Added thread to read worker %s's combined STDOUT and "
                      "STDERR.", self.name)

            # If configured to save, save the 
            if save_streams:
                if not save_streams_to:
                    raise ValueError("Was told to `save_streams` but did not "
                                     "find a `save_streams_to` argument in "
                                     "`worker_kwargs`: {}."
                                     "".format(worker_kwargs))

                # Perform a format operation to generate the path
                save_path = save_streams_to.format(name='out')
                self.streams['out']['save_path'] = save_path

        # If given, call the callback function
        self._invoke_callback('spawn')

        return self.worker

    def read_streams(self, stream_names: list='all', max_num_reads: int=1) -> None:
        """Read the streams associated with this task's worker.
        
        Args:
            stream_names (list, optional): The list of stream names to read.
                If 'all' (default), will read all streams.
            max_num_reads (int, optional): How many lines should be read from
                the buffer. For -1, reads the whole buffer.
                WARNING: Do not make this value too large as it could block the
                whole reader thread of this worker.
        
        Returns:
            None: Description
        """
        def read_single_stream(stream: dict, stream_name: str, max_num_reads=max_num_reads):
            """A function to read a single stream"""            
            q = stream['queue']

            # In certain cases, read as many as queue reports to have
            if max_num_reads == -1:
                max_num_reads = q.qsize()
                # NOTE this value is approximate; thus, this should only be called if it is reasonably certain that the queue size will not change

            # Perform the read operations
            for _ in range(max_num_reads):
                # Try to read a single entry
                try:
                    entry = q.get_nowait()
                except queue.Empty:
                    break
                else:
                    # Got entry, write it to the stream's log
                    stream['log'].append(entry)

        if not self.streams:
            # There are no streams to read
            log.debug("No streams to read for WorkerTask '%s' (uid: %s).",
                      self.name, self.uid)
            return

        elif stream_names == 'all':
            # Gather list of stream names
            stream_names = list(self.streams.keys())

        # Loop over stream names and call the function to read a single stream
        for stream_name in stream_names:
            # Get the corresponding stream dict
            stream = self.streams[stream_name]
            # NOTE: This way a non-existent stream_name will not pass silently
            #       put raise a KeyError

            read_single_stream(stream, stream_name)

        return

    def save_streams(self, stream_names: list='all'):
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
            lines_to_save = stream['log'][slice(stream['lines_saved'], None)]

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
                    f.write("Log of '{}' stream of WorkerTask '{}'\n\n"
                            "".format(stream_name, self.name))

                # Write the lines to save
                f.write("\n".join(lines_to_save))

                # Ensure new line at the end
                f.write("\n")

            # Update counter
            stream['lines_saved'] += len(lines_to_save)

            log.debug("Saved %d lines of stream '%s'.",
                      len(lines_to_save), stream_name)

    def forward_streams(self, stream_names: list='all') -> bool:
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
        
        Deleted Parameters:
            log_level (Union[int, None], optional): 
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
            lines = stream['log'][stream['lines_forwarded']:]
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

    def signal_worker(self, signal: Union[str, int]):
        """Sends a signal to this WorkerTask's worker.
        
        Args:
            signal (Union[str, int]): The signal to send. Can be SIGTERM or
                SIGKILL or a valid signal number.
        
        Raises:
            ValueError: When an invalid `signal` argument was given
        """
        if signal == 'SIGTERM':
            log.debug("Terminating worker of task %s ...", self.name)
            self.worker.terminate()

        elif signal == 'SIGKILL':
            log.debug("Killing worker of task %s ...", self.name)
            self.worker.kill()

        elif isinstance(signal, int):
            log.debug("Sending signal %d to worker of task %s ...",
                      signal, self.name)
            self.worker.send_signal(signal)

        else:
            raise ValueError("Invalid argument `signal`: Got '{}', but need "
                             "either SIGTERM, SIGKILL, or an integer signal "
                             "number.".format(signal))

        self._invoke_callback('after_signal')

    # Private API .............................................................

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
        self.read_streams(max_num_reads=-1)
        self.forward_streams()
        self.save_streams()

        # If given, call the callback function
        self._invoke_callback('finished')

        log.debug("Task %s: worker finished with status %s.",
                  self.name, self.worker_status)

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

# -----------------------------------------------------------------------------
# These solely relate to the WorkerTask class, thus not in the tools module

def enqueue_lines(*, queue: queue.Queue, stream: BinaryIO, parse_func: Callable=None) -> None:
    """From the given stream, read line-buffered lines and add them to the provided queue. If they are json-parsable, parse them and add the parsed dictionary to the queue.

    Args:
        queue (queue.Queue): The queue object to put the read line into
        stream (BinaryIO): The stream identifier
        parse_func (Callable, optional): A parse function that the read line
            is passed through
    """

    if not parse_func:
        # Define a pass-through function, doing nothing
        parse_func = lambda line: line

    # Read the lines and put them into the queue
    for line in iter(stream.readline, b''): # <-- thread waits here for new
                                            #     lines, without idle looping
        # Got a line (byte-string, assumed utf8-encoded)
        try:
            # Try to decode and strip newline
            line = line.decode('utf8').rstrip()

        except UnicodeDecodeError:
            # Remains a bytestring
            pass

        # else: could decode

        # Send it through the parse function and add it to the queue
        queue.put_nowait(parse_func(line))

    # Everything read. Close the stream
    stream.close()
    # Thread also finishes here.

def parse_json(line: str) -> Union[dict, str]:
    """A json parser that can be passed to enqueue_lines.

    It tries to decode the line, and parse it as a json.
    If that fails, it will still try to decode the string.
    If that fails yet again, the unchanged line will be returned.

    Args:
        line (str): The line to decode, assumed byte-string, utf8-encoded

    Returns:
        Union[dict, str]: Either the decoded json, or, if that failed, the str
    """
    try:
        d = json.loads(line, encoding='utf8')
    
    except (json.JSONDecodeError, TypeError) as err:
        # One of the expected errors occured
        log.debug("%s: %s for line '%s'.", err.__class__.__name__, err, line)
        
        # Just return it as the string representation
        return str(line)

    except Exception as err:
        # Failed to do that for another reason; be more verbose about it
        log.error("%s: %s for line '%s'.", err.__class__.__name__, err, line)

        # Still return the string representation
        return str(line)

    # Could load it. Still check, if it is a dictionary. If not, return as str
    if isinstance(d, dict):
        return d
    return str(d)

def enqueue_json(*, queue: queue.Queue, stream: BinaryIO, parse_func: Callable=parse_json) -> None:
    """Wrapper function for enqueue_lines with parse_json set as parse_func."""
    return enqueue_lines(queue=queue, stream=stream, parse_func=parse_func)
