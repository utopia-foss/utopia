"""For functions that are not bound to classes, but generally useful."""

import os
import sys
import operator
import io
import re
import logging
import collections
import subprocess
from datetime import timedelta
from typing import Union, Tuple

import numpy as np

from paramspace import yaml
import paramspace.yaml_constructors as pspyc

import utopya.stopcond
from utopya.info import MODELS

# Local constants
log = logging.getLogger(__name__)

# terminal / TTY-related
# Get information on the size of the terminal. This is already executed at
# import time of this module.
IS_A_TTY = sys.stdout.isatty()
try:
    _, TTY_COLS = subprocess.check_output(['stty', 'size']).split()
except:
    # Probably not run from terminal --> set value manually
    TTY_COLS = 79
else:
    TTY_COLS = int(TTY_COLS)
log.debug("Determined TTY_COLS: %s,  IS_A_TTY: %s", TTY_COLS, IS_A_TTY)


# -----------------------------------------------------------------------------
# yaml constructors -----------------------------------------------------------

def _expr_constructor(loader, node):
    """Custom pyyaml constructor for evaluating strings with simple mathematical expressions.

    Supports: +, -, *, **, /, e-X, eX
    """
    # get expression string
    expr_str = loader.construct_scalar(node)

    # Remove spaces
    expr_str = expr_str.replace(" ", "")

    # Parse some special strings
    if expr_str in ['nan', 'NaN']:
        return float("nan")

    # NOTE these will cause errors if emitted file is not read by python!
    elif expr_str in ['np.inf', 'inf', 'INF']:
        return np.inf

    elif expr_str in ['-np.inf', '-inf', '-INF']:
        return -np.inf

    # remove everything that might cause trouble -- only allow digits, dot, +,
    # -, *, /, and eE to allow for writing exponentials
    expr_str = re.sub(r'[^0-9eE\-.+\*\/]', '', expr_str)

    # Try to eval
    return eval(expr_str)

def _model_cfg_constructor(loader, node) -> dict:
    """Custom yaml constructor for loading a model configuration file.

    This extracts the `model_name` key, loads the corresponding model config
    and then recursively updates the loaded config with the remaining keys
    from that part of the configuration.
    """
    # Get a mapping from the node
    d = loader.construct_mapping(node, deep=True)
    # NOTE using the deep flag here to allow nested calls to this constructor

    # Extract the model name
    model_name = d.pop('model_name')

    # Load the corresponding model configuration
    mcfg, _ = load_model_cfg(model_name)

    # Update the loaded config with the remaining keys
    mcfg = recursive_update(mcfg, d)

    # Return the updated dictionary
    return mcfg

# .............................................................................
# Attaching representers and constructors .....................................
# First register the classes, which directly implemented dumping/loading
yaml.register_class(utopya.stopcond.StopCondition)

# Now, add (additional, potentially overwriting) constructors for certain tags
# For the expression and model config objects.
yaml.constructor.add_constructor(u'!expr',
                                 _expr_constructor)
yaml.constructor.add_constructor(u'!model',
                                 _model_cfg_constructor)

# Add aliases for the (coupled) parameter dimensions
yaml.constructor.add_constructor(u'!sweep',
                                 pspyc.pdim)
yaml.constructor.add_constructor(u'!sweep-default',
                                 pspyc.pdim_default)

yaml.constructor.add_constructor(u'!coupled-sweep',
                                 pspyc.coupled_pdim)
yaml.constructor.add_constructor(u'!coupled-sweep-default',
                                 pspyc.coupled_pdim_default)

# Set the flow style
yaml.default_flow_style = False


# input/output ----------------------------------------------------------------

