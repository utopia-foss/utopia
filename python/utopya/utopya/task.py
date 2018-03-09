"""Implementation of the Task class.

The Task supplies a container for a task handled by the WorkerManager.
"""

class Task:
    """The Task is a container for a task handled by the WorkerManager.
    
    It aims to provide necessary interfaces
    
    Attributes:
        id (int): Task_ID (positive, read only)
        priority (int): The model name associated with this Multiverse
    """
    def __init__(self, task_id: int, priority: int=0):
        self.id = task_id
        self._priority = priority  # (possible between -int and + int)
        self.setup_func = None
        self.setup_kwargs = None
        
    def __repr__(self):
        return "ID: {}, Pri: {}".format(self.id, self._priority)
    
    # compariosn operator for priority
    def __lt__(self, other):
        return self._priority < other._priority

    # Properties --------------------------------------------------------------
    @property
    def id(self):
        return self._id
    @id.setter
    def id(self, task_id: int):
        """Checks if the model name is valid, then sets it and makes it read-only."""
        if task_id < 0:
            raise ValueError("Negative task Id not allowed, chosen ID: {}".format(task_id))
        elif self.id:
            raise RuntimeError("A Task's ID cannot be changed!")
        else:
            self._id = task_id
            log.debug("Set Task ID:  %s", task_id)

    # Public API --------------------------------------------------------------

    # Non-Public API ----------------------------------------------------------