"""The Task supplies a container for a task handled by the WorkerManager."""

import logging
from typing import Callable

# Initialise logger
log = logging.getLogger(__name__)


class Task:
    """The Task is a container for a task handled by the WorkerManager.
    
    It aims to provide the necessary interfaces for the WorkerManager to easily
    associate tasks with the corresponding workers and vice versa.
    
    Attributes:
        uid (int): the task ID, assumed to be a unique integer
        priority (int): The task priority
    """

    def __init__(self, *, uid: int, priority: int=None, setup_func: Callable=None, setup_kwargs: dict=None, worker_kwargs: dict=None):
        """Initialize a Task object."""
        # Create property-managed attributes
        self._uid = None

        # Carry over attributes
        self.uid = uid
        self._priority = priority

        self.setup_func = setup_func
        self.setup_kwargs = setup_kwargs
        self.worker_kwargs = worker_kwargs

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

    # Public API --------------------------------------------------------------

    # Non-Public API ----------------------------------------------------------


class TaskList(list):
    """This list is meant to store tasks in it.

    It disallows the use of some parent methods.
    """

    # Adapt some methods to make this a list of tasks -------------------------

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

    # Disallow any methods that change the existing content -------------------

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
