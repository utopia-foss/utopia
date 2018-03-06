"""For functions that are not bound to classes, but useful."""
import yaml


def recursive_update(d: dict, u: dict) -> dict:
    """Update dict d with values from dict u."""
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
