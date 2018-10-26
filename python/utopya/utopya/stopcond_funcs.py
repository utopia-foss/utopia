"""Here, functions that are used in the StopCondition class are defined.

These all get passed the worker process, information and additional kwargs.

Required signature:  (task: WorkerTask, **kws)
"""

import logging
import time

# Initialise logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

def timeout_wall(task, *, seconds: float) -> bool:
    """Checks the wall timeout of the given worker
    
    Args:
        task (WorkerTask): The WorkerTask object to use as base for the check
        seconds (float): After how many seconds to trigger the wall timeout
    
    Returns:
        Whether the timeout condition is fulfilled
    """
    return bool((time.time() - task.profiling['create_time']) > seconds)
