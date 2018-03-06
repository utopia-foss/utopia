"""The WorkerManager class."""

import os
import subprocess
import queue
import threading
import time
import json
import warnings
import logging
from collections import OrderedDict
from typing import Union, Callable
from typing.io import BinaryIO

# Initialise logger
log = logging.getLogger()


# -----------------------------------------------------------------------------

class WorkerManager:
    """The WorkerManager class manages tasks and worker processes to execute them.

    It is non-blocking and the main thread does not use significant CPU time for low poll frequencies (< 100/s).
    At the same time, it reads the worker's stream in yet separate non-blocking threads.
    """

    def __init__(self, num_workers: Union[int, str], poll_freq: float=42, QueueCls=queue.Queue):
        """Initialize the worker manager.
        
        Args:
            num_workers (Union[int, str]): The number of workers that can work
                in parallel. If `auto`, uses os.cpu_count(). If negative,
                deduces that many from the CPU count
            poll_freq (float, optional): How many times per second the workers
                should be polled. More precisely: 1/poll_freq gives the sleep
                time between polls. Should not be choosen too high, as this
                determines the CPU load of the main thread.
            QueueCls (Class, optional): Which class to use for the Queue.
        """
        # Initialize property-managed attributes
        self._num_workers = None
        self._workers = OrderedDict()
        self._working = []
        self._poll_freq = None
        self._task_cnt = 0

        # Initialize method-managed attributes
        self._tasks = QueueCls()

        # Hand over arguments
        self.poll_freq = poll_freq

        if num_workers == 'auto':
            self.num_workers = os.cpu_count()
        elif num_workers < 0:
            self.num_workers = os.cpu_count() + num_workers
        else:
            self.num_workers = num_workers

    # Properties ..............................................................
    @property
    def tasks(self) -> queue.Queue:
        """The task queue."""
        return self._tasks

    @property
    def workers(self) -> dict:
        """The dictionary that keeps track of all workers."""
        return self._workers

    @property
    def num_workers(self) -> int:
        """The number of workers that can work in parallel."""
        return self._num_workers

    @num_workers.setter
    def num_workers(self, val):
        """Set the number of workers that can work in parallel."""
        if val <= 0 or not isinstance(val, int):
            raise ValueError("Need positive integer for number of workers, got ", val)
        elif val > os.cpu_count():
            warnings.warn("Set WorkerManager to use more parallel workers ({})"
                          "than there are cpu cores ({}) on this "
                          "machine.".format(val, os.cpu_count()),
                          UserWarning)

        self._num_workers = val
        log.debug("Set number of workers to %d", self.num_workers)

    @property
    def working(self) -> list:
        """The list of currently working workers.

        Note that this information might not be up-to-date; a process might quit just after the list has been updated.
        """
        return self._working

    @property
    def poll_freq(self) -> float:
        """The poll frequency in polls/second. Strictly speaking: the sleep time between two polls, which roughly equals the poll frequency."""
        return self._poll_freq

    @poll_freq.setter
    def poll_freq(self, val) -> None:
        """Set the poll frequency to a positive value."""
        if val <= 0.:
            raise ValueError("Poll frequency needs to be positive, was", val)
        self._poll_freq = val

    @property
    def task_count(self) -> int:
        """Returns the number of tasks that this manager *ever* took care of. Careful: This is NOT the current number of tasks in the queue!"""
        return self._task_cnt

    @property
    def free_workers(self) -> bool:
        """Returns True if not `num_workers` amount of workers are busy at the moment."""
        return bool(len(self.working) < self.num_workers)


    # Public API ..............................................................

    def add_task(self, args: Union[str, list], *, priority: int=None, read_stdout: bool=True, **kwargs):
        """Adds a task to the queue.
        
        A priority can be assigned to the task, but will only be used during task retrieval if the WorkerManager was initialized with a queue.PriorityQueue as task manager.
        
        Additionally, each task will be assigned an ID; this is used to preserve order in a priority queue if the priority of two or more tasks is the same.
        
        Args:
            args (str): The arguments to subprocess.Popen, i.e.: the command
                that should be executed in the new process
            priority (int, optional): The priority of this task; is only used
                if the task queue is a priority queue
            read_stdout (bool, optional): Whether to create a thread that reads
                the stdout of the created process
            **kwargs: Additional task arguments
        """
        # Generate a new ID for the task (using the current task counter)
        task_id = self.task_count

        log.debug("Adding task with ID %d ...", task_id)
        log.debug("  Task args:   %s", args)
        log.debug("  read_stdout: %s", read_stdout)
        log.debug("  kwargs:      %s", kwargs)

        # Parse the task argument, ensuring it is a tuple
        if isinstance(args, str):
            args = args.split(' ')
        args = tuple(args)

        # Put it into the task queue and increment the task counter
        self._tasks.put_nowait((priority,
                                task_id,
                                dict(args=args, read_stdout=read_stdout,
                                     **kwargs)))
        self._increment_task_count()

        log.debug("Task %s added.", task_id)

    def start_working(self, detach: bool=False):
        """Upon call, all enqueued tasks will be worked on sequentially.
        
        Args:
            detach (bool, optional): If False (default), the WorkerManager
                will block here, as it continuously polls the workers and
                distributes tasks.
        
        Raises:
            NotImplementedError: for `detach` True
        """
        log.info("Starting to work ...")

        if detach:
            # TODO implement the content of this in a separate thread.
            raise NotImplementedError("It is currently not possible to "
                                      "detach the WorkerManager from the "
                                      "main thread.")

        while not self.tasks.empty() or self.working:
            # Check if there are free workers and remaining tasks.
            if self.free_workers and not self.tasks.empty():
                # Yes. => Grab a task and start working on it
                # Conservative approach: one task is grabbed here, even if there are more than one free workers
                self._grab_task()

            # Gather the streams of all working workers
            self._read_worker_streams()

            # Poll the workers
            self._poll_workers()
            # NOTE this will also remove no longer active workers

            # Sleep until next poll
            time.sleep(1/self.poll_freq)

        log.info("Finished working. Total tasks worked on: %d",
                 self.task_count)


    # Non-public API ..........................................................

    def _increment_task_count(self) -> None:
        """Increments the task counter."""
        self._task_cnt += 1

    def _grab_task(self, task=None) -> subprocess.Popen:
        """Will initiate that a new (or already existing) worker will work on a task. If a task is given, that one will be used; if not, a task will be taken from the queue.
        
        Returns the process the task is being worked on at. If no task was given and the queue is empty, will raise `queue.Empty`.
        
        Args:
            task (None, optional): If given, this task will be used rather than
                one that is gotten from the task queue.
        
        Returns:
            subprocess.Popen: The created process object
        """

        if not task:
            # Get one from the queue
            _, task_id, task = self._tasks.get_nowait()
            log.debug("Got task %s from queue.", task_id)

        # Interpret the given task
        # NOTE optionally, add an interpretation here, e.g. a mapping to an already existing worker

        # Spawn a worker and return the resulting process
        return self._spawn_worker(**task)

    def _spawn_worker(self, *, args: tuple, read_stdout: bool, line_read_func: Callable=None, **popen_kwargs) -> subprocess.Popen:
        """Spawn a worker process using subprocess.Popen and manage the corresponding queue and thread for reading the stdout stream. The new worker process is registered with the class.
        
        Args:
            args (tuple): The arguments to the Popen call, i.e. the command to
                spawn a new process with
            read_stdout (bool): Whether stdout should be streamed and its
                output queued
            line_read_func (Callable, optional): The function to read the
                stdout with. If not present, will use `enqueue_lines`.
            debug (bool, optional): Attaches all streams to main thread stdout
            **popen_kwargs: kwargs passed to subprocess.Popen
        
        Returns:
            subprocess.Popen: The created process object
        
        No Longer Raises:
            ValueError: When stdout should be read but no `line_read_func` was
                supplied during initialisation
        """

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

        # Spawn the child process with the given arguments
        log.debug("Spawning worker process with args:  %s", args)
        proc = subprocess.Popen(args,
                                bufsize=1, # line buffered
                                stdout=stdout, stderr=stderr,
                                **popen_kwargs)
        create_time = time.time() # approximate
        log.debug("Spawned worker process with PID %s.", proc.pid)
        # ... it is running now.
        
        # If enabled, prepare for reading the output
        if read_stdout:
            # Generate the thread that reads the stream and populates the queue
            t = threading.Thread(target=line_read_func,
                                 kwargs=dict(queue=q, stream=proc.stdout))
            # Set to be a daemon thread => will die with the parent thread
            t.daemon = True

            # Start the thread; this will lead to line_read_func being called
            t.start()

            # Create a dictionary with stream information
            streams = dict(out=dict(queue=q, thread=t, log=[]))
            # NOTE could have more streams here, but focus on stdout right now

            log.debug("Added thread to read worker's combined STDOUT and STDERR.")
        else:
            streams = None

        # Register the worker process with the class
        self._register_worker(proc=proc, args=args, streams=streams, create_time=create_time)
        
        return proc

    def _register_worker(self, *, proc: subprocess.Popen, args: str, streams: dict, create_time: float, **kwargs) -> None:
        """Registers a worker process with the class. This should be called soon after a process was created.
        
        Args:
            proc (subprocess.Popen): The process object to register
            args (str): The arguments it was created with
            streams (dict): The corresponding stream information
            create_time (float): The creation time
            **kwargs: Any other information to store
        """

        # Register in the working list
        self.working.append(proc)
        # NOTE if the process has already finished by this time, it will be removed from the working list at the next poll time

        # Gather information of this worker in the worker dict
        self.workers[proc] = dict(proc=proc,
                                  args=args,
                                  # status information
                                  status=None, # assume running here
                                  create_time=create_time,
                                  end_time=None,
                                  # TODO monitor CPU time rather than wall time
                                  # other information
                                  streams=streams,
                                  **kwargs)

        log.debug("Worker process %s registered.", proc.pid)

    def _poll_workers(self):
        """Will poll all workers that are in the working list and remove them from that list if they are no longer alive.
        """
        rebuild = False

        for proc in self.working:
            pr = proc.poll()

            if pr is not None:
                # Worker finished. Mark for removal from list.
                self._worker_finished(proc)
                rebuild = True

        if rebuild:
            # One process finished; have to rebuild the list of working workers. Opposite approach: (in-place) recreate list of those that are still working
            self.working[:] = [proc for proc in self.working
                               if self.workers[proc]['status'] is None]

        log.debug("Polled workers. Currently %d alive.", len(self.working))

    def _worker_finished(self, proc: subprocess.Popen) -> None:
        """Updates the entry of the worker dict after it has finished working.
        
        Args:
            proc (subprocess.Popen): The process to apply these actions to
        """
        self.workers[proc]['status'] = proc.returncode
        self.workers[proc]['end_time'] = time.time()

        # Read the remaining entries from the worker stream
        self._read_worker_stream(proc, max_num_reads=-1)

    def _read_worker_streams(self, stream_name: str='out') -> None:
        """Gathers all working workers' streams with the given `stream_name`.
        
        Args:
            stream_name (str, optional): The stream name to read
        """
        for proc in self.working:
            self._read_worker_stream(proc, stream_name=stream_name)

    def _read_worker_stream(self, proc: subprocess.Popen, stream_name: str='out', forward_to_log: bool=True, max_num_reads: int=1) -> None:
        """Gather a single entry from the given worker's stream queue and store the value in the stream log.
        
        Args:
            proc (subprocess.Popen): The processes to read the stream of
            stream_name (str, optional): The stream name
            forward_to_log (bool, optional): Whether the read stream should be
                forwarded to this module's log.info() function
            max_num_reads (int, optional): How many lines should be read from
                the buffer. For -1, reads the whole buffer.
                WARNING: Do not make this value too large as it could block the
                whole reader thread of this worker.
        """

        if not self.workers[proc]['streams']:
            # There are no streams to read
            return

        # Get the stream dictionary and the queue
        stream = self.workers[proc]['streams'][stream_name]
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
                if forward_to_log:
                    # print it to the parent processe's stdout
                    log.info("  process %s:  %s", proc.pid, entry)

                # Write to the stream's log
                stream['log'].append(entry)            


# Helper functions ------------------------------------------------------------
# These solely relate to the WorkerManager, thus not in tools

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
        # At least try to decode it
        try:
            return line.decode('utf8')
        except UnicodeDecodeError:
            # Return the (unparsed, undecoded) line into the queue
            return line
