"""Implements data container classes specialised on Utopia output data.

It is based on the dantro.DataContainer classes, especially its numeric form,
the NumpyDataContainer.
"""

import logging

import dantro as dtr
import dantro.container
import dantro.mixins

# Configure and get logger
log = logging.getLogger(__name__)

# Local constants

# -----------------------------------------------------------------------------

class UtopiaDC(dtr.container.NumpyDataContainer):
    """This is the base class for all numerical data used in Utopia.

    It is based on the NumpyDataContainer provided by dantro and extends it
    with the Hdf5ProxyMixin, allowing to load the data from the Hdf5 file only
    once it becomes necessary.
    """
