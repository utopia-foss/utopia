"""Extends dantro's basic plotting functions to work with Utopia data"""

import dantro.plot_creators.ext_funcs.basic as dtr_basic

from ._setup import *

# Local constants
log = logging.getLogger(__name__)


# -----------------------------------------------------------------------------

def lineplot(dm: DataManager, *, uni: UniverseGroup, **kwargs):
    """Performs a simple lineplot of a specific universe.
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        uni (UniverseGroup): The data for this universe
        **kwargs: Passed on to the dantro basic plotting function
    
    Returns:
        Whatever the dantro lineplot returns
    """
    # Relay to the dantro function
    return dtr_basic.lineplot(dm=uni['data'], **kwargs)