def read_yml(path: str, *, error_msg: str=None) -> dict:
    """Read a yaml file and return the resulting dict.
    
    Args:
        path (str): path to yml file that is to be read
        error_msg (str, optional): if given, this is used as error message
    
    Returns:
        dict: with contents of yml file
    
    Raises:
        FileNotFoundError: If file was not found at `path`
    """
    # Ensure the given path is a string (and not a path-like object)
    path = str(path)

    # Try opening and loading
    try:
        with open(path, 'r') as ymlfile:
            d = yaml.load(ymlfile)

    except FileNotFoundError as err:
        if error_msg:  # is None by default
            raise FileNotFoundError(error_msg) from err
        raise err

    # Everything ok, return the dict
    return d

def write_yml(d: dict, *, path: str) -> None:
    """Write dict to yml file in path.
    
    Writes a given dictionary into a yaml file. Error is raised if file already exists.
    
    Args:
        d (dict): dict to be written
        path (str): target path. This should include the extension `*.yml`
    
    Raises:
        FileExistsError: If the file already exists.
    """
    # Ensure the given path is a string (and not a path-like object)
    path = str(path)
    
    # check whether file already exists
    if os.path.exists(path):
        raise FileExistsError("Target file {0} already exists.".format(path))
    
    # else: dump the dict into the config file
    with open(path, 'w') as ymlout:
        yaml.dump(d, ymlout)

def load_model_cfg(model_name: str) -> Tuple[dict, str]:
    """Loads the model configuration for the given model name using the info
    from the info module.
    
    The model configuration is a file named <model_name>_cfg.yml and resides
    beside the source files.
    
    Args:
        model_name (str): The name of the model to load
    
    Returns:
        Tuple[dict, str]: The corresponding model configuration and the path
            to the model configuration file
    
    Raises:
        ValueError: model name not registered with info module
    """
    log.debug("Loading model configuration for '%s' model ...", model_name)

    # Check the model name
    if model_name not in MODELS:
        raise ValueError("No '{}' model available! Model names registered "
                         "with the info module are: {}"
                         "".format(model_name, [k for k in MODELS.keys()]))

    # Get the path to the model configuration
    path = MODELS[model_name]['model_cfg']

    # Read in the model configuration
    mcfg = read_yml(path, error_msg=("Could not locate model configuration "
                                     "for '{}' model! Expected to find it at: "
                                     "{}".format(model_name, path)))

    # And return it
    return mcfg, path


# working on dicts ------------------------------------------------------------

def recursive_update(d: dict, u: dict) -> dict:
    """Update dict `d` with values from dict `u`.
    
    NOTE: This method does _not_ copy mutable entries! If you want to assure
    that the contents of `u` cannot be changed by changing its counterparts in
    the updated `d`, you need to supply a deep copy of `u` to this method.
    
    Args:
        d (dict): The dict to be updated
        u (dict): The dict used to update
    
    Returns:
        dict: updated version of d
    """
    # Need to check whether `d` is a mapping at all
    if not isinstance(d, collections.Mapping):
        # It is not. Need not go any further and can just return `u`
        return u

    # d can now assumed to be a mapping
    # Iterate over the keys of the update dict and (recursively) add them
    for key, val in u.items():
        if isinstance(val, collections.Mapping):
            # Already a Mapping, continue recursion
            d[key] = recursive_update(d.get(key, {}), val)
        else:
            # Not a mapping -> at leaf -> update value
            d[key] = val    # ... which is just u[key]

    # Finished at this level; return updated dict
    return d


# string formatting -----------------------------------------------------------

