"""Methods needed to implement the utopia command line interface"""

import os
import re
import logging
from typing import Callable
from pkg_resources import resource_filename

from utopya.multiverse import Multiverse

# Configure and get logger
log = logging.getLogger(__name__)

# Local constants
USER_CFG_HEADER_PATH = resource_filename('utopya', 'cfg/user_cfg_header.yml')
BASE_CFG_PATH = resource_filename('utopya', 'cfg/base_cfg.yml')

# -----------------------------------------------------------------------------

def add_entry(value, *, add_to: dict, key_path: list, value_func: Callable=None, is_valid: Callable=None, ErrorMsg: Callable=None):
    """Adds the given value to the `add_to` dict at the given key path.
    
    If `value` is a callable, that function is called and the return value is
    what is stored in the dict.
    
    Args:
        value: The value of what is to be stored
        add_to (dict): The dict to add the entry to
        key_path (list): The path at which to add it
        value_func (Callable, optional): If given, calls it with `value` as
            argument and uses the return value to add to the dict
        is_valid (Callable, optional): Used to determine whether `value` is
            valid or not; should take single positional argument, return bool
        ErrorMsg (Callable, optional): A raisable object that prints an error
            message; gets passed `value` as positional argument.
    
    Raises:
        ErrorMsg: Description
    """
    log.debug("Adding CLI entry '%s' at %s ...")

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
        log.debug("Traversing key '%s' ...", _key)

        # Check if a new entry is needed
        if _key not in d:
            d[_key] = dict()

        # Select the new entry
        d = d[_key]

    # Now d is where the value should be added
    # If applicable
    if value_func is not None:
        value = value_func(value)
        log.debug("Resolved value from callable:  %s", value)
    
    # Store in dict, mutable
    d[last_key] = value

    log.debug("Successfully added entry.")


def add_from_kv_pairs(*pairs, add_to: dict, attempt_conversion: bool=True, allow_eval: bool=False):
    """Parses the key=value pairs and adds them to the given dict.
    
    Args:
        *pairs: Sequence of key=value strings
        base_dict (dict): The dict to add the keys to
        attempt_conversion (bool, optional): Whether to attempt converting the
            strings to bool, float, int types. This also tries calling eval on
            the string!
    """
    def conversions(val):
        # Boolean
        if val.lower() in ["true", "false"]:
            return bool(val.lower() == "true")

        # Floating point number (requiring '.' being present)
        if re.match(r'^[-+]?[0-9]*\.[0-9]*([eE][-+]?[0-9]+)?$', val):
            try:
                return float(val)
            except:
                pass

        # Integer
        if re.match(r'^[-+]?[0-9]+$', val):
            try:
                return int(val)
            except: # very unlike to be reached; regex is quite restrictive
                pass

        # Last resort, if activated: eval
        if allow_eval:
            try:
                return eval(val)
            except:
                pass

        # Just return the string
        return val

    log.debug("Adding entries from key-value pairs ...")

    # Go over all pairs and add them to the given base dict
    for kv in pairs:
        # Split key and value
        key, val = kv.split("=")

        # Process the key
        key_sequence = key.split(".")
        traverse_keys, last_key = key_sequence[:-1], key_sequence[-1]

        # Set temporary variable to root dict
        d = add_to

        # Traverse through the key sequence, if available
        for _key in traverse_keys:
            # Check if a new entry is needed
            if _key not in d:
                d[_key] = dict()

            # Select the new entry
            d = d[_key]

        # Attempt conversion
        if attempt_conversion:
            val = conversions(val)

        # Write the value
        d[last_key] = val

    # No need to return the base dict as it is a mutable!
    log.debug("Added %d entries from key-value pairs.", len(pairs))


def deploy_user_cfg(user_cfg_path: str=Multiverse.USER_CFG_SEARCH_PATH) -> None:
    """Deploys a copy of the full config to the specified location (usually
    the user config search path of the Multiverse class)
    
    Instead of just copying the full config, it is written line by line,
    commenting out lines that are not already commented out, and changing the
    header.
    
    Args:
        user_cfg_path (str, optional): The path the file is expected at. Is an
            argument in order to make testing easier.
    
    Returns:
        None
    """
    # Check if a user config already exists
    if os.path.isfile(user_cfg_path):
        # There already is one. Ask if this should be overwritten...
        print("A config file already exists at " + str(user_cfg_path))
        if input("Replace? [y, N]  ").lower() in ['yes', 'y']:
            # Delete the file
            os.remove(user_cfg_path)
            print("")

        else:
            # Abort here
            print("Not deploying user config.")
            return
    
    # At this point, can assume that it is desired to write the file and there
    # is no other file there
    # Make sure that the folder exists
    os.makedirs(os.path.dirname(user_cfg_path), exist_ok=True)

    # Create a file at the given location
    with open(user_cfg_path, 'x') as ucfg:
        # Write header section, from user config header file
        with open(USER_CFG_HEADER_PATH, 'r') as ucfg_header:
            ucfg.write(ucfg_header.read())

        # Now go over the full config and write the content, commenting out
        # the lines that are not already commented out
        with open(BASE_CFG_PATH, 'r') as bcfg:
            past_prefix = False

            for line in bcfg:
                # Look for "---" to find out when the header section ended
                if line == "---\n":
                    past_prefix = True
                    continue

                # Write only if past the prefix
                if not past_prefix:
                    continue

                # Check if the line in the target (user) config needs to be
                # commented out or not
                if line.strip().startswith("#") or line.strip() == "":
                    # Is a comment or empty line -> just write it
                    ucfg.write(line)

                else:
                    # There is an entry on this line -> comment out before the
                    # first character (looks cleaner)
                    spaces = " " * (len(line.rstrip()) - len(line.strip()))
                    ucfg.write(spaces + "# " + line[len(spaces):])
        # Done

    print("Deployed user config to: {}\n\nAll entries are commented out; "
          "open the file to edit your configuration. Note that it is wise to "
          "only enable those entries that you absolutely _need_ to set."
          .format(user_cfg_path))
