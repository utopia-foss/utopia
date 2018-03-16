"""Here, functions that are used in the StopCondition class are defined.

These all get passed the worker process, information and additional kwargs.

Required signature:  (*, worker_info, proc, **kws)

If `proc` or `worker_info` is not needed, this can be indicated via default arguments.
"""

import logging
import time
from subprocess import Popen

# Initialise logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

def timeout_wall(*, worker_info: dict, seconds: float, proc: Popen=None) -> bool:
    """Checks the wall timeout of the given worker"""
    return bool(time.time() - worker_info['create_time'] > seconds)
