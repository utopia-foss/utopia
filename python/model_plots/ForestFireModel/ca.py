"""This module implements customisations for the CA plots of the FFM"""

import utopya.plot_funcs.ca

# -----------------------------------------------------------------------------

def state_anim(*args, **kwargs) -> None:
    """See utopya.plot_funcs.ca.state_anim for docstring

    This function merely wraps that function and provides the preprocess_funcs
    that are needed for the FFM
    """
    def cluster_id_mod20(arr):
        return arr % 20;
        # TODO @jweninger check if this does, what you want!
        
    # Bundle the functions into a dict, then call the actual state_anim
    pp_funcs = dict(cluster_id=cluster_id_mod20)

    return utopya.plot_funcs.ca.state_anim(*args,
                                           preprocess_funcs=pp_funcs,
                                           **kwargs)
