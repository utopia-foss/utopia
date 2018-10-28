"""Here, functions that are used in the StopCondition class are defined.

These all get passed the worker process, information and additional kwargs.

Required signature:  (task: WorkerTask, **kws) -> bool
"""

import logging
import time
import operator

# Initialise logger
log = logging.getLogger(__name__)

# Local variables
OPERATORS = {
    '<' : operator.lt, 'lt': operator.lt,
    '<=': operator.le, 'le': operator.le,
    '==': operator.eq, 'eq': operator.eq,
    '!=': operator.ne, 'ne': operator.ne,
    '>=': operator.ge, 'ge': operator.ge,
    '>' : operator.gt, 'gt': operator.gt,
    'xor': operator.xor,
    'contains': operator.contains
}

# -----------------------------------------------------------------------------

def timeout_wall(task, *, seconds: float) -> bool:
    """Checks the wall timeout of the given worker
    
    Args:
        task (WorkerTask): The WorkerTask object to check
        seconds (float): After how many seconds to trigger the wall timeout
    
    Returns:
        bool: Whether the timeout is fulfilled
    """
    return bool((time.time() - task.profiling['create_time']) > seconds)

def check_monitor_entry(task, *, entry_name: str, operator: str, value: float) -> bool:
    """Checks if a monitor entry compares in a certain way to a given value
    
    Args:
        task (WorkerTask): The WorkerTask object to check
        entry_name (str): The name of the monitor entry
        operator (str): The binary operator to use
        value (float): The value to compare to
    
    Returns:
        bool: Result of op(entry, value)
    """
    # See if there were and parsed objects
    if not task.outstream_objs:
        # Nope. Nothing to check yet.
        return False

    # Get the entry from the latest result
    entry = task.outstream_objs[-1][entry_name]

    # And perform the comparison
    return OPERATORS[operator](entry, value)
