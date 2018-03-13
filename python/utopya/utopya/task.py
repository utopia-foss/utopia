"""The Task class supplies a container for all information needed for a task.

The WorkerTask specialises on tasks for the WorkerManager."""

import warnings
import logging
from typing import Callable

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
        return "Task<uid: {}, priority: {}>".format(self.uid, self._priority)

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


class WorkerTask(Task):
    """A specialisation of the Task class that is aimed at use in the WorkerManager."""

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

        log.debug("Specialized Task to be a WorkerTask.")
        log.debug("  With setup function?  %s", bool(setup_func))

    def __str__(self) -> str:
        return "WorkerTask<uid: {}, priority: {}>".format(self.uid, self._priority)


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
