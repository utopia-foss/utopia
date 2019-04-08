"""This module holds data processing functionality that is used in the plot
functions.
"""

import operator
import logging
from typing import Union, List, Tuple

import numpy as np
import xarray as xr

# Local variables .............................................................
log = logging.getLogger(__name__)


# A map from operator names to binary functions returning booleans
OPERATOR_MAP = {
    '==': operator.eq,  'eq': operator.eq,
    '<': operator.lt,   'lt': operator.lt,
    '<=': operator.le,  'le': operator.le,
    '>': operator.gt,   'gt': operator.gt,
    '>=': operator.ge,  'ge': operator.ge,
    '!=': operator.ne,  'ne': operator.ne,
    '^': operator.xor,  'xor': operator.xor,
    # Expecting an iterable as second argument
    'in':              (lambda x, y: x in y),
    'not in':          (lambda x, y: x not in y),
    # Performing bitwise boolean operations to support numpy logic
    'in interval':     (lambda x, y: x >= y[0] & x <= y[1]),
    'not in interval': (lambda x, y: x < y[0] | x > y[1]),
}

# Data Processing functions ---------------------------------------------------

def create_mask(data: xr.DataArray, *,
                operator_name: str, rhs_value: float) -> xr.DataArray:
    """Given the data, returns a binary mask by applying the following
    comparison: ``data <operator> rhs value``.
    
    Args:
        data (xr.DataArray): The data to apply the comparison to. This is the
            lhs of the comparison.
        operator_name (str): The name of the binary operator function as
            registered in utopya.tools.OPERATOR_MAP
        rhs_value (float): The right-hand-side value
    
    Raises:
        KeyError: On invalid operator name
    
    No Longer Returned:
        xr.DataArray: Boolean mask
    """
    # Get the operator function
    try:
        comp_func = OPERATOR_MAP[operator_name]

    except KeyError as err:
        raise KeyError("No operator with name '{}' available! "
                       "Available operators: {}"
                       "".format(operator_name,
                                 ", ".join(OPERATOR_MAP.keys()))
                       ) from err

    # Apply the comparison
    data = comp_func(data, rhs_value)

    # Create a new name
    name = data.name + " (masked by '{} {}')".format(operator_name, rhs_value)

    # Build a new xr.DataArray from that data, retaining all information
    return xr.DataArray(data=data,
                        name=name,
                        dims=data.dims,
                        coords=data.coords,
                        attrs=dict(**data.attrs))


def count_unique(data, **kwargs):
    """Applies np.unique to the given data"""
    unique, counts = np.unique(data, return_counts=True)

    # Construct a new data array and return
    return xr.DataArray(data=counts,
                        name=data.name + " (unique counts)",
                        dims=('unique',),
                        coords=dict(unique=unique),
                        attrs=dict(**data.attrs))


# Transformation interface ----------------------------------------------------

# A mapping of transformation operations to callables.
# All callables expect xr.DataArray as their first argument and additional
# arguments as their second argument
TRANSFORMATIONS = {
    # Select data by value
    'sel':      lambda d, kws: d.sel(**(kws if kws else {})),

    # Select data by index
    'isel':     lambda d, kws: d.isel(**(kws if kws else {})),

    # Calculate the sum
    'sum':      lambda d, kws: d.sum(**(kws if isinstance(kws, dict)
                                        else dict(dim=kws))),

    # Calculate the mean
    'mean':     lambda d, kws: d.mean(**(kws if isinstance(kws, dict)
                                         else dict(dim=kws))),

    # Calculate the median
    'median':   lambda d, kws: d.median(**(kws if isinstance(kws, dict)
                                           else dict(dim=kws))),

    # Calculate the standard deviation
    'std':      lambda d, kws: d.std(**(kws if isinstance(kws, dict)
                                        else dict(dim=kws))),

    # Calculate the minimum
    'min':      lambda d, kws: d.min(**(kws if isinstance(kws, dict)
                                        else dict(dim=kws))),

    # Calculate the maximum
    'max':      lambda d, kws: d.max(**(kws if isinstance(kws, dict)
                                        else dict(dim=kws))),

    # Perform a squeeze operation
    'squeeze':  lambda d, kws: d.squeeze(**(kws if kws else {})),

    # Count unique values
    'count_unique': lambda d, _: count_unique(d),
    
    # Count unique values
    'create_mask':  
        lambda d, kws: create_mask(d, **(kws if isinstance(kws, dict)
                                         else dict(operator_name=kws[0],
                                                   rhs_value=kws[1]))),

    # Logarithms
    'log':      lambda d, _: np.log(d),
    'log10':    lambda d, _: np.log10(d),
    'log2':     lambda d, _: np.log2(d),
    'log1p':    lambda d, _: np.log1p(d),

    # Powers
    'power':    lambda d, e: np.power(d, e),
    'squared':  lambda d, _: np.square(d),
    'sqrt':     lambda d, _: np.sqrt(d),
    'cubed':    lambda d, _: np.power(d, 3),
    'sqrt3':    lambda d, _: np.power(d, 1./.3),

    # Normalizations
    'normalize_to_sum':         lambda d, _: d / np.sum(d),
    'normalize_to_max':         lambda d, _: d / np.max(d),

    # Cumulation
    'cumulate':                 lambda d, _: np.cumsum(d),
    'cumulate_complementary':   lambda d, _: np.cumsum(d[::-1])[::-1]
}

