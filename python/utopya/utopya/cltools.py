"""Methods needed to implement the utopia command line interface"""

import os
import logging
from pkg_resources import resource_filename

from utopya.multiverse import Multiverse

# Configure and get logger
log = logging.getLogger(__name__)

# Local constants
USER_CFG_HEADER_PATH = resource_filename('utopya', 'cfg/user_cfg_header.yml')
BASE_CFG_PATH = resource_filename('utopya', 'cfg/base_cfg.yml')

# -----------------------------------------------------------------------------

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
