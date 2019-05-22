"""Implements a class that manages data written out by Utopia models.

It is based on the dantro.DataManager class and the containers specialised for
Utopia data.
"""

import logging

import dantro as dtr
import dantro.data_mngr
from dantro.data_loaders import YamlLoaderMixin, Hdf5LoaderMixin

import utopya.datacontainer as udc
import utopya.datagroup as udg

# Configure and get logger
log = logging.getLogger(__name__)

# Local constants

# -----------------------------------------------------------------------------

class DataManager(Hdf5LoaderMixin, YamlLoaderMixin, dtr.data_mngr.DataManager):
    """This class manages the data that is written out by Utopia simulations.

    It is based on the dantro.DataManager class and adds the functionality for
    specific loader functions that are needed in Utopia: Hdf5 and Yaml.
    """

    # Register known group types
    _DATA_GROUP_CLASSES = dict(MultiverseGroup=udg.MultiverseGroup,
                               NetworkGroup=udg.NetworkGroup)

    # Tell the HDF5 loader which container class to use
    _HDF5_DSET_DEFAULT_CLS = udc.XarrayDC

    # The name of the attribute to read for the mapping
    _HDF5_MAP_FROM_ATTR = "content"

    # The mapping of different content values to a data group type
    _HDF5_GROUP_MAP = dict(network=udg.NetworkGroup,
                           time_series=udg.TimeSeriesGroup)

    # The mapping of different content values to a data container types
    _HDF5_DSET_MAP = dict(grid=udc.GridDC,
                          unlabelled_data=udc.NumpyDC,
                          labelled_data=udc.XarrayDC)
