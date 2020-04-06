"""Helper module that can manipulate system path, e.g. to make additional
utopya-related modules available.
"""
import os
import sys
import copy
import logging
from typing import Sequence

from .cfg import load_from_cfg_dir

# Local constants
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

class temporary_sys_path:
    """A sys.path context manager, temporarily adding a path and removing it
    again upon exiting. If the given path already exists in the sys.path, it
    is neither added nor removed and the sys.path remains unchanged.
    """
    def __init__(self, path: str):
        self.path = path
        self.path_already_exists = self.path in sys.path

    def __enter__(self):
        if not self.path_already_exists:
            log.debug("Temporarily adding '%s' to sys.path ...")
            sys.path.insert(0, self.path)

    def __exit__(self, *_):
        if not self.path_already_exists:
            log.debug("Removing temporarily added path from sys.path ...")
            sys.path.remove(self.path)


class temporary_sys_modules:
    """A context manager for the sys.modules cache, ensuring that it is in the
    same state after exiting as it was before entering the context manager.
    """
    def __init__(self):
        self.modules = copy.copy(sys.modules)

    def __enter__(self):
        pass

    def __exit__(self, *_):
        if sys.modules != self.modules:
            sys.modules = self.modules
            log.debug("Resetted sys.modules cache to state before the "
                      "context manager was added.")

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
            log.debug("Added path '%s' to sys.path.", path)
