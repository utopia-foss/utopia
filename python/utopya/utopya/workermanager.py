"""The WorkerManager class."""

import os
import queue
import time
import warnings
import logging
from typing import Union, List

from utopya.task import WorkerTask, TaskList

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
        
        A priority can be assigned to the task, but will only be used during task retrieval if the WorkerManager was initialized with a queue.PriorityQueue as task manager.
        
        Additionally, each task will be assigned an ID; this is used to preserve order in a priority queue if the priority of two or more tasks is the same.
        
        Args:
            **task_kwargs: All arguments needed for Task initialization. See
                utopya.task.Task.__init__ for all valid arguments.
        """
        # Generate the task object, creating a unique ID by using the current task count
        task = WorkerTask(uid=self.task_count, **task_kwargs)

        # Append it to the task list and put it into the task queue
        self.tasks.append(task)
        self.task_queue.put_nowait(task)

        log.debug("Task %s added.", task.uid)

    def start_working(self, detach: bool=False, forward_streams: bool=False):
        """Upon call, all enqueued tasks will be worked on sequentially.
        
        Args:
            detach (bool, optional): If False (default), the WorkerManager
                will block here, as it continuously polls the workers and
                distributes tasks.
            forward_streams (bool, optional): If True, the streams are
                forwarded to the main thread's stdout
        
        Raises:
            NotImplementedError: for `detach` True
        """
        log.info("Starting to work ...")

        if detach:
            # TODO implement the content of this in a separate thread.
            raise NotImplementedError("It is currently not possible to "
                                      "detach the WorkerManager from the "
                                      "main thread.")
        
        # Count the polls
        poll_no = 0

        # Enter the polling loop, where most of the time will be spent
        while self.active_tasks or self.task_queue.qsize() > 0:
            # Check if there are free workers and remaining tasks.
            if self.num_free_workers and self.task_queue.qsize():
                # Yes. => Grab a task and start working on it
                # Conservative approach: one task is grabbed here, even if there are more than one free workers
                new_task = self._grab_task()
                self.active_tasks.append(new_task)

            # Gather the streams of all working workers
            for task in self.active_tasks:
                task.read_streams(forward_streams=forward_streams)

            # Poll the workers and update the active_tasks list
            self._poll_workers()
            
            # Some information
            poll_no += 1
            log.debug("Poll # %6d:  %d active tasks",
                      poll_no, len(self.active_tasks))

            # Delay the next poll
            time.sleep(self.poll_delay)

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
        
        Returns:
            None
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
