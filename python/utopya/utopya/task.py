"""The Task class supplies a container for all information needed for a task.

The WorkerTask specialises on tasks for the WorkerManager."""

import time
import json
import queue
import threading
import subprocess
import warnings
import logging
from typing import Callable, Union
from typing.io import BinaryIO

# Initialise logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

class Task:
    """The Task is a container for a task handled by the WorkerManager.
    
    It aims to provide the necessary interfaces for the WorkerManager to easily
    associate tasks with the corresponding workers and vice versa.
    
    Attributes:
        uid (int): the task ID, assumed to be a unique integer
        priority (int): The task priority
    """

    def __init__(self, *, uid: int, priority: int=None):
        """Initialize a Task object.

        Args:
            uid (int): The task's unique ID
            priority (int, optional): The priority of this task; is only used
                if the task queue is a priority queue
        """
        # Create property-managed attributes
        self._uid = None

        # Carry over attributes
        self.uid = uid
        self._priority = priority

        log.debug("Initialized Task with ID %d.", self.uid)

    # Properties --------------------------------------------------------------
    
    @property
    def uid(self) -> int:
        """The task's ID, assumed to be unique"""
        return self._uid

    @uid.setter
    def uid(self, uid: int):
        """Checks if the given ID is valid, then sets it and makes it read-only."""
        if not isinstance(uid, int):
            raise TypeError("Need integer UID, got " + str(type(uid)))
        
        elif uid < 0:
            raise ValueError("Negative UID not allowed: " + str(uid))
        
        elif self.uid is not None:
            raise RuntimeError("Task UID was already set and cannot be "
                               "changed!")

        else:
            self._uid = uid
            log.debug("Set task UID:  %d", self.uid)

    @property
    def priority(self) -> float:
        """The task priority, usually a """
        return self._priority

    @property
    def order_tuple(self) -> tuple:
        """Returns the ordering tuple (priority, task ID)"""
        return (self.priority, self.uid)

    # Magic methods -----------------------------------------------------------
    
    def __str__(self) -> str:
        return "Task<uid: {}, priority: {}>".format(self.uid, self.priority)

    # Rich comparisons, needed in PriorityQueue
    # NOTE only need to implement __lt__, __le__, and __eq__
    
    def __lt__(self, other):
        return bool(self.order_tuple < other.order_tuple)
    
    def __le__(self, other):
        return bool(self.order_tuple <= other.order_tuple)
    
    def __eq__(self, other):
        return bool(self.order_tuple == other.order_tuple)
        # NOTE that this should only occur if comparing to itself
        # TODO consider throwing an error here; identity should be checked via is keyword rather than ==

# -----------------------------------------------------------------------------

