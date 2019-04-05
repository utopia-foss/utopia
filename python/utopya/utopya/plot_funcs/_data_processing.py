"""This module holds data processing functionality that is used in the plot
functions.
"""

from typing import Union, List, Tuple

import xarray as xr

from ..tools import OPERATOR_MAP

# -----------------------------------------------------------------------------

def process_data(data: xr.DataArray, *,
                 sum_over: Tuple[str]=None):
    """processes the given xarray data"""
    # Summing
    if sum_over:
        data = data.sum(dim=sum_over)

    return data

def create_mask(data: xr.DataArray,
                operator_name: str, rhs_value: float) -> xr.DataArray:
    """Given the data, returns a binary mask by applying the following
    comparison: ``data <operator> rhs value``.
    
    Args:
        data (xr.DataArray): The data to apply the comparison to. This is the
            lhs of the comparison.
        operator_name (str): The name of the binary operator function as
            registered in utopya.tools.OPERATOR_MAP
        rhs_value (float): The right-hand-side value
    
    Returns:
        xr.DataArray: Boolean mask
    
    Raises:
        KeyError: On invalid operator name
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

    # Apply the comparison and return
    return comp_func(data, rhs_value)
