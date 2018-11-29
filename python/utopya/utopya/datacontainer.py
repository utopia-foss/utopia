"""Implements data container classes specialised on Utopia output data.

It is based on the dantro.DataContainer classes, especially its numeric form,
the NumpyDataContainer.
"""

import logging

from dantro.containers import NumpyDataContainer
from dantro.mixins import Hdf5ProxyMixin

# Configure and get logger
log = logging.getLogger(__name__)

# Local constants

# -----------------------------------------------------------------------------

class NumpyDC(Hdf5ProxyMixin, NumpyDataContainer):
    """This is the base class for all numerical data used in Utopia.

    It is based on the NumpyDataContainer provided by dantro and extends it
    with the Hdf5ProxyMixin, allowing to load the data from the Hdf5 file only
    once it becomes necessary.
    """
