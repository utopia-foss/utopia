"""Implements data container classes specialised on Utopia output data.

It is based on the dantro.DataContainer classes, especially its numeric form,
the NumpyDataContainer.
"""

import logging
from typing import Union, List, Dict
from operator import mul
from functools import reduce

import numpy as np
import xarray as xr

from dantro.containers import NumpyDataContainer, XrDataContainer
from dantro.mixins import Hdf5ProxyMixin

# Configure and get logger
log = logging.getLogger(__name__)

# Local constants

# -----------------------------------------------------------------------------

class NumpyDC(Hdf5ProxyMixin, NumpyDataContainer):
    """This is the base class for numpy data containers used in Utopia.

    It is based on the NumpyDataContainer provided by dantro and extends it
    with the Hdf5ProxyMixin, allowing to load the data from the Hdf5 file only
    once it becomes necessary.
    """


class XarrayDC(XrDataContainer):
    """This is the base class for xarray data containers used in Utopia.

    It is based on the XrDataContainer provided by dantro. As of now, it has
    no proxy support, but will gain it once available on dantro side.
    """
    # Specialize XrDataContainer for Utopia ...................................
    # Define as class variable the name of the attribute that determines the
    # dimensions of the xarray.DataArray
    _XRC_DIMS_ATTR = 'dim_names'
    
    # Attributes prefixed with this string can be used to set names for
    # specific dimensions. The prefix should be followed by an integer-parsable
    # string, e.g. `dim_name__0` would be the dimension name for the 0th dim.
    _XRC_DIM_NAME_PREFIX = 'dim_name__'

    # Attributes prefixed with this string determine the coordinate values for
    # a specific dimension. The prefix should be followed by the _name_ of the
    # dimension, e.g. `coords__time`. The values are interpreted according to
    # the default coordinate mode or, if given, the coords_mode__* attribute
    _XRC_COORDS_ATTR_PREFIX = 'coords__'

    # The default mode by which coordinates are interpreted. Available modes:
    #   - `list`            List of coordinate values
    #   - `range`           Range expression (start, stop, step)
    #   - `start_and_step`  Range (start, <deduced>, step) with auto-deduced
    #                       stop value from length of dataset
    _XRC_COORDS_MODE_DEFAULT = 'list'

    # Prefix for the coordinate mode if a custom mode is to be used. To, e.g.,
    # use mode 'start_and_step' for time dimension, set the coords_mode__time
    # attribute to value 'start_and_step'
    _XRC_COORDS_MODE_ATTR_PREFIX = 'coords_mode__'

    # Whether to inherit the other container attributes
    _XRC_INHERIT_CONTAINER_ATTRIBUTES = True

    # Whether to use strict attribute checking; throws errors if there are
    # container attributes available that match the prefix but don't match a
    # valid dimension name. Can be disabled for speed improvements
    _XRC_STRICT_ATTR_CHECKING = True


class GridDC(XarrayDC):
    """This is the base class for all grid data used in Utopia.

    It is based on the NumpyDC and reshapes the data to the grid shape.
    The last dimension is assumed to be the dimension that goes along the 
    grid cell IDs.
    """
    
    # Define as class variable the attribute that determines the shape of the data.
    _GDC_attrs_grid_shape = 'grid_shape'
    _GDC_attrs_dim_names = 'dims'

    def __init__(self, *, name: str, data: Union[np.ndarray, xr.DataArray], **dc_kwargs):
        """Initialize a GridDC.
        
        Args:
            name (str): The name of the data container
            data (np.ndarray): The not yet reshaped data
            **kwargs: Further initialization kwargs, e.g. `attrs` ...
        """
        # Call the __init__ function of the base class
        super().__init__(name=name, data=data, **dc_kwargs)

        name = self.name
        values = self.values
        dims = self.dims
        coords = self.coords
        attrs = self.attrs

        # Get the shape of the internal data and the desired shape of the grid
        data_shape = self.shape
        grid_shape = tuple(self.attrs[self._GDC_attrs_grid_shape])
        
        # To get the new shape add up the old data shape without the grid 
        # dimension and the expected grid shape. 
        # NOTE It is assumed that the _last_ data dimension goes along grid ID's!
        new_shape = data_shape[:-1] + grid_shape

        if (len(new_shape) == 2):
            dims = ('x', 'y')
        elif (len(new_shape) == 3):
            dims = ('time', 'x', 'y')
        else:
            raise NotImplementedError("No method implemented to initialize "
                                      "GridDC with more than 2 spatial "
                                      "dimensions.")

        if not coords:
            coords = dict()
        for k, v in coords:
            if k is not 'time':
                coords.pop(k)
        coords['x'] = range(grid_shape[0])
        coords['y'] = range(grid_shape[1])

        # Check that the number of total elements before reshaping equals the
        # number of elements after reshaping in the data
        log.debug("Reshaping data of shape %s to %s "
                  "to match given grid shape %s ...", 
                  data_shape, new_shape, grid_shape)
        try:
            # Reshape the data
            data = np.reshape(self.values, new_shape)

        except ValueError as err:
            raise ValueError("Reshaping failed! This is probably due to a "
                             "mismatch between the written dataset attribute " 
                             "for the grid shape ('{}': {}, configured by " 
                             "class variable `_GDC_attrs_grid_shape`) and the "
                             "actual shape {} of the written data."
                             "".format( self._GDC_attrs_grid_shape, 
                                        grid_shape, data_shape)) from err

        self._data = xr.DataArray(name=self.name, data=data, dims=dims, 
                                  coords=coords, attrs=self.attrs)

