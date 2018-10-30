"""This module implements the StopCondition class, which is used by the
WorkerManager to stop a worker process in certain situations."""

import copy
import logging
import warnings
from typing import List, Callable, Union

import ruamel.yaml

import utopya.stopcond_funcs as sc_funcs

# Initialise logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

class StopCondition:
    """A StopCondition object holds information on the conditions in which a
    worker process should be stopped.

    It is formulated in a general way, applying to all Workers. The attributes
    of this class store the information required to deduce whether the
    condition if fulfilled or not.
    """

    def __init__(self, *, to_check: List[dict]=None, name: str=None, description: str=None, enabled: bool=True, func: Union[Callable, str]=None, **func_kwargs):
        """Create a new stop condition
        
        Args:
            to_check (List[dict], optional): A list of dicts, that holds the
                functions to call and the arguments to call them with. The only
                requirement for the dict is that the `func` key is available.
                All other keys are unpacked and passed as kwargs to the given
                function. The `func` key can be either a callable or a string
                corresponding to a name in the utopya.stopcond_funcs module.
            name (str, optional): The name of this stop condition
            description (str, optional): A short description of this stop
                condition
            enabled (bool, optional): Whether this stop condition should be
                checked; if False, it will be created but will always be un-
                fulfilled when checked.
            func (Union[Callable, str], optional): (For the short syntax
                only!) If no `to_check` argument is given, a function can be
                given here that will be the only one that is checked. If this
                argument is a string, it is also resolved from the utopya
                stopcond_funcs module.
            **func_kwargs: (For the short syntax) The kwargs that are passed
                to the single stop condition function
        """

        # Resolve functions that are to be checked
        self.to_check = self._resolve_sc_funcs(to_check, func, func_kwargs)

        # Carry over descriptive attributes
        self.enabled = enabled
        self.description = description
        self.name = name if name else " && ".join([fspec[1]
                                                   for fspec in self.to_check])

        # Store the initialization kwargs such that they can be used for yaml
        # representation
        self._init_kwargs = dict(to_check=to_check, name=name,
                                 description=description, enabled=enabled,
                                 func=func, **func_kwargs)

        log.debug("Initialized stop condition '%s' with %d checking "
                  "function(s).", self.name, len(self.to_check))

    @staticmethod
    def _resolve_sc_funcs(to_check: List[dict], func: Callable, func_kwargs: dict) -> List[tuple]:
        """Resolves the functions and kwargs that are to be checked."""

        def retrieve_func(func_name: str) -> Callable:
            """Given a function name, returns the callable from the utopya
            module stopcond_funcs.
            """                
            log.debug("Getting function with name '%s' from the "
                      "stopcond_funcs module ...", func_name)
            func = sc_funcs.__dict__.get(func_name)

            if not func or not callable(func):
                raise ImportError("Could not find a callable named '{}' "
                                  "in stopcond_funcs module!"
                                  "".format(func_name))

            return func

        # Check argument combinations
        if func and not to_check:
            # Not using the `to_check` argument
            log.debug("Got `func` directly and no `to_check` argument; will "
                      "use only this function for checking.")
            func = retrieve_func(func)
            return [(func, func.__name__, func_kwargs)]

        elif to_check and (func or func_kwargs):
            raise ValueError("Got arguments `to_check` and (one or more of) "
                             "`func` or `func_kwargs`! Please pass either the "
                             "`to_check` (list of dicts) or a single `func` "
                             "with a dict of `func_kwargs`.")

        elif to_check is None and func is None:
            raise TypeError("Need at least one of the required "
                            "keyword-arguments `to_check` or `func`!")

        # Everything ok
        # Resolve the to_check list
        funcs_and_kws = []

        for func_dict in to_check:
            # Work on a copy (to be able to pop the func off)
            func_dict = copy.deepcopy(func_dict)

            # Pop the function off the dict and get its name
            func = func_dict.pop('func')

            # This might not actually be a function yet. Resolve it...
            if isinstance(func, str):
                # Is a function name, resolve to a function
                func = retrieve_func(func)

            elif not callable(func):
                raise TypeError("Given `func` needs to be a callable, but was "
                                "{} with value {}."
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

    def fulfilled(self, task) -> bool:
        """Checks if the stop condition is fulfilled for the given worker,
        using the information from the dict.
        
        All given stop condition functions are evaluated; if all of them return
        True, this method will also return True.
        
        Args:
            task (Task): Task object that is to be checked
            worker_info (dict): The information dict for this specific worker
        
        Returns:
            bool: If all stop condition functions returned true for the given
                worker and its current information
        """
        if not self.enabled or not self.to_check:
            # Never fulfilled
            return False

        # Now perform the check on this task with all stop condition functions
        for sc_func, name, kws in self.to_check:
            if not sc_func(task, **kws):
                # One was not True -> not fulfilled, need not check the others
                log.debug("%s not fulfilled! Returning False ...", name)
                return False

        # All were True -> fulfilled
        return True

    # YAML Constructor & Representer ..........................................
    yaml_tag = u'!stop-condition'

    @classmethod
    def to_yaml(cls, representer, node):
        """Creates a yaml representation of the StopCondition object by storing
        the initialization kwargs as a yaml mapping.

        Args:
            representer (ruamel.yaml.representer): The representer module
            node (type(self)): The node, i.e. an instance of this class
        
        Returns:
            a yaml mapping that is able to recreate this object
        """
        # Filter out certain entries that are None
        d = copy.deepcopy(node._init_kwargs)
        d = {k:v for k, v in d.items()
             if not  (k in ['name', 'description', 'func', 'to_check']
                      and v is None)
             and not (k in ['enabled'] and v is True)}

        # Create the mapping representation from the filtered dict
        return representer.represent_mapping(cls.yaml_tag, d)

    @classmethod
    def from_yaml(cls, constructor, node):
        """Creates a StopCondition object by unpacking the given mapping such
        that all stored arguments are available to __init__.
        """
        return cls(**constructor.construct_mapping(node, deep=True))
