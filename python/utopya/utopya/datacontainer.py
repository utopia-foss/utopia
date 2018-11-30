"""Implements data container classes specialised on Utopia output data.

It is based on the dantro.DataContainer classes, especially its numeric form,
the NumpyDataContainer.
"""

import logging
from operator import mul
from functools import reduce

import numpy as np

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


class GridDC(NumpyDC):
    """This is the base class for all grid data used in Utopia.

    It is based on the NumpyDC and reshapes the data to the grid shape.
    The last dimension is assumed to be the dimension that goes along the 
    grid cell IDs.
    """
    
    # Define as class variable the attribute that determines the shape of the data.
    _GDC_attrs_grid_shape = 'grid_shape'

    def __init__(self, *, name: str, data: np.ndarray, **dc_kwargs):
        """Initialize a GridDC.
        
        Args:
            name (str): The name of the data container
            data (np.ndarray): The not yet reshaped data
            **kwargs: Further initialization kwargs, e.g. `attrs` ...
        """
        # Call the __init__ function of the base class
        super().__init__(name=name, data=data, **dc_kwargs)

        # Get the shape of the internal data and the desired shape of the grid
        data_shape = self.shape
        grid_shape = tuple(self.attrs[self._GDC_attrs_grid_shape])
        
        # To get the new shape add up the old data shape without the grid 
        # dimension and the expected grid shape. 
        # NOTE It is assumed that the _last_ data dimension goes along grid ID's!
        new_shape = data_shape[:-1] + grid_shape

        # Check that the number of total elements before reshaping equals the
        # number of elements after reshaping in the data
        log.debug("Reshaping data of shape %s to %s "
                  "to match given grid shape %s ...", 
                  data_shape, new_shape, grid_shape)
        try:
            # Reshape the data 
            self._data = np.reshape(self.data, new_shape)

        except ValueError as err:
            raise ValueError("Reshaping failed! This is probably due to a "
                             "mismatch between the written dataset attribute " 
                             "for the grid shape ('{}': {}, configured by " 
                             "class variable `_GDC_attrs_grid_shape`) and the "
                             "actual shape {} of the written data."
                             "".format( self._GDC_attrs_grid_shape, 
                                        grid_shape, data_shape)) from err

