"""Implements a class that manages data written out by Utopia models.

It is based on the dantro.DataManager class and the containers specialised for
Utopia data.
"""

import logging

import dantro as dtr
import dantro.data_mngr
import dantro.data_loaders

import utopya.datacontainer as udc
import utopya.datagroup as udg

# Configure and get logger
log = logging.getLogger(__name__)

# Local constants
def _condense_thresh_func(*, level, num_items, total_item_count) -> int:
    """Dynamically computes the condensation threshold for the current
    element.
    """
    # For high item counts, always condense
    if total_item_count > 100: # NOTE This is along one recursion branch!
        return 7 # shows first three and last three

    # Now, distinguish by level
    if level == 1:
        # Level of the universes 0, ..., N
        if num_items > 3:
            return 3

    elif level >= 4:
        # Level of the model data, e.g. multiverse/0/data/dummy/...
        if num_items > 15:
            return 5

    # All other cases: do not condense
    return None

# -----------------------------------------------------------------------------

class DataManager(dtr.data_loaders.AllAvailableLoadersMixin,
                  dtr.data_mngr.DataManager):
    """This class manages the data that is written out by Utopia simulations.

    It is based on the dantro.DataManager class and adds the functionality for
    specific loader functions that are needed in Utopia: Hdf5 and Yaml.

    Furthermore, to enable file caching via the DAG framework, all available
    data loaders are included here.
    """

    # Register known group types
    _DATA_GROUP_CLASSES = dict(MultiverseGroup=udg.MultiverseGroup,
                               GraphGroup=udg.GraphGroup)

    # Tell the HDF5 loader which container class to use
    _HDF5_DSET_DEFAULT_CLS = udc.XarrayDC

    # The name of the attribute to read for the mapping
    _HDF5_MAP_FROM_ATTR = "content"

    # The mapping of different content values to a data group type
    _HDF5_GROUP_MAP = dict(network=udg.GraphGroup,
                           graph=udg.GraphGroup,
                           time_series=udg.TimeSeriesGroup,
                           time_series_heterogeneous=udg.HeterogeneousTimeSeriesGroup)

    # The mapping of different content values to a data container types
    _HDF5_DSET_MAP = dict(grid=udc.GridDC,
                          unlabelled_data=udc.NumpyDC,
                          labelled_data=udc.XarrayDC,
                          array_of_yaml_strings=udc.XarrayYamlDC)

    # Condensed tree representation: maximum level
    _COND_TREE_MAX_LEVEL = 10

    # Condensed tree representation: threshold parameter
    _COND_TREE_CONDENSE_THRESH = lambda _, **kws: _condense_thresh_func(**kws)

    # Where the tree cache file is to be stored; overwritten by config entry
    _DEFAULT_TREE_CACHE_PATH = "data/.tree_cache.d3"
