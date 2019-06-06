"""Helper module that can manipulate system path, e.g. to make additional
utopya-related modules available.
"""
import os
import sys
import logging
from typing import Sequence

from .cfg import load_from_cfg_dir

# Local constants
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

def add_modules_to_path(*modules, cfg_name: str='external_module_paths',
                        ignore_missing: bool=False):
    """Reads the modules configuration file from the configuration directory
    and adds the paths stores under the keys given in ``modules`` to sys.path
    """
    module_cfg = load_from_cfg_dir(cfg_name)

    for module_name in modules:
        try:
            path = module_cfg[module_name]

        except KeyError as err:
            if ignore_missing:
                continue
            raise KeyError("Missing configuration entry for '{}' module!"
                           "".format(module_name)
                           ) from err
        
        if path not in sys.path:
            sys.path.append(path)
            log.debug("Added path of '%s' module to sys.path: %s",
                      module_name, path)
