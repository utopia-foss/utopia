"""Here, functions that are used in the StopCondition class are defined.

These all get passed the worker process, information and additional kwargs.

Required signature:  (task: WorkerTask, **kws)
"""

import logging
import time

from utopya.task import WorkerTask

# Initialise logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

def timeout_wall(task: WorkerTask, *, seconds: float) -> bool:
    """Checks the wall timeout of the given worker"""
    return bool((time.time() - task.profiling['create_time']) > seconds)
