"""The WorkerManager class manages tasks and worker processes to execute them.

It is non-blocking and the main thread does not use significant CPU time for low poll frequencies (< 100/s). At the same time, it reads the worker's stream in yet separate non-blocking threads and -- if json-parsable -- creates a dict from them.
"""

import os
import sys
import subprocess
import queue
import threading
import time
import logging
import json
from typing import Union, Callable

# Initialise logger
log = logging.getLogger()


# -----------------------------------------------------------------------------

class WorkerManager:
    """"""

    def __init__(self, num_workers: Union[int, str], poll_freq: float=42, QueueCls=queue.Queue):
        """Initialize the worker manager."""
        # Initialize property-managed attributes
        self._num_workers = None
        self._workers = {}
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
            self.num_workers = os.cpu_count() - num_workers
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
        self._num_workers = val

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

    @task_count.setter
    def task_count(self, val: int) -> None:
        """Sets the task counter, only allowing increments by one."""
        if val - self._task_cnt != 1:
            raise ValueError("Can increment task counter only by one.")
        self._task_cnt = val

    @property
    def free_workers(self) -> bool:
        """Returns True if not `num_workers` amount of workers are busy at the moment."""
        return bool(len(self.working) < self.num_workers)


    # Public API ..............................................................

    def add_task(self, args: str, *, priority: int=None, read_stdout: bool=True, **kwargs):
        """Adds a task to the queue.

        A priority can be assigned to the task, but will only be used during task retrieval if the WorkerManager was initialized with a queue.PriorityQueue as task manager.

        Additionally, each task will be assigned an ID; this is used to preserve order in a priority queue if the priority of two or more tasks is the same.
        """
        # Generate a new ID for the task (using the current task counter)
        task_id = self.task_count

        log.debug("Adding task with ID %d ...", task_id)

        # Put it into the task queue and increment the task counter
        self._tasks.put_nowait((priority, task_id, dict(args=args, read_stdout=read_stdout, **kwargs)))
        self.task_count += 1

        log.debug("Task added.", task_id)

    def start_working(self):
        """Main execution routine of this class. When this method is called, all enqueued tasks will be worked on sequentially."""
        log.info("Starting to work ...")

        while not self.tasks.empty() or len(self.working) > 0:
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

        log.info("Finished working. Total tasks worked on: %d", self.task_count)


    # Non-public API ..........................................................

    def _grab_task(self, task=None) -> subprocess.Popen:
        """Will initiate that a new (or already existing) worker will work on a task. If a task is given, that one will be used; if not, a task will be taken from the queue.

        Returns the process the task is being worked on at. If no task was given and the queue is empty, will raise `queue.Empty`.
        """

        if not task:
            # Get one from the queue
            _, task_no, task = self._tasks.get_nowait()
            log.debug("Got task %s from queue.", task_no)

        # Interpret the given task
        # NOTE optionally, add an interpretation here, e.g. a mapping to an already existing worker

        # Spawn a worker and return the resulting process
        return self._spawn_worker(**task)

    def _spawn_worker(self, *, args: str, read_stdout: bool, line_read_func: Callable=None) -> subprocess.Popen:
        """Spawn a worker process using subprocess.Popen and manage the corresponding queue and thread for reading the stdout stream. The new worker process is registered with the class.

        Returns the created process object.
        """

        if read_stdout:
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
                                stdout=stdout, stderr=stderr)
        create_time = time.time() # approximate
        log.debug("Spawned worker process with PID %s.", proc.pid)
        # ... it is running now.
        
        # If enabled, prepare for reading the output
        if read_stdout:
            if line_read_func is None:
                raise ValueError("Need argument `line_read_func` for reading stdout.")

            # Generate the thread that reads the stream and populates the queue
            t = threading.Thread(target=line_read_func,
                                 kwargs=dict(queue=q, stream=proc.stdout))
            # Set to be a daemon thread => will die with the parent thread
            t.daemon = True

            # Start the thread; this will lead to enqueue_lines being called
            t.start()

            # Create a dictionary with stream information
            streams = dict(out=dict(queue=q, thread=t, log=[]))
            # NOTE could have more streams here, this function only focusses on stdout

            log.debug("Added thread to read worker's combined STDOUT and STDERR.")
        else:
            streams = None

        # Register the worker process with the class
        self._register_worker(proc=proc, args=args, streams=streams, create_time=create_time)
        
        return proc

    def _register_worker(self, *, proc, args, streams, create_time, **kwargs) -> None:
        """Registers a worker process with the class. This should be called soon after a process was created."""

        # Register in the working list
        self.working.append(proc)
        # NOTE if the process has already finished by this time, it will be removed from the working list at the next poll time

        # Gather information of this worker in the worker dict
        self.workers[proc] = dict(# worker information
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
        """Will poll all workers that are in the working list and remove them from that list if they are no longer alive."""
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

    def _worker_finished(self, proc):
        """Updates the entry of the worker dict after it has finished working."""
        self.workers[proc]['status'] = proc.returncode
        self.workers[proc]['end_time'] = time.time()

        # Read the remaining entries from the worker stream
        self._read_worker_stream(proc, max_num_reads=-1)

    def _read_worker_streams(self, stream_name: str='out'):
        """Gathers all working workers' streams with the given `stream_name`."""
        for proc in self.working:
            self._read_worker_stream(proc, stream_name=stream_name)

    def _read_worker_stream(self, proc, stream_name: str='out', log_to_stdout: bool=True, max_num_reads: int=1):
        """Gather a single entry from the given worker's stream queue and store the value in the stream log."""

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
                if log_to_stdout:
                    # print it to the parent processe's stdout
                    log.info("  process %s:  %s", proc.pid, entry)

                # Write to the stream's log
                stream['log'].append(entry)            
