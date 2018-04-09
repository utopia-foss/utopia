"""For functions that are not bound to classes, but useful."""

import os
import yaml
import re

import numpy as np
import paramspace.yaml_constructors as psp_constrs

import utopya.stopcond


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
    # check whether file already exists
    if os.path.exists(path):
        raise FileExistsError("Target file {0} already exists.".format(path))
    
    # else: dump the dict into the config file
    with open(path, 'w') as ymlout:
        yaml.dump(d, ymlout, default_flow_style=False)

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
    # FIXME these will cause errors if emitting again to C++
    if expr_str in ['np.nan', 'nan', 'NaN']:
        return np.nan

    elif expr_str in ['np.inf', 'inf', 'INF']:
        return np.inf

    elif expr_str in ['-np.inf', '-inf', '-INF']:
        return -np.inf

    # remove everything that might cause trouble -- only allow digits, dot, +,
    # -, *, /, and eE to allow for writing exponentials
    expr_str = re.sub(r'[^0-9eE\-.+\*\/]', '', expr_str)

    # Try to eval
    return eval(expr_str)


# Add the constructors to the yaml module
yaml.add_constructor(u'!expr', _expr_constructor)
yaml.add_constructor(u'!stop-condition', utopya.stopcond.stop_cond_constructor)
yaml.add_constructor(u'!sc-func', utopya.stopcond.sc_func_constructor)
yaml.add_constructor(u'!pspace', psp_constrs.pspace)
yaml.add_constructor(u'!sweep', psp_constrs.pdim_enabled_only)
yaml.add_constructor(u'!sweep-default', psp_constrs.pdim_get_default)