class WorkerTask(Task):
    """A specialisation of the Task class that is aimed at use in the WorkerManager.
    
    It is able to spawn a worker process, executing the task. Task execution
    is non-blocking. At the same time, the worker's stream can be read in via
    another non-blocking thread.

    Attributes:
        setup_func (Callable): The setup function to use before this task is
            being worked on
        setup_kwargs (dict): The kwargs to use to call setup_func
        worker_kwargs (dict): The kwargs to use to spawn a worker process
    """

    def __init__(self, *, setup_func: Callable=None, setup_kwargs: dict=None, worker_kwargs: dict=None, **task_kwargs):
        """Initialize a WorkerTask object, a specialization of a task for use in the WorkerManager.
        
        Args:
            setup_func (Callable, optional): The function to call before the
                worker process is spawned
            setup_kwargs (dict, optional): The kwargs to call setup_func with
            worker_kwargs (dict, optional): The kwargs needed to spawn the
                worker. Note that these are also passed to setup_func and, if a
                setup_func is given, the return value of that function will be
                used for the worker_kwargs.
            **task_kwargs: Arguments to be passed to Task.__init__
        
        Raises:
            ValueError: If neither `setup_func` nor `worker_kwargs` were given
        """

        super().__init__(**task_kwargs)

        # Check the argument values
        if setup_func:
            setup_kwargs = setup_kwargs if setup_kwargs else dict()

            if worker_kwargs:
                warnings.warn("Received argument `worker_kwargs` despite a "
                              "setup function having been given; the passed "
                              "`worker_kwargs` will not be used!",
                              UserWarning)
        
        elif worker_kwargs:
            if setup_kwargs:
                warnings.warn("worker_kwargs given but also setup_kwargs "
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

        log.debug("Finished setting up task %d as a worker task.", self.uid)
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

        log.debug("Associated worker process %d with task %d.",
                  self.worker_pid, self.uid)

    @property
    def worker_pid(self) -> int:
        """The process ID of the associated worker process"""
        return self._worker_pid

    @property
    def worker_status(self) -> Union[int, None]:
        """The worker processe's current status or False, if there is no worker spawned yet.
        
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
        read_stdout = worker_kwargs.get('read_stdout', True)
        line_read_func = worker_kwargs.get('line_read_func')
        popen_kwargs = worker_kwargs.get('popen_kwargs', {})

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
        proc = subprocess.Popen(args,
                                bufsize=1, # line buffered
                                stdout=stdout, stderr=stderr,
                                # stdout=stdout, stderr=stderr,
                                **popen_kwargs)

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
            self.streams['out'] = dict(queue=q, thread=t, log=[])
            # NOTE could have more streams here, but focus on stdout right now

            log.debug("Added thread to read worker's combined STDOUT and STDERR.")        

        return self.worker

    def read_streams(self, stream_names: list='all', forward_streams: bool=True, max_num_reads: int=1) -> None:
        """Read the streams associated with this task's worker.
        
        Args:
            stream_names (list, optional): The list of stream
                names to read. If 'all' (default), will read all streams.
            forward_streams (bool, optional): Whether the read stream should be
                forwarded to this module's log.info() function
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
                    # got entry, do something with it
                    if forward_streams:
                        # print it to the parent processe's stdout
                        log.info("  Task %4d %s:   %s",
                                 self.uid, stream_name, entry)

                    # Write to the stream's log
                    stream['log'].append(entry)

        if not self.streams:
            # There are no streams to read
            log.debug("No streams to read for task %d.", self.uid)
            return

        elif stream_names == 'all':
            # Gather list of stream names
            stream_names = list(self.streams.keys())

        # Loop over stream names and call the function to read a single stream
        for stream_name in stream_names:
            read_single_stream(self.streams[stream_name], stream_name)

        return

    # Private API .............................................................

    def _finished(self) -> None:
        """Is called once the worker has finished working on this task.

        It takes care that a profiling time is saved and that the remaining
        stream information is logged.
        """
        self.profiling['end_time'] = time.time()  # approximate

        # Read all remaining stream lines
        self.read_streams(max_num_reads=-1)

        log.debug("Worker of task %d finished with status %s.",
                  self.uid, self.worker_status)

# -----------------------------------------------------------------------------

class TaskList(list):
    """This list is meant to store tasks in it.

    It disallows the use of some parent methods.
    """

    # Adapt some methods to make this a list of tasks .........................

    def __setitem__(self, idx: int, val: Task):
        """Set the item with the given index. Only allow Tasks with the correct uid."""
        self._check_item_val_and_idx(idx=idx, val=val)

        # Everything ok, set the item
        super().__setitem__(idx, val)

    def append(self, val: Task):
        """Append a Task object to this TaskList"""
        self._check_item_val_and_idx(idx=len(self), val=val)

        # Everything ok, append the object
        super().append(val)

    @staticmethod
    def _check_item_val_and_idx(*, idx: int, val: Task):
        """Checks the validity of the given item value and index.
        
        Args:
            idx (int): The index where this task should be inserted
            val (Task): The item to be set, i.e. the Task object
        
        Raises:
            TypeError: Given item was not a Task object
            ValueError: Given Task did not match the allowed index
        """
        if not isinstance(val, Task):
            raise TypeError("TaskList can only be filled with tasks, got "
                            "object of type {}, value {}".format(type(val),
                                                                 val))
        
        elif val.uid != idx:
            raise ValueError("Task's UID and TaskList index do not match!")

        # No raise: everything ok.

    # Disallow any methods that change the existing content ...................

    def __add__(self, other):
        raise NotImplementedError("Please use append to add Task objects.")
    
    def __iadd__(self, other):
        raise NotImplementedError("Please use append to add Task objects.")

    def __delitem__(self, idx):
        raise NotImplementedError("TaskList does not allow item deletion.")

    def clear(self):
        raise NotImplementedError("TaskList does not allow clearing.")
    
    def insert(self, idx, item):
        raise NotImplementedError("TaskList does not allow insertion.")
    
    def pop(self, idx):
        raise NotImplementedError("TaskList does not allow popping items.")
    
    def reverse(self):
        raise NotImplementedError("TaskList does not allow reversion.")

    def sort(self):
        raise NotImplementedError("TaskList does not allow sorting.")
    
    def remove(self, idx):
        raise NotImplementedError("TaskList does not allow removing.")
    
    def extend(self, *args):
        raise NotImplementedError("TaskList does not allow extension. "
                                  "Please use append to add tasks.")

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
    for line in iter(stream.readline, b''): # <-- thread waits here for new lines, without idle looping
        # Got a line (byte-string, assumed utf8-encoded)
        # Try to decode and strip newline
        try:
            line = line.decode('utf8').rstrip()
        except UnicodeDecodeError:
            # Remains a bytestring
            pass
        else:
            # Could decode. Pass to parse function
            line = parse_func(line)

        # Send it through the parse function
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
        return json.loads(line, encoding='utf8')
    except json.JSONDecodeError:
        # Failed to do that. Just return the unparsed line
        return line

def enqueue_json(*, queue: queue.Queue, stream: BinaryIO, parse_func: Callable=parse_json) -> None:
    """Wrapper function for enqueue_lines with parse_json set as parse_func."""
    return enqueue_lines(queue=queue, stream=stream, parse_func=parse_func)
