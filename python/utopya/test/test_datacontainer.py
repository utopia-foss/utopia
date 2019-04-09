"""Test the data container""" 

from pkg_resources import resource_filename

import numpy as np
import pytest

from utopya.datacontainer import GridDC
from utopya.testtools import ModelTest


def test_griddc() -> None:
    """Test the functionality of the GridDC."""
    ### 1d data ---------------------------------------------------------------
    # Create some one dimensional test data of the form
    # [ 0  1  2  3  4  5 ]
    # Data in columns represents the time and data in rows the grid data
    data_1d = np.arange(6)

    # Create a GridDC and assert that the shape is correct
    attrs_1d = dict(content="grid", grid_shape=(2, 3))
    gdc_1d = GridDC(name="data_1d", data=data_1d, attrs=attrs_1d)

    assert gdc_1d.shape == (2,3)
    assert 'x' in gdc_1d.dims
    assert 'y' in gdc_1d.dims

    for i in range(attrs_1d['grid_shape'][0]):
        assert i == gdc_1d.data.coords['x'][i]

    for i in range(attrs_1d['grid_shape'][1]):
        assert i == gdc_1d.data.coords['y'][i]
    
    # Assert that the data is correct and have the form
    # [[ 0  1  2 ]
    #  [ 3  4  5 ]]
    # To check this, iterate through all elements in the data and check 
    # that each value is at the expected location. 
    for i in range(6):
        assert(gdc_1d[i//3][i%3] == i)

    # Test the case where the grid_size is bad
    with pytest.raises(ValueError, match="Reshaping failed! "):
        GridDC(name="bad_data", data=np.arange(5), attrs=attrs_1d)

    # Test case where the data is of too high dimensionality
    with pytest.raises(ValueError, match="Can only reshape from 1D or 2D"):
        GridDC(name="bad_data",
               data=np.zeros((2,3,4)), attrs=dict(grid_shape=(2,3)))
    
    # Test missing attribute
    with pytest.raises(ValueError, match="Missing attribute 'grid_shape'"):
        GridDC(name="missing_attr",
               data=np.zeros((2,3)), attrs=dict())


    ### 2d data ---------------------------------------------------------------
    # Create some test data of the form
    # [[ 0  1  2  3  4  5 ]
    #  [ 6  7  8  9 10 11 ]]
    # Data in columns represents the time and data in rows the grid data
    data_2d = np.arange(12).reshape((2, 6))

    # Create a GridDC and assert that the shape is correct
    attrs_2d = dict(content="grid", grid_shape=(2, 3))
    gdc_2d = GridDC(name="data_2d", data=data_2d, attrs=attrs_2d)
    assert(gdc_2d.shape == (2,2,3))
    assert 'time' in gdc_2d.dims
    assert 'x' in gdc_2d.dims
    assert 'y' in gdc_2d.dims
    for i in range(attrs_2d['grid_shape'][0]):
        assert i == gdc_2d.data.coords['x'][i]
    for i in range(attrs_2d['grid_shape'][1]):
        assert i == gdc_2d.data.coords['y'][i]

    # Assert that the data is correct and of the form
    # [[[ 0  1  2 ]
    #  [ 3  4  5 ]]
    # [[ 6  7  8 ]
    #  [ 9 10 11 ]]]
    # To check this, iterate through all elements in the data and check 
    # that each value is at the expected location.
    for i in range(12):
        if (i//6 == 0 ):
            assert(gdc_2d[0][i//3][i%3] == i)
        else:
            tmp = i - 6
            assert(gdc_2d[1][tmp//3][tmp%3] == i)

def test_griddc_integration():
    """Integration test for the GridDC."""

    # Configure the ModelTest class for ContDisease
    mtc = ModelTest("ContDisease", test_file=__file__)

    # Create and run a multiverse and load the data
    _, dm = mtc.create_run_load(from_cfg="cfg/griddc_cfg.yml",
                                parameter_space=dict(num_steps=3))

    # Get the data
    grid_data = dm['multiverse'][0]['data/ContDisease/state']

    # Assert the type of the state is a GridDC
    assert isinstance(grid_data, GridDC)

    # See that grid shape, extent etc. is carried over and matches
    assert 'grid_shape' in grid_data.attrs
    assert 'space_extent' in grid_data.attrs

    # By default, this should be a proxy
    assert grid_data.data_is_proxy

    # Nevertheless, the properties should show the target values
    print("proxy:\n", grid_data._data)
    assert grid_data.ndim == 3
    assert grid_data.shape == (4,) + tuple(grid_data.attrs['grid_shape'])
    assert grid_data.grid_shape == tuple(grid_data.attrs['grid_shape'])

    # ... without resolving
    assert grid_data.data_is_proxy

    # Now resolve the proxy and check the properties again
    grid_data.data
    assert not grid_data.data_is_proxy

    print("resolved:\n", grid_data._data)
    assert grid_data.ndim == 3
    assert grid_data.shape == (4,) + tuple(grid_data.attrs['grid_shape'])
    assert grid_data.grid_shape == tuple(grid_data.attrs['grid_shape'])

    # Try re-instating the proxy
    grid_data.reinstate_proxy()
    assert grid_data.data_is_proxy

    # And resolve it again
    grid_data.data
    assert not grid_data.data_is_proxy
    
    print("reinstated:\n", grid_data._data)
    assert grid_data.ndim == 3
    assert grid_data.shape == (4,) + tuple(grid_data.attrs['grid_shape'])
    assert grid_data.grid_shape == tuple(grid_data.attrs['grid_shape'])


    # Make sure the grid shape is a multiple of the space extent
    # Resolution 4 set in griddc_cfg.yml
    assert (grid_data.grid_shape == 4 * grid_data.space_extent).all()