def format_time(duration: Union[float, timedelta], *, ms_precision: int=0) -> str:
    """Given a duration (in seconds), formats it into a string.
    
    The formatting divisors are: days, hours, minutes, seconds
    
    If `ms_precision` > 0 and `duration` < 60, decimal places will be shown
    for the seconds.
    
    Args:
        duration (Union[float, timedelta]): The duration in seconds to format
            into a duration string; it can also be a timedelta object.
        ms_precision (int, optional): The precision of the seconds slot if only
            seconds will be shown.
    
    Returns:
        str: The formatted duration string    
    """

    if isinstance(duration, timedelta):
        duration = duration.total_seconds()

    divisors = (24*60*60, 60*60, 60, 1)
    letters = ("d", "h", "m", "s")
    remaining = float(duration)
    parts = []

    # Also handle negative numbers
    is_negative = bool(duration < 0)
    if is_negative:
        # Calculate with the positive value
        remaining *= -1

    # Go over divisors and letters and see if there is something to represent
    for divisor, letter in zip(divisors, letters):
        time_to_represent = int(remaining/divisor)
        remaining -= time_to_represent * divisor

        if time_to_represent > 0:
            # Distinguish between seconds and other divisors for short times
            if ms_precision <= 0 or duration > 60:
                # Regular behaviour: Seconds do not have decimals
                s = "{:d}{:}".format(time_to_represent, letter)

            elif ms_precision > 0 and letter == "s":
                # Decimal places for seconds
                s = "{val:.{prec:d}f}s".format(val=(time_to_represent
                                                    + remaining),
                                               prec=int(ms_precision))

            parts.append(s)

    # If nothing was added so far, the time was below one second
    if not parts:
        if ms_precision == 0:
            # Just show an approximation
            if not is_negative:
                s = "< 1s"
            else:
                s = "> -1s"

        else:
            # Show with ms_precision decimal places
            s = '{val:{tot}.{prec}f}s'.format(val=remaining,
                                              tot=int(ms_precision) + 2,
                                              prec=int(ms_precision))
            if is_negative:
                s = "-" + s

        parts.append(s)

    if is_negative:
        # Prepend a minus
        parts = ["-"] + parts

    return " ".join(parts)

def fill_line(s: str, *, num_cols: int=TTY_COLS, fill_char: str=" ", align: str="left") -> str:
    """Extends the given string such that it fills a whole line of `num_cols` columns.
    
    Args:
        s (str): The string to extend to a whole line
        num_cols (int, optional): The number of colums of the line; defaults to
            the number of TTY columns or – if those are not available – 79
        fill_char (str, optional): The fill character
        align (str, optional): The alignment. Can be: 'left', 'right', 'center'
            or the one-letter equivalents.
    
    Returns:
        str: The string of length `num_cols`
    
    Raises:
        ValueError: For invalid `align` or `fill_char` argument
    """
    if len(fill_char) != 1:
        raise ValueError("Argument `fill_char` needs to be string of length 1 "
                         "but was: "+str(fill_char))

    fill_str = fill_char * (num_cols - len(s))

    if align in ["left", "l", None]:
        return s + fill_str

    elif align in ["right", "r"]:
        return fill_str + s

    elif align in ["center", "centre", "c"]:
        return fill_str[:len(fill_str)//2] + s + fill_str[len(fill_str)//2:]

    raise ValueError("align argument '{}' not supported".format(align))

def center_in_line(s: str, *, num_cols: int=TTY_COLS, fill_char: str="·", spacing: int=1) -> str:
    """Shortcut for a common fill_line use case.
    
    Args:
        s (str): The string to center in the line
        num_cols (int, optional): The number of columns in the line
        fill_char (str, optional): The fill character
        spacing (int, optional): The spacing around the string `s`
    
    Returns:
        str: The string centered in the line
    """
    spacing = " " * spacing
    return fill_line(spacing + s + spacing, num_cols=num_cols,
                     fill_char=fill_char, align='centre')

def pprint(obj, **kwargs):
    """Prints a "pretty" string representation of the given object.
    
    Args:
        obj (TYPE): The object to print
        **kwargs: Passed to print
    """
    print(pformat(obj), **kwargs)

def pformat(obj) -> str:
    """Creates a "pretty" string representation of the given object.

    This is achieved by creating a yaml representation.
    """
    sstream = io.StringIO("")
    yaml.dump(obj, stream=sstream)
    sstream.seek(0)
    return sstream.read()
