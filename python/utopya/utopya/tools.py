"""For functions that are not bound to classes, but useful."""

import os
import yaml


def recursive_update(d: dict, u: dict) -> dict:
    """Update dict d with values from dict u.

    No copy of d is created so its contents will be changed.

    Args:
        d: The dict to be updated
        u: The dict used to update

    Returns:
        dict: d with updated contents
    """
    for key, val in u.items():
        if isinstance(val, dict):
            # Already a Mapping, continue recursion
            d[key] = recursive_update(d.get(key, {}), val)
        else:
            # Not a mapping -> at leaf -> update value
            d[key] = val 	# ... which is just u[key]
    return d


def read_yml(path: str, error_msg: str=None) -> dict:
    """Read yaml configuration file and return dict.

    The given error_msg is given upon FileNotFoundError.

    Args:
        path:       path to yml file
        error_msg:  string to be printed upon FileNotFoundError. Defaults to None.

    Returns:
        dict: with contents of yml file. Raises FileNotFoundError if file is not found.
    """
    try:
        with open(path, 'r') as ymlfile:
            d = yaml.load(ymlfile)
    except FileNotFoundError as err:
        if error_msg:  # is None by default
            raise FileNotFoundError(error_msg) from err
        else:
            raise err
    return d


def write_yml(d: dict, path: str):
    """Write dict to yml file in path.

    Writes a given dictionary into a yaml file. Error is raised if file already exists.

    Args:
        d:      dict to be written
        path:   to output file
    """
    # check whether file already exists
    if os.path.exists(path):
        raise FileExistsError("Target file {0} already exists.".format(path))
    else:  # dump the dict into the config file
        with open(path, 'w') as ymlout:
            yaml.dump(d, ymlout, default_flow_style=False)
