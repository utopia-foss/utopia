"""Module that coordinates the Utopia Configuration Directory"""

import os
import logging

from ._yaml import load_yml, write_yml

log = logging.getLogger(__name__)

# Some globally relevant variables --------------------------------------------
# Path to the persistent Utopia configuration directory
UTOPIA_CFG_DIR = os.path.expanduser('~/.config/utopia')

# Names of configuration entries
UTOPIA_CFG_FILE_NAMES = dict(
    user='user_cfg.yml',
    utopya='utopya_cfg.yml',
    batch='batch_cfg.yml',
    projects='projects.yml',
    external_module_paths='external_module_paths.yml',
    plot_module_paths='plot_module_paths.yml',
)

# Corresponding absolute paths
UTOPIA_CFG_FILE_PATHS = {k: os.path.join(UTOPIA_CFG_DIR, fname)
                         for k, fname in UTOPIA_CFG_FILE_NAMES.items()}

# -----------------------------------------------------------------------------

def get_cfg_path(cfg_name: str) -> str:
    """Returns the absolute path to the specified configuration file"""
    try:
        return UTOPIA_CFG_FILE_PATHS[cfg_name]

    except KeyError as err:
        raise KeyError("No configuration entry '{}' available! Possible keys: "
                       "{}".format(cfg_name,
                                   ", ".join(UTOPIA_CFG_FILE_NAMES.keys()))
                       ) from err

def load_from_cfg_dir(cfg_name: str) -> dict:
    """Load a configuration file; returns empty dict if no file exists.

    Args:
        cfg_name (str): The name of the configuration to read

    Returns:
        dict: The configuration as read from the config directory; if no file
            is available, will return an empty dict.
    """
    cfg_fpath = get_cfg_path(cfg_name)
    try:
        d = load_yml(cfg_fpath)

    except FileNotFoundError:
        log.debug("No '%s' configuration file exists at %s ! Returning empty.",
                  cfg_name, cfg_fpath)
        return dict()

    # If the yaml object is None for whatever reason, return an empty dict
    if d is None:
        return dict()
    return d

def write_to_cfg_dir(cfg_name: str, obj: dict):
    """Writes a YAML represetation of the given object to the configuration
    directory. Always overwrites a possibly existing file.

    Args:
        cfg_name (str): The configuration name
        obj (dict): The yaml-representable object that is to be written;
            usually a dict.
    """
    write_yml(obj, path=get_cfg_path(cfg_name))
