"""The WorkerManager class."""

import os
import queue
import time
import warnings
import logging
from typing import Union, Callable, Sequence, List, Set
from typing.io import BinaryIO

from utopya.task import WorkerTask, TaskList
from utopya.stopcond import StopCondition

# Initialise logger
log = logging.getLogger(__name__)


# -----------------------------------------------------------------------------

class WorkerManager:
    """The WorkerManager class manages WorkerTasks.
    
    Attributes:
        num_workers (int): The number of parallel workers
        poll_delay (float): The delay (in s) between after a poll
    """

    def __init__(self, num_workers: Union[int, str], poll_delay: float=0.05, QueueCls=queue.Queue):
        """Initialize the worker manager.
        
        Args:
            num_workers (Union[int, str]): The number of workers that can work
                in parallel. If `auto`, uses os.cpu_count(). If below zero,
                deduces abs(num_workers) from the CPU count.
            poll_delay (float, optional): How long (in seconds) the delay
                between worker polls should be. For too small delays (<0.01),
                the CPU load will become significant.
            QueueCls (Class, optional): Which class to use for the Queue.
                Defaults to FiFo.
        """
        # Initialize attributes, some of which are property-managed
        self._num_workers = None
        self._poll_delay = None
        self._tasks = TaskList()
        self._task_q = QueueCls()
        self._active_tasks = []

        # Hand over arguments
        self.poll_delay = poll_delay

        if num_workers == 'auto':
            self.num_workers = os.cpu_count()
        elif num_workers < 0:
            self.num_workers = os.cpu_count() + num_workers
        else:
            self.num_workers = num_workers

    # Properties ..............................................................
    @property
    def tasks(self) -> TaskList:
        """The list of all tasks."""
        return self._tasks

    @property
    def task_queue(self) -> queue.Queue:
        """The task queue."""
        return self._task_q

    @property
    def task_count(self) -> int:
        """Returns the number of tasks that this manager *ever* took care of. Careful: This is NOT the current number of tasks in the queue!"""
        return len(self.tasks)

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
    def active_tasks(self) -> List[WorkerTask]:
        """The list of currently active tasks.

        Note that this information might not be up-to-date; a process might quit just after the list has been updated.
        """
        return self._active_tasks

    @property
    def num_free_workers(self) -> int:
        """Returns the number of free workers."""
        return self.num_workers - len(self.active_tasks)

    @property
    def poll_delay(self) -> float:
        """The poll frequency in polls/second. Strictly speaking: the sleep time between two polls, which roughly equals the poll frequency."""
        return self._poll_delay

    @poll_delay.setter
    def poll_delay(self, val) -> None:
        """Set the poll frequency to a positive value."""
        if val <= 0.:
            raise ValueError("Poll delay needs to be positive, was "+str(val))
        elif val < 0.01:
            warnings.warn("Setting a poll delay of {} < 0.01s can lead to "
                          "significant CPU load. Consider choosing a higher "
                          "value.", UserWarning)
        self._poll_delay = val            

    # Public API ..............................................................

    def add_task(self, **task_kwargs):
        """Adds a task to the WorkerManager.
        
        Args:
            **task_kwargs: All arguments needed for WorkerTask initialization.
                See utopya.task.WorkerTask.__init__ for all valid arguments.
        """
        # Generate the WorkerTask object from the given parameters
        task = WorkerTask(**task_kwargs)

        # Append it to the task list and put it into the task queue
        self.tasks.append(task)
        self.task_queue.put_nowait(task)

        log.debug("Task %s (uid: %s) added.", task.name, task.uid)

    def start_working(self, *, detach: bool=False, forward_streams: bool=False, timeout: float=None, stop_conditions: Sequence[StopCondition]=None, post_poll_func: Callable=None) -> None:
        """Upon call, all enqueued tasks will be worked on sequentially.
        
        Args:
            detach (bool, optional): If False (default), the WorkerManager
                will block here, as it continuously polls the workers and
                distributes tasks.
            forward_streams (bool, optional): If True, workers' streams are
                forwarded to stdout via the log module.
            timeout (float, optional): If given, the number of seconds this
                work session is allowed to take. Workers will be aborted if
                the number is exceeded. Note that this is not measured in CPU time, but the host systems wall time.
            stop_conditions (Sequence[StopCondition], optional): During the
                run these StopCondition objects will be checked
            post_poll_func (Callable, optional): If given, this is called after
                all workers have been polled. It can be used to perform custom
                actions during a the polling loop.
        
        Raises:
            NotImplementedError: for `detach` True
            ValueError: For invalid (i.e., negative) timeout value
            WorkerManagerTotalTimeout: Upon a total timeout
        """
        # Determine timeout arguments 
        timeout_time = None

        if timeout:
            if timeout <= 0:
                raise ValueError("Invalid value for argument `timeout`: {} -- "
                                 "needs to be positive.".format(timeout))
            # Already calculate the time after which a timeout would be reached
            timeout_time = time.time() + timeout
            log.debug("Set timeout time to now + %f seconds", timeout) 

        # Determine whether to detach the whole working loop
        if detach:
            # TODO implement the content of this in a separate thread.
            raise NotImplementedError("It is currently not possible to "
                                      "detach the WorkerManager from the "
                                      "main thread.")
        
        # Count the polls
        poll_no = 0

        log.info("Starting to work ...")
        log.debug("  Timeout:          now + %ss", timeout)
        log.debug("  Stop conditions:  %s", stop_conditions)

        # Enter the polling loop, where most of the time will be spent
        
        # Start with the polling loop
        # Basically all working time will be spent in there ...
        try:
            while self.active_tasks or self.task_queue.qsize() > 0:
                # Check total timeout
                if timeout_time is not None and time.time() > timeout_time:
                    raise WorkerManagerTotalTimeout()

                # Check if there are free workers and remaining tasks.
                if self.num_free_workers and self.task_queue.qsize():
                    # Yes. => Grab a task and start working on it
                    # Aproach with adding tasks as number of free workers, limited by available tasks in queue
                    for _ in range(min(self.num_free_workers, self.task_queue.qsize())):
                        new_task = self._grab_task()
                        self.active_tasks.append(new_task)

                # Gather the streams of all working workers
                for task in self.active_tasks:
                    task.read_streams(forward_streams=forward_streams)

                # Check stop conditions
                if stop_conditions is not None:
                    # Compile a list of workers that need to be terminated
                    to_terminate = self._check_stop_conds(stop_conditions)
                    self._signal_workers(to_terminate, signal='SIGTERM')

                # Poll the workers
                self._poll_workers()
                # NOTE this will also remove no longer active workers

                # Call the post-poll function
                if post_poll_func is not None:
                    log.debug("Calling post_poll_func %s ...",
                              post_poll_func.__name__)
                    post_poll_func()

                # Some information
                poll_no += 1
                log.debug("Poll # %6d:  %d active tasks",
                          poll_no, len(self.active_tasks))

                # Delay the next poll
                time.sleep(self.poll_delay)

        except WorkerManagerError as err:
            log.warning("Did not finish working due to a %s ...",
                        err.__class__.__name__)
            
            log.warning("Terminating active tasks ...")
            self._signal_workers(self.active_tasks, signal='SIGTERM')
            raise

        else:
            # Finished because no working workers and no more tasks remained
            log.info("Finished working. Total tasks worked on: %d",
                     self.task_count)

    # Non-public API ..........................................................

    def _grab_task(self) -> WorkerTask:
        """Will initiate that a task is gotten from the queue and that it
        spawns its worker process.
        
        Returns:
            WorkerTask: The WorkerTask grabbed from the queue.
        
        Raises:
            queue.Empty: If the task queue was empty
        """

        # Get a task from the queue
        try:
            task = self.task_queue.get_nowait()

        except queue.Empty as err:
            raise queue.Empty("No more tasks available in tasks queue.") from err
        
        else:
            log.debug("Got task %s from queue. (Priority: %s)",
                      task.uid, task.priority)

        # Let it spawn its own worker
        task.spawn_worker()

        # Now return the task
        return task
    
    def _poll_workers(self) -> None:
        """Will poll all workers that are in the working list and remove them from that list if they are no longer alive.
        """
        # Poll the task's worker's status
        for task in self.active_tasks:
            if task.worker_status is not None:
                # This task has finished. Need to rebuild the list
                break
        else:
            # Nothing to rebuild
            return

        # Broke out of the loop, i.e.: at least ne task finished
        # have to rebuild the list of active tasks now...
        self.active_tasks[:] = [t for t in self.active_tasks
                                if t.worker_status is None]
        # NOTE this will also poll all other active tasks and potentially not add them to the active_tasks list again.

        return

    def _check_stop_conds(self, stop_conds: Sequence[StopCondition]) -> Set[WorkerTask]:
        """Checks the given stop conditions for the active tasks and compiles a list of workers that need to be terminated.
        
        Args:
            stop_conds (Sequence[StopCondition]): The stop conditions that
                are to be checked.
        
        Returns:
            List[WorkerTask]: The WorkerTasks whose workers need to be
                terminated
        """
        to_terminate = []
        log.debug("Checking %d stop condition(s) ...", len(stop_conds))

        for sc in stop_conds:
            log.debug("Checking stop condition '%s' ...", sc.name)
            fulfilled = [t for t in self.active_tasks if sc.fulfilled(t)]

            if fulfilled:
                log.debug("Stop condition '%s' was fulfilled for %d task(s) "
                          "with name(s):  %s", sc.name, len(fulfilled),
                          ", ".join([t.name for t in fulfilled]))
                to_terminate += fulfilled

        # Return as set to be sure that they are unique
        return set(to_terminate)

    def _signal_workers(self, tasks: Union[str, List[WorkerTask]], *, signal: Union[str, int]) -> None:
        """Send signals to a list of WorkerTasks.
        
        Args:
            tasks (Union[str, List[WorkerTask]]): strings 'all' or 'active' or
                a list of WorkerTasks to signal
            signal (Union[str, int]): The signal to send
        """
        if isinstance(tasks, str):
            if tasks == 'all':
                tasks = self.tasks
            elif tasks == 'active':
                tasks = self.active_tasks
            else:
                raise ValueError("Tasks cannot be specified by string '{}', "
                                 "allowed strings are: 'all', 'active'."
                                 "".format(tasks))

        if not tasks:
            log.debug("No worker tasks to signal.")
            return 

        log.debug("Sending signal %s to %d task(s) ...", signal, len(tasks))
        for task in tasks:
            task.signal_worker(signal)
        
        log.debug("All tasks signalled. Tasks' worker status:\n  %s",
                  ", ".join([str(t.worker_status) for t in tasks]))

# Custom exceptions -----------------------------------------------------------


class WorkerManagerError(BaseException):
    """The base exception class for WorkerManager errors"""
    pass


class WorkerManagerTotalTimeout(WorkerManagerError):
    """Raised when a total timeout occured"""
    pass
