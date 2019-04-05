"""This module holds data processing functionality that is used in the plot
functions.
"""

from typing import Union, List, Tuple

import xarray as xr

# -----------------------------------------------------------------------------

def preprocess_data(data: xr.DataArray, *, sum_over: Tuple[str]=None):
    """Pre-processes the given xarray data"""
    if sum_over:
        data = data.sum(dim=sum_over)

    # TODO add more pre-processing here

    return data
