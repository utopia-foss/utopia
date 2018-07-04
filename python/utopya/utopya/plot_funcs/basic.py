"""Extends dantro's basic plotting functions to work with Utopia data"""

import dantro.plot_creators.ext_funcs.basic as dtr_basic

from ._setup import *


# Local constants
log = logging.getLogger(__name__)


# -----------------------------------------------------------------------------

def lineplot(dm: DataManager, *, uni: str, **kwargs):
    """Performs a simple lineplot of a specific universe.
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (str): The number of the universe to plot
        **kwargs: Passed on to the dantro basic plotting function
    """
    # Relay to the dantro function
    return dtr_basic.lineplot(dm=dm['uni'][str(uni)]['data'], **kwargs)
