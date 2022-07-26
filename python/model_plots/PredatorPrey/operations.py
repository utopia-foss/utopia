"""Custom dantro DAG framework operations for the PredatorPrey model"""

import logging

import xarray as xr

from utopya.eval import is_operation

log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------


@is_operation("PredatorPrey.combine_positions")
def combine_positions(
    predator: xr.DataArray, prey: xr.DataArray
) -> xr.DataArray:

    """Given the predator and prey positions, combine their positions into an
    xr.DataArray, also creating an entry 'both' for if a cell contains both
    predators and preys.

        Args:
            predator (xr.DataArray): The predator dataset
            prey (xr.DataArray): The prey dataset
    """
    both = xr.where(predator + prey != 0, 3, 0).rename("both")
    predator = predator.where(both != 0, 1, 0)
    prey = prey.where(both != 0, 2, 0)

    return prey + predator + both
