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


class XarrayDC(Hdf5ProxyMixin, XrDataContainer):
    """This is the base class for xarray data containers used in Utopia.

    It is based on the XrDataContainer provided by dantro. As of now, it has
    no proxy support, but will gain it once available on dantro side.
    """
    # Configure proxy support .................................................
    # Which type to resolve the proxy to. None defaults to np.ndarray
    PROXY_RESOLVE_ASTYPE = None

    # Whether to retain a proxy after resolving it; allows .reinstate_proxy()
    PROXY_RETAIN = True
    
    # Which action to take if reinstating a proxy was not possible
    PROXY_REINSTATE_FAIL_ACTION = 'log_warning'


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
    _XRC_INHERIT_CONTAINER_ATTRIBUTES = False

    # Whether to use strict attribute checking; throws errors if there are
    # container attributes available that match the prefix but don't match a
    # valid dimension name. Can be disabled for speed improvements
    _XRC_STRICT_ATTR_CHECKING = True


# -----------------------------------------------------------------------------

class GridDC(XarrayDC):
    """This is the base class for all grid data used in Utopia.

    It is based on the XarrayDC and reshapes the data to the grid shape.
    The last dimension is assumed to be the dimension that goes along the 
    grid cell IDs.
    """
    # Define class variables to allow specializing behaviour ..................
    # The attribute to read the desired grid shape from
    _GDC_grid_shape_attr = 'grid_shape'

    # The attribute to read the space extent from
    _GDC_space_extent_attr = 'space_extent'

    # .........................................................................

    def __init__(self, *, name: str, data: Union[np.ndarray, xr.DataArray],
                 **dc_kwargs):
        """Initialize a GridDC which represents grid-like data.

        Given the container attribute for _GDC_grid_shape_attr, this container
        takes care to reshape the underlying data such that it represents that
        grid, even if it is saved in another shape.
        
        Args:
            name (str): The name of the data container
            data (np.ndarray): The not yet reshaped data. If this is 1D, it is
                assumed that there is no time dimension. If it is 2D, it is
                assumed to be (time, cell ids).
            **kwargs: Further initialization kwargs, e.g. `attrs` ...
        """
        # To prohibit proxy resolution in the below __init__, need a flag
        self._shapes_cached = False

        # Call the __init__ function of the base class
        super().__init__(name=name, data=data, **dc_kwargs)

        # Data is an xr.DataArray now or might be a proxy
        # Store the old and determine the new shape of the data, needed for
        # proxy property support
        self._data_shape = self.shape

        try:
            self._grid_shape = tuple(self.attrs[self._GDC_grid_shape_attr])

        except KeyError as err:
            raise ValueError("Missing attribute '{}' in {} to extract the "
                             "desired grid shape from! Available: {}"
                             "".format(self._GDC_grid_shape_attr, self.logstr,
                                       ", ".join(self.attrs.keys()))
                             ) from err
        
        # To get the new shape, add up the old data shape without the grid 
        # dimension and the expected grid shape. 
        # NOTE It is assumed that the _last_ dimension goes along cell IDs and
        #      the reshaped x-y data dimensions are thus appended.
        self._new_shape = tuple(self._data_shape[:-1] + self._grid_shape)

        # Have all shapes cached now; set the flag such that the properties
        # use those values instead of the proxy's
        self._shapes_cached = True

        # Do the reshaping and everything only if it is not a proxy
        if not self.data_is_proxy:
            self._data = self._reshape_data()
        # otherwise: postpone reshaping until proxy gets resolved

    def _postprocess_proxy_resolution(self):
        """Invoked from ``Hdf5ProxyMixin`` after a proxy was resolved, this
        takes care to apply the reshaping operation onto the underlying data.
        """
        # First let the XarrayDC do its thing
        super()._postprocess_proxy_resolution()

        # Now do the reshaping
        self._data = self._reshape_data()


    # Properties ..............................................................

    @property
    def grid_shape(self) -> tuple:
        """The shape of the grid"""
        if self._shapes_cached:
            return self.shape[1:]

    @property
    def space_extent(self) -> tuple:
        """The space's extent this grid is representing, read from attrs"""
        return self.get(self._GDC_space_extent_attr)

    @property
    def shape(self) -> tuple:
        """Returns shape, proxy-aware

        This is an overload of the property in Hdf5ProxyMixin which takes care
        that not the actual underlying proxy data shape is returned but
        whatever the container's shape is to be after reshaping.
        """
        if self.data_is_proxy:
            # Might not be set yet, i.e. during call to super().__init__
            if self._shapes_cached:
                return self._new_shape
            return self.proxy.shape
        return self.data.shape

    @property
    def ndim(self) -> int:
        """Returns ndim, proxy-aware

        This is an overload of the property in Hdf5ProxyMixin which takes care
        that not the actual underlying proxy data ndim is returned but
        whatever the container's ndim is to be after reshaping.
        """
        if self.data_is_proxy:
            return len(self.shape)
        return self.data.ndim


    # Reshaping ...............................................................

    def _reshape_data(self) -> xr.DataArray:
        """Looks at the current data shape and container attributes to
        reshape the data such that it represents a grid.
        """
        # New shape was already calculated, data shape is still the same
        data_shape = self._data_shape
        new_shape = self._new_shape
        grid_shape = self._grid_shape

        # Determine new dimension names
        if len(data_shape) == 2:
            new_dims = ('time',) + ('x', 'y', 'z')[:len(grid_shape)]
        
        elif len(data_shape) == 1:
            new_dims = ('x', 'y', 'z')[:len(grid_shape)]

        else:
            raise ValueError("Can only reshape from 1D or 2D data, got {}!"
                             "".format(data_shape))

        # Determine new coordinates
        new_coords = dict()

        # Carry over time, if possible
        if 'time' in new_coords and 'time' in self.coords:
            new_coords['time'] = self.coords['time']

        # If there is a space extent attribute available, use that to set the
        # spatial coordinates. Otherwise, assign the trivial coordinates.
        extent = self.attrs.get(self._GDC_space_extent_attr)
        if extent is None:
            # Trivial coordinate generator
            coord_gen = lambda n, _: range(n)
            extent = (None,) * len(grid_shape)  # dummy for the iterator below

        else:
            # Actual cell position coordinate generator
            coord_gen = lambda n, l: np.linspace(0., l, n, False) + (.5*l/n)

        for n, l, dim_name in zip(grid_shape, extent, ('x', 'y', 'z')):
            new_coords[dim_name] = coord_gen(n, l)

        # Reshape data now
        log.debug("Reshaping data of shape %s to %s to match given grid "
                  "shape %s ...", data_shape, new_shape, grid_shape)

        try:
            data = np.reshape(self._data.values, new_shape)

        except ValueError as err:
            raise ValueError("Reshaping failed! This is probably due to a "
                             "mismatch between the written dataset attribute " 
                             "for the grid shape ('{}': {}, configured by " 
                             "class variable `_GDC_grid_shape_attr`) and the "
                             "actual shape {} of the written data."
                             "".format(self._GDC_grid_shape_attr, 
                                       grid_shape, data_shape)
                             ) from err

        # All succeeded. Overwrite the existing _data attribute with a new
        # xr.DataArray, to which all the information is carried over.
        new_data = xr.DataArray(name=self.name, data=data,
                                dims=new_dims, coords=new_coords,
                                attrs={k: v for k, v in self.attrs.items()
                                       if k in (self._GDC_space_extent_attr,
                                                self._GDC_grid_shape_attr)})

        log.debug("Successfully reshaped data to represent a spatial grid.")

        return new_data

