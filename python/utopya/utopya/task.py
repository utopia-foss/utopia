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
            raise TypeError("Need integer task ID, got " + str(type(uid)))
        
        elif uid < 0:
            raise ValueError("Negative task ID not allowed: " + str(uid))
        
        elif self.uid is not None:
            raise RuntimeError("Task ID was already set and can't be changed!")

        else:
            self._uid = uid
            log.debug("Set task ID:  %d", self.uid)

    @property
    def priority(self) -> float:
        """The task priority, usually a """
        return self._priority

    @property
    def order_tuple(self) -> tuple:
        """Returns the ordering tuple (priority, task ID)"""
        return (self.priority, self.uid)

    # Magic methods -----------------------------------------------------------
    
    def __str__(self):
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
