"""For functions that are not bound to classes, but generally useful."""

import os
import sys
import io
import logging
import collections
import subprocess
from datetime import timedelta
from typing import Union, Tuple, Sequence, Callable

from ._yaml import yaml, load_yml, write_yml

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
    if not isinstance(d, collections.abc.Mapping):
        # It is not. Need not go any further and can just return `u`
        return u

    # d can now assumed to be a mapping
    # Iterate over the keys of the update dict and (recursively) add them
    for key, val in u.items():
        if isinstance(val, collections.abc.Mapping):
            # Already a Mapping, continue recursion
            d[key] = recursive_update(d.get(key, {}), val)
        else:
            # Not a mapping -> at leaf -> update value
            d[key] = val    # ... which is just u[key]

    # Finished at this level; return updated dict
    return d


def load_selected_keys(src: dict, *, add_to: dict,
                       keys: Sequence[Tuple[str, type, bool]], 
                       err_msg_prefix: str=None,
                       prohibit_unexpected: bool=True) -> None:
        """Loads (only) selected keys from dict ``src`` into dict ``add_to``.
        
        Args:
            src (dict): The dict to load values from
            add_to (dict): The dict to load values into
            keys (Sequence[Tuple[str, type, bool]]): Which keys to load, given
                as sequence of (key, allowed types, [required=False]) tuples.
            description (str): A description string, used in error message
            prohibit_unexpected (bool, optional): Whether to raise on keys
                that were unexpected, i.e. not given in ``keys`` argument.
        
        Raises:
            KeyError: On missing key in ``src``
            TypeError: On bad type of value in ``src``
            ValueError: On unexpected keys in ``src``
        """
        for spec in keys:
            if len(spec) == 3:
                k, allowed_types, required = spec
            else:
                k, allowed_types = spec
                required = False

            if k not in src:
                if not required:
                    continue
                raise KeyError("{}Missing required key: {}"
                               "".format(err_msg_prefix + " "
                                         if err_msg_prefix else "",
                                         k))

            if not isinstance(src[k], allowed_types):
                raise TypeError("{}Bad type for value of '{}': {}! Expected "
                                "{} but got {}."
                                "".format(err_msg_prefix + " "
                                          if err_msg_prefix else "",
                                          k, src[k], allowed_types,
                                          type(src[k])))

            add_to[k] = src[k]

        if not prohibit_unexpected:
            return

        unexpected_keys = [k for k in src if k not in add_to]
        if unexpected_keys:
            raise ValueError("{}Received unexpected keys: {}! "
                             "Expected only: {}"
                             "".format(err_msg_prefix + " "
                                       if err_msg_prefix else "",
                                       ", ".join(unexpected_keys),
                                       ", ".join([s[0] for s in keys])))


def add_item(value, *, add_to: dict, key_path: Sequence[str],
             value_func: Callable=None, is_valid: Callable=None,
             ErrorMsg: Callable=None) -> None:
    """Adds the given value to the ``add_to`` dict, traversing the given key
    path. This operation happens in-place.
    
    Args:
        value: The value of what is to be stored. If this is a callable, the
            result of the call is stored.
        add_to (dict): The dict to add the entry to
        key_path (Sequence[str]): The path at which to add it
        value_func (Callable, optional): If given, calls it with `value` as
            argument and uses the return value to add to the dict
        is_valid (Callable, optional): Used to determine whether `value` is
            valid or not; should take single positional argument, return bool
        ErrorMsg (Callable, optional): A raisable object that prints an error
            message; gets passed `value` as positional argument.
    
    Raises:
        ErrorMsg: (depends on specified exception callable)
    """
    # Check the value by calling the function; it should raise an error
    if is_valid is not None:
        if not is_valid(value):
            raise ErrorMsg(value)

    # Determine which keys need to be traversed
    traverse_keys, last_key = key_path[:-1], key_path[-1]

    # Set the starting point
    d = add_to

    # Traverse keys
    for _key in traverse_keys:
        # Check if a new entry is needed
        if _key not in d:
            d[_key] = dict()

        # Select the new entry
        d = d[_key]

    # Now d is where the value should be added
    # If applicable
    if value_func is not None:
        value = value_func(value)
    
    # Store in dict, mutable. Done.
    d[last_key] = value

# string formatting -----------------------------------------------------------

def format_time(duration: Union[float, timedelta], *,
                ms_precision: int=0, max_num_parts: int=None) -> str:
    """Given a duration (in seconds), formats it into a string.
    
    The formatting divisors are: days, hours, minutes, seconds
    
    If `ms_precision` > 0 and `duration` < 60, decimal places will be shown
    for the seconds.
    
    Args:
        duration (Union[float, timedelta]): The duration in seconds to format
            into a duration string; it can also be a timedelta object.
        ms_precision (int, optional): The precision of the seconds slot
        max_num_parts (int, optional): How many parts to include when creating
            the formatted time string. For example, if the time consists of
            the parts seconds, minutes, and hours, and the argument is ``2``,
            only the hours and minutes parts will be shown.
            If None, all parts are included.
    
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

    # Prepend a minus for negative values
    if is_negative:
        parts = ["-"] + parts

    # Join the parts together, but only the maximum number of parts
    return " ".join(parts[:(max_num_parts if max_num_parts else len(parts))])

def fill_line(s: str, *, num_cols: int=TTY_COLS, fill_char: str=" ",
              align: str="left") -> str:
    """Extends the given string such that it fills a whole line of `num_cols`
    columns.
    
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

def center_in_line(s: str, *, num_cols: int=TTY_COLS, fill_char: str="·",
                   spacing: int=1) -> str:
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
