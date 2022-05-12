"""Plots and operations specific to the ForestFire model"""

from utopya.eval import register_operation

from .operations import compute_compl_cum_distr, isel_range_fraction

register_operation(
    name="ForestFire.compl_cum_distr", func=compute_compl_cum_distr
)

register_operation(
    name="ForestFire.isel_range_fraction", func=isel_range_fraction
)
