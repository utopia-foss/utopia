"""Customisations of the CA plot for the PredatorPrey model"""

import numpy as np

from utopya.eval import (
    DataManager, UniverseGroup, is_plot_func, PlotHelper, UniversePlotCreator,
    XarrayDC,
)

from utopya.eval.plots.ca import state as ca_state_plot

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator, supports_animation=True)
def combine_pred_and_prey(dm: DataManager, *,
                          uni: UniverseGroup,
                          model_name: str,
                          x: str='predator',
                          y: str='prey',
                          **kwargs):
    """See utopya.plot_funcs.ca.state for docstring.

    This function creates a new entry inside the data, where the information
    of prey and predator positions is overlayed.
    """
    # Get the group that all model datasets are in
    grp = uni['data'][model_name]

    # Compute the combined dataset, but only if it was not already created
    if 'combined' not in grp:
        # Both are binary one-hot encodings; combine combinatorically
        combined = grp['prey'].data + 2 * grp['predator'].data
        # States are:
        #    0: empty
        #    1: prey only
        #    2: predator only
        #    3: predator and prey
        # NOTE This retains the existing dimension names and coordinates

        # Add a new group to the data group
        grp.new_container('combined', Cls=XarrayDC, data=combined)
        # NOTE Cannot store as GridDC because it always performs reshaping

    # Call the actual plotting function, which takes care of the rest, and has
    # the 'combined' entry available.
    ca_state_plot(dm, uni=uni, model_name=model_name, **kwargs)
