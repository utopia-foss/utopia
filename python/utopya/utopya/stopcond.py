"""This module implements the StopCondition class, which is used by the
WorkerManager to stop a worker process in certain situations."""

import copy
import logging
import warnings
from typing import List, Callable
from subprocess import Popen

import yaml

import utopya.stopcond_funcs as sc_funcs

# Initialise logger
log = logging.getLogger(__name__)


class StopCondition:
    """A StopCondition object holds information on the conditions in which a worker process should be stopped.

    It is formulated in a general way, applying to all Workers. The attributes
    of this class store the information required to deduce whether the
    condition if fulfilled or not.
    """
    def __init__(self, *, to_check: List[dict]=None, name: str=None, description: str=None, enabled: bool=True, func: Callable=None, **func_kwargs):
        """Create a new stop condition
        
        Args:
            to_check (List[dict], optional): A list of dicts, that holds the
                functions to call and the arguments to call them with. The only
                requirement for the dict is that the `func` key is available.
                All other keys are unpacked and passed as kwargs to the given
                function.
            name (str, optional): The name of this stop condition
            description (str, optional): A short description of this stop
                condition
            enabled (bool, optional): Whether this stop condition should be
                checked; if False, it will be created but will always be un-
                fulfilled when checked.
            func (Callable, optional): (For the short syntax only!) If no
                `to_check` argument is given, a function can be given here that
                will be the only one that is checked.
            **func_kwargs: (For the short syntax) The kwargs that are passed
                to the single stop condition function
        """ 

        # Resolve functions that are to be checked
        self.to_check = self._resolve_sc_funcs(to_check, func, func_kwargs)

        # Carry over descriptive attributes
        self.enabled = enabled
        self.description = description
        self.name = name if name else " && ".join([e.get('func_name')
                                                   for e in to_check])

        log.debug("Initialized stop condition '%s' with %d checking "
                  "function(s).", self.name, len(self.to_check))

    @staticmethod
    def _resolve_sc_funcs(to_check: List[dict], func: Callable, func_kwargs: dict) -> List[tuple]:
        """Resolves the functions and kwargs that are to be checked."""

        if func and not to_check:
            # Not using the `to_check` argument
            log.debug("Got `func` directly and no `to_check` argument; will "
                      "use only this function for checking.")
            return [(func, func.__name__, func_kwargs)]

        elif to_check and (func or func_kwargs):
            warnings.warn("Got both `to_check` and `func` or `func_kwargs` "
                          "argument. `func` and `func_kwargs` will be "
                          "ignored!")

        # Everything ok, resolve the to_check list
        funcs_and_kws = []

        for func_dict in to_check:
            # Work on a copy (to be able to pop the func off)
            func_dict = copy.deepcopy(func_dict)

            # Pop the function off the dict and get its name
            func = func_dict.pop('func')

            # Check it
            if not callable(func):
                raise TypeError("Given value of key `func` needs to be a "
                                "callable, but was {} with value {}."
                                "".format(type(func), func))

            # else: is callable, has the __name__ attribute
            func_name = func.__name__
            log.debug("Got function '%s' for stop condition ...", func_name)

            # Valid. Append the information to the list
            funcs_and_kws.append((func, func_name, func_dict))
        
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
    """Constructor for creating a StopCondition object from a mapping"""
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

def sc_func_constructor(loader, node) -> Callable:
    """Constructor for creating a callable from a function name.

    The callables are used in a StopCondition object and are thus only searched
    for in the `stopcond_funcs` module.
    """
    log.debug("Encountered tag associated with stop condition function.")

    if isinstance(node, yaml.nodes.ScalarNode):
        log.debug("Constructing python string from scalar ...")
        sc_func_name = loader.construct_python_str(node)
        log.debug("  Resolved string:  %s", sc_func_name)
    else:
        raise TypeError("A stop condition function can only be constructed "
                        "from a scalar node, but got node of type {} "
                        "with value:\n{}".format(type(node), node))

    log.debug("Getting corresponding function from stopcond_funcs module ...")
    sc_func = sc_funcs.__dict__.get(sc_func_name)

    if not sc_func or not callable(sc_func):
        raise ImportError("Could not find a callable named '{}' in "
                          "stopcond_funcs module!".format(sc_func_name))

    return sc_func
