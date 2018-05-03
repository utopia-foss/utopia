"""Implements a class that manages data written out by Utopia models.

It is based on the dantro.DataManager class and the containers specialised for
Utopia data.
"""

import logging

import dantro as dtr
import dantro.data_mngr
from dantro.data_loaders import YamlLoaderMixin, Hdf5LoaderMixin

import utopya.datacontainer as udc

# Configure and get logger
log = logging.getLogger(__name__)

# Local constants

# -----------------------------------------------------------------------------

class DataManager(Hdf5LoaderMixin, YamlLoaderMixin, dtr.data_mngr.DataManager):
    """This class manages the data that is written out by Utopia simulations.

    It is based on the dantro.DataManager class and adds the functionality for
    specific loader functions that are needed in Utopia: Hdf5 and Yaml.
    """

    # Tell the HDF5 loader which container class to use
    _HDF5_DSET_DEFAULT_CLS = udc.UtopiaDC