# End of TRANSFORMATIONS dict population
# .............................................................................


def transform(data: xr.DataArray, *operations: Union[dict, str],
              log_level: int=None) -> xr.DataArray:
    """Applies transformations to the given data, e.g.: reducing dimensionality
    or calculating 
    
    Args:
        data (xr.DataArray): The data that is to be reduced in dimensionality
        *operations (Union[dict, str]): Which operations to apply and with
            which parameters. These should be operation names or dicts. For
            dicts, they should only have a single key, which is the name of
            the operation to perform.
            The available operations are defined in the TRANSFORMATIONS dict.
        log_level (int, optional): Which level to log the progress of the
            operations on. If not given, will be 10.
    
    Returns:
        xr.DataArray: A new object with dimensionality-reduced data.
    
    Raises:
        TypeError: On bad ``operations`` specification
        ValueError: On bad operation name
    """
    # Set default log level
    log_level = log_level if log_level is not None else 10

    log.log(log_level, "Performing %d transformation%s on input data:\n%s\n",
            len(operations), "s" if len(operations) != 1 else "", data)

    # Apply operations in the given order
    for i, op_spec in enumerate(operations):
        if isinstance(op_spec, str):
            op_name = op_spec
            op_spec = dict()
            may_fail = False

        elif not isinstance(op_spec, dict):
            raise TypeError("Entries of `operations` argument need to be dict"
                            "-like, but entry {} was {}!"
                            "".format(i, repr(op_spec)))

        else:
            # Get the names that don't start with _
            names = [k for k in op_spec.keys() if not k.startswith("_")]

            if len(names) != 1:
                raise ValueError("Could not extract an operation name from "
                                 "`operations` argument at index {}! For dict-"
                                 "like arguments, the dict need to contain "
                                 "one and only one key that does not start "
                                 "with '_'. Got keys: {}. Full dict: {}"
                                 "".format(i, ", ".join(names), op_spec))
            
            # Get the single name
            op_name = names[0]
            may_fail = op_spec.get('_may_fail', False)
            op_spec = op_spec[op_name]

        # Get the transform operation
        try:
            op_func = TRANSFORMATIONS[op_name]

        except KeyError as err:
            raise ValueError("No transform operation with name '{}' is "
                             "available! Choose from: {}"
                             "".format(op_name,
                                       ", ".join(TRANSFORMATIONS.keys()))
                             ) from err

        # Apply it
        log.log(log_level, "Applying operation %d/%d:  %s  ...",
                i+1, len(operations), op_name)
        log.log(log_level, "  … with arguments:  %s", op_spec)

        try:
            data = op_func(data, op_spec)

        except Exception as exc:
            if may_fail:
                log.warning("Operation '%s' failed with %s: %s",
                            op_name, exc.__class__.__name__, exc)
                continue
            raise

        log.log(log_level, "Result:\n%s\n", data)

    return data

