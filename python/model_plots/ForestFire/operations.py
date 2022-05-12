"""Custom data operations for the ForestFire model"""

import logging
from math import ceil, floor
from typing import Dict, Tuple

import numpy as np
import xarray as xr

log = logging.getLogger(__name__)

# TODO Consider implementing this in dantro or utopya?
def isel_range_fraction(
    d: xr.DataArray, dims: Dict[str, Tuple[float, float]], **isel_kwargs
) -> xr.DataArray:
    """Applies index selection on the data, but the to be selected indices can
    be defined using a fractional range.
    For instance, ``(0, 0.5)`` selects the first half of indices along a
    dimension, ``(0.9, 1.0)`` the last 10%.

    .. note::

        The first index that will be selected is rounded *down*; the last is
        rounded *up*. The resulting range is inclusive on *both* ends.
    """
    is_valid = lambda v: v >= 0 and v <= 1

    indexers = dict()
    for dim, (f_start, f_stop) in dims.items():
        if not is_valid(f_start) or not is_valid(f_stop) or f_stop <= f_start:
            raise ValueError(
                f"The start and/or stop range fractions ({f_start}, {f_stop}) "
                f"for dimension '{dim}' are invalid! "
                "Values need be in [0, 1] and fulfil start ≤ stop."
            )

        max_idx = d.sizes[dim] - 1
        first_idx = floor(f_start * max_idx)
        last_idx = ceil(f_stop * max_idx)  # inclusive
        indexers[dim] = range(first_idx, last_idx + 1)

    return d.isel(indexers, **isel_kwargs)


def compute_compl_cum_distr(
    data: np.ndarray,
    *,
    mask_repeated: bool = False,
):
    """Computes a complementary cumulative distribution from the given data.
    Expects integer-type data that can be binned using ``np.unique``.

    Args:
        data (np.ndarray): Integer-type array data from which unique counts
            can be computed.
        mask_repeated (bool, optional): Whether to mask repeated count values.
            These repeated values indicate that there were bins with zero
            counts in the non-cumulative distribution; the visual appearance
            is somewhat cleaner if not drawing those points.

    Returns:
        xr.DataArray: 1D array of counts, labelled along ``bin_pos`` dimension
    """
    if not np.issubdtype(data.dtype, np.integer):
        raise TypeError(
            "Calculating a histogram via np.unique using data of "
            f"non-integer dtype {data.dtype} might lead to undesired results! "
            "Make sure that the data is of the expected data tpye.\n"
            f"Given data was:\n{data}"
        )

    bin_pos, counts = np.unique(data, return_counts=True)

    if mask_repeated:
        mask = (counts - np.roll(counts, 1)) != 0

        bin_pos = bin_pos[mask]
        counts = counts[mask]

    counts = np.cumsum(counts[::-1])[::-1]

    return xr.DataArray(
        counts, dims=("bin_pos",), coords=dict(bin_pos=bin_pos)
    )
