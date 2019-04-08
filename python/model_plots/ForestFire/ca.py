"""This module implements customisations for the CA plots of the FFM"""

import numpy as np

import utopya.plot_funcs.ca
from utopya.plot_funcs.ca import is_plot_func, UniversePlotCreator

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator, supports_animation=True)
def state(*args, **kwargs):
    """A wrapper around utopya.plot_funcs.ca.state that adds a preprocessing
    function for clusters.

    See utopya.plot_funcs.ca.state for docstring.
    """
    def cluster_id_mod20(arr):
        arr = arr.astype(float)
        arr = arr.where(arr != 0, np.nan)
        return arr % 20
        
    # Bundle the function into a dict
    pp_funcs = dict(cluster_id=cluster_id_mod20)

    # Call the actual state plot function
    return utopya.plot_funcs.ca.state(*args,
                                      preprocess_funcs=pp_funcs,
                                      **kwargs)
