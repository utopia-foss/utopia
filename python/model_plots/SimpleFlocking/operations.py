"""Custom DAG data transformation operations for the SimpleFlocking model"""

import numpy as np
import xarray as xr

from utopya.eval import is_operation


@is_operation
def set_nan_at_discontinuities(
    ds: xr.Dataset, *, variable: str, threshold: float
) -> xr.Dataset:
    """Given a 1D dataset, detects discontinuities (jumps that have an absolute
    value beyond a threshold, e.g. half the domain size) in *one* data variable
    and sets the first value after such a discontinuity to NaN for all data
    variables.

    While this changes the data, it makes visualization easier.
    In contrast to ``insert_nan_at_discontinuities``, this function will not
    lead to additional coordinate values.

    Args:
        ds (xr.Dataset): The dataset
        variable (str): The name of the data variable to look for
            discontinuities in.
        threshold (float): The threshold beyond which a jump is considered a
            discontinuity.
    """
    if len(ds.dims) != 1:
        raise ValueError("Expected 1D xr.Dataset, got:\n{ds}")

    # Need to extract data; xarray does not allow insertion, apparently
    coords = ds.coords
    data_vars = {k: v.data for k, v in ds.data_vars.items()}

    # Find positions of discontinuities, then set those positions to NaN
    pos = np.where(np.abs(np.diff(data_vars[variable])) >= threshold)[0]
    for k, v in data_vars.items():
        v[pos] = np.nan

    # Reconstruct the xr.Dataset
    return xr.Dataset(
        {
            k: xr.DataArray(v, name=k, coords=coords)
            for k, v in data_vars.items()
        }
    )


@is_operation
def insert_nan_at_discontinuities(
    ds: xr.Dataset, *, variable: str, threshold: float
) -> xr.Dataset:
    """Given a 1D dataset, detects discontinuities (jumps that have an absolute
    value beyond a threshold, e.g. half the domain size) in *one* data variable
    and inserts NaN values at those point for all data variables.

    When plotting, this leads to a nicer visual representation.

    .. warning::

        If the resulting dataset is combined with other data, there will be
        additional coordinates, thus the discontinuity will show in all other
        data dimensions as well! This is due to the shared coordinates.

    Args:
        ds (xr.Dataset): The dataset
        variable (str): The name of the data variable to look for
            discontinuities in.
        threshold (float): The threshold beyond which a jump is considered a
            discontinuity.
    """
    if len(ds.dims) != 1:
        raise ValueError("Expected 1D xr.Dataset, got:\n{ds}")

    # Need to extract data; xarray does not allow insertion, apparently
    coords = ds.coords
    data_vars = {k: v.data for k, v in ds.data_vars.items()}

    # Find positions of discontinuities, then insert data *after* that
    # position (hence the +1)
    pos = np.where(np.abs(np.diff(data_vars[variable])) >= threshold)[0]
    insert_nan = lambda d: np.insert(d, pos + 1, np.nan)

    data_vars = {k: insert_nan(v) for k, v in data_vars.items()}

    # Need to do the same for the coordinates, but interpolate the NaNs away
    # to not have duplicate coordinate values later on
    coords_name = list(coords.dims)[0]
    coords_data = list(coords.values())[0].data
    coords_data = xr.DataArray(
        insert_nan(coords_data.astype(float)), dims=(coords_name,)
    )
    coords_data = coords_data.interpolate_na(coords_name)

    # Reconstruct the xr.Dataset
    return xr.Dataset(
        {
            k: xr.DataArray(
                v,
                name=k,
                dims=(coords_name,),
                coords={coords_name: coords_data},
            )
            for k, v in data_vars.items()
        }
    )
