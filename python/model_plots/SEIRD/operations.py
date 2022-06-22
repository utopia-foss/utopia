"""Custom dantro DAG framework operations for the SEIRD model"""

import logging
from typing import Sequence

import numpy as np
import xarray as xr
from dantro.data_ops.arr_ops import count_unique

log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------


def compute_age_distribution(
    *,
    age: xr.DataArray,
    kind: xr.DataArray,
    coarsen_by: int = 1,
    compute_for_kinds: Sequence[str] = (
        "susceptible",
        "exposed",
        "infected",
        "recovered",
        "deceased",
    ),
    count_over: Sequence[str] = ("x", "y"),
    normalize: bool = False,
) -> xr.DataArray:
    """Given the age dataset and the kinds, compute the age distributions for
    all kinds, and finally coarsen it by a certain value.

    Args:
        age (xr.DataArray): The age dataset
        kind (xr.DataArray): The kind dataset
        coarsen_by (int, optional): How many ages to bin together.
        compute_for_kinds (Sequence[str], optional): The kinds to compute the
            distributions for.
        count_over (Sequence[str], optional): The dimensions to count the ages
            over. Typically want to do this over the spatial dimensions.
        normalize (bool, optional): If given, normalizes the ``kind`` dimension

    No Longer Returned:
        xr.DataArray: with dimensions ``age`` and ``kind``, where the latter
            has coordinates that are the same as ``compute_for_kinds``.
            Note that ``age`` coordinates represent the lower *edge* of their
            respective bin.
    """
    # Retrieve the inverse state map (name -> int) from the `kind` dataset
    state_map = list(kind.attrs["kind_names"])
    assert len(state_map) == 8  # sanity check
    inv_state_map = {k: v for v, k in enumerate(state_map)}

    # The dataset to store distributions in
    dists = xr.Dataset()

    for kind_str in compute_for_kinds:
        kind_id = inv_state_map[kind_str]

        # Get the ages for this kind
        age_for_this_kind = age.where(kind == kind_id)

        # Count unique values to arrive at a distribution
        age_dist = count_unique(age_for_this_kind, dims=count_over)

        # Rename dimensions and DataArray name
        age_dist = age_dist.rename(unique="age")
        age_dist.name = kind_str

        # Pad coordinates such that they start at 0
        pad_l = int(age_dist.coords["age"].min())
        age_dist = age_dist.pad({"age": (pad_l, 0)}, constant_values=np.nan)
        age_dist = age_dist.assign_coords(age=range(age_dist.sizes["age"]))

        # Store in Dataset
        dists[kind_str] = age_dist

    # Combine to integer array and get rid of NaNs (makes it easier to handle)
    dists = dists.to_array(dim="kind").fillna(0).astype(int)

    # Coarsen and normalize, if desired
    if coarsen_by:
        log.remark("Coarsening 'age' dimension by %d ...", coarsen_by)
        dists = dists.coarsen(
            {"age": coarsen_by}, boundary="pad", coord_func="min"
        ).sum()

    if normalize:
        log.remark("Normalizing over 'age' and 'kind' ...")
        dists = dists / dists.sum(("age", "kind"))

    return dists
