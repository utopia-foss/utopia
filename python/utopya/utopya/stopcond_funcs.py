"""Here, functions that are used in the StopCondition class are defined.

These all get passed the worker task, information and additional kwargs.

Required signature:  ``(task: WorkerTask, **kws) -> bool``
"""

import logging
import time
import operator

from dantro.utils.data_ops import BOOLEAN_OPERATORS as OPERATORS
from paramspace.tools import recursive_getitem as _recursive_getitem

# Initialise logger
log = logging.getLogger(__name__)

# To not repeatedly show warnings, keep track of failed checks
_FAILED_MONITOR_ENTRY_CHECKS = []


# -----------------------------------------------------------------------------

def timeout_wall(task: 'utopya.task.WorkerTask', *, seconds: float) -> bool:
    """Checks the wall timeout of the given worker

    Args:
        task (utopya.task.WorkerTask): The WorkerTask object to check
        seconds (float): After how many seconds to trigger the wall timeout

    Returns:
        bool: Whether the timeout is fulfilled
    """
    return bool((time.time() - task.profiling['create_time']) > seconds)


def check_monitor_entry(task: 'utopya.task.WorkerTask', *, entry_name: str,
                        operator: str, value: float) -> bool:
    """Checks if a monitor entry compares in a certain way to a given value

    Args:
        task (utopya.task.WorkerTask): The WorkerTask object to check
        entry_name (str): The name of the monitor entry, leading to the value
            to the left-hand side of the operator
        operator (str): The binary operator to use
        value (float): The right-hand side value to compare to

    Returns:
        bool: Result of op(entry, value)
    """
    # See if there were and parsed objects
    if not task.outstream_objs:
        # Nope. Nothing to check yet.
        return False

    # Try to recursively retrieve the entry from the latest monitoring output
    latest_monitor = task.outstream_objs[-1]
    try:
        entry = _recursive_getitem(latest_monitor, keys=entry_name.split("."))

    except KeyError:
        # Only warn once
        if entry_name not in _FAILED_MONITOR_ENTRY_CHECKS:
            log.caution(
                "Failed evaluating stop condition due to missing entry '%s' "
                "in monitor output!\nAvailable monitor data: %s",
                entry_name, latest_monitor,
            )
            _FAILED_MONITOR_ENTRY_CHECKS.append(entry_name)
        return False

    # And perform the comparison
    return OPERATORS[operator](entry, value)
