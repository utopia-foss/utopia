"""Implementation of the Task class.

The Task supplies a container for a Task handled by the WorkerManager.
"""

class Task:
    def __init__(self, task_id: int, priority: int=0):
        self.id=task_id
        self._priority=priority #(possible between -int and + int)
        self.setup_func = None
        self.setup_kwargs = None
        
    def __repr__(self):
        return "ID: {}, Pri: {}".format(self.id, self._priority)
    
    #compariosn operator for priority
    def __lt__(self, other):
        return self._priority < other._priority
