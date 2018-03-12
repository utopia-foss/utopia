"""This module implements the StopCondition class, which is used by the
WorkerManager to stop a worker process in certain situations."""

import copy
import time
import yaml
import logging
from typing import List
from subprocess import Popen

# Initialise logger
log = logging.getLogger(__name__)


class StopCondition:
    """A StopCondition object holds information on the conditions in which a worker process should be stopped.

    It is formulated in a general way, applying to all Workers. The attributes
    of this class store the information required to deduce whether the
    condition if fulfilled or not.
    """
    def __init__(self, *, to_check: List[dict], name: str=None, description: str=None, enabled: bool=True):
        """Create a new stop condition"""

        # Resolve functions that are to be checked
        self.to_check = self._resolve_sc_funcs(to_check)

        # Carry over descriptive attributes
        self.enabled = enabled
        self.description = description
        self.name = name if name else " && ".join([e.get('func_name')
                                                   for e in to_check])

        log.debug("Initialized stop condition '%s' with %d checking "
                  "function(s).", self.name, len(self.to_check))

    @staticmethod
    def _resolve_sc_funcs(to_check: List[dict]) -> List[tuple]:
        """Resolves the functions and kwargs"""
        funcs_and_kws = []

        for func_dict in to_check:
            # Work on a copy
            func_dict = copy.deepcopy(func_dict)

            # Pop the name and resolve the function
            func_name = func_dict.pop('func_name')
            _func_name = '_sc_' + func_name

            log.debug("Resolving function '%s' from global scope ...",
                      _func_name)
            sc_func = globals().get(_func_name)

            # Check it
            if not (sc_func and callable(sc_func)):
                raise ValueError("Could not find function '{}' in local "
                                 "scope!".format(_func_name))

            # Valid. Append the information to the list
            funcs_and_kws.append((sc_func, func_name, func_dict))
        
        log.debug("Resolved %d stop condition function(s).",
                  len(funcs_and_kws))
        return funcs_and_kws

    def __str__(self) -> str:
        return ("StopCondition '{name:}':\n"
                "  {desc:}\n".format(name=self.name, desc=self.description))

    def fulfilled(self, proc: Popen, *, worker_info: dict) -> bool:
        """Checks if the stop condition is fulfilled for the given worker, using the information from the dict.
        
        All given stop condition functions are evaluated; if all of them return
        True, this method will also return True.
        
        Args:
            proc (Popen): Worker object that is to be checked
            worker_info (dict): The information dict for this specific worker
        
        Returns:
            bool: If all stop condition functions returned true for the given
                worker and its current information
        """
        if not self.enabled or not self.to_check:
            # Never fulfilled
            return False

        # Now perform the check on all stop condition functions
        for sc_func, name, kws in self.to_check:
            if not sc_func(worker_info=worker_info, proc=proc, **kws):
                # One was not True -> not fulfilled, need not check the others
                log.debug("%s not fulfilled! Returning False ...", name)
                return False

        # All were True -> fulfilled
        return True


# -----------------------------------------------------------------------------

def stop_cond_constructor(loader, node) -> StopCondition:
    """constructor for creating a StopCondition object from a mapping"""
    log.debug("Encountered tag associated with StopCondition.")

    if isinstance(node, yaml.nodes.MappingNode):
        log.debug("Constructing mapping ...")
        mapping = loader.construct_mapping(node, deep=True)
        stop_cond = StopCondition(**mapping)
    else:
        raise TypeError("StopCondition can only be constructed from a mapping "
                        "node, got node of type {} "
                        "with value:\n{}".format(type(node), node))

    return stop_cond

# -----------------------------------------------------------------------------
# Stop condition functions
# These all get passed the worker process, information and additional kwargs
# Required signature:  (*, worker_info, proc, **kws)
# If `proc` or `worker_info` is not needed, this can be indicated via default arguments

def _sc_timeout_wall(*, worker_info: dict, seconds: float, proc: Popen=None) -> bool:
    """Checks the wall timeout of the given worker"""
    return bool(time.time() - worker_info['create_time'] > seconds)
