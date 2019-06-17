"""Methods needed to implement the utopia command line interface"""

import os
import re
import logging
from typing import Callable, Dict
from pkg_resources import resource_filename

from .multiverse import Multiverse
from .tools import recursive_update, add_item

# Local constants
log = logging.getLogger(__name__)

USER_CFG_HEADER_PATH = resource_filename('utopya', 'cfg/user_cfg_header.yml')
BASE_CFG_PATH = resource_filename('utopya', 'cfg/base_cfg.yml')

# -----------------------------------------------------------------------------

def add_from_kv_pairs(*pairs, add_to: dict,
                      attempt_conversion: bool=True,
                      allow_eval: bool=False,
                      allow_deletion: bool=True) -> None:
    """Parses the key=value pairs and adds them to the given dict.

    Note that this happens directly on the object, i.e. making use of the
    mutability of the given dict. This function has no return value!
    
    Args:
        *pairs: Sequence of key=value strings
        add_to (dict): The dict to add the pairs to
        attempt_conversion (bool, optional): Whether to attempt converting the
            strings to bool, float, int types
        allow_eval (bool, optional): Whether to try calling eval() on the
            value strings during conversion
        allow_deletion (bool, optional): If set, can pass DELETE string to
            a key to remove the corresponding entry.
    """
    # Object to symbolise deletion
    class _DEL: pass
    DEL = _DEL()

    def conversions(val):
        # Boolean
        if val.lower() in ["true", "false"]:
            return bool(val.lower() == "true")
        
        # None
        if val.lower() in ["null"]:
            return None

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

        # Deletion placeholder
        if val == "DELETE":
            return DEL

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

        # In all cases but that where the value is the DEL object, write:
        if val is not DEL:
            d[last_key] = val
            continue

        # Otherwise: need to check whether deletion is allowed and the entry
        # is present ...
        if not allow_deletion:
            raise ValueError("Attempted deletion of value for key '{}', but "
                             "deletion is not allowed.".format(key))

        if last_key not in d:
            continue
        del d[last_key]

    # No need to return the base dict as it is a mutable!
    log.debug("Added %d entries from key-value pairs.", len(pairs))


def register_models(args, *, registry):
    """Handles registration of multiple models given argparse args"""
    # The dict to hold all arguments
    specs = dict()

    if not args.separator:
        # Will only register a single model.
        # Gather all the path-related arguments
        raise NotImplementedError() # TODO
        # paths = dict()
        # specs[args.model_name] = dict(paths=paths)

    else:
        # Got separator for lists of model names, binary paths, and source dirs
        log.debug("Splitting given model registration arguments by '%s' ...",
                  args.separator)

        model_names = args.model_name.split(args.separator)
        bin_paths = args.bin_path.split(args.separator)
        src_dirs = args.src_dir.split(args.separator)

        if not (len(model_names) == len(bin_paths) == len(src_dirs)):
            raise ValueError("Mismatch of sequence lengths during batch model "
                             "registration! The model_name, bin_path, and "
                             "src_dir lists should all be of equal length "
                             "after having been split by separator '{}', but "
                             "were: {}, {}, and {}, respectively."
                             "".format(args.separator, model_names,
                                       bin_paths, src_dirs))
        # TODO Will ignore other path-related arguments! Warn if given.

        # Go over them, create the path_args dict, and populate specs dict
        for model_name, bin_path, src_dir in zip(model_names,
                                                 bin_paths, src_dirs):
            paths = dict(src_dir=src_dir,
                         binary=bin_path,
                         base_src_dir=args.base_src_dir,
                         base_bin_dir=args.base_bin_dir)
            specs[model_name] = dict(paths=paths)

    log.debug("Received registry parameters for %d model%s.",
              len(specs), "s" if len(specs) != 1 else "")

    # Now, actually register. Here, pass along the common arguments.
    for model_name, bundle_kwargs in specs.items():
        registry.register_model_info(model_name, **bundle_kwargs,
                                     exists_action=args.exists_action,
                                     label=args.label,
                                     overwrite_label=args.overwrite_label
                                     )

    log.info("Model registration finished.\n\n%s\n", registry.info_str)


def deploy_user_cfg(user_cfg_path: str=Multiverse.USER_CFG_SEARCH_PATH
                    ) -> None:
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
