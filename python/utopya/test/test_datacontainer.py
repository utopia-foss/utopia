"""Test the data container""" 

from pkg_resources import resource_filename

import numpy as np
import pytest

from utopya.datacontainer import GridDC
from utopya.testtools import ModelTest

# -----------------------------------------------------------------------------

def test_griddc():
    """Test the functionality of the GridDC."""
    ### 1d data . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
    # Create some one dimensional test data of the form
    # [ 0  1  2  3  4  5 ]
    # Data in columns represents the time and data in rows the grid data
    data_1d = np.arange(6)

    # Create a GridDC and assert that the shape is correct
    attrs_1d = dict(content="grid", grid_shape=(2, 3), index_order="F")
    gdc_1d = GridDC(name="data_1d", data=data_1d, attrs=attrs_1d)

    assert gdc_1d.shape == (2, 3)
    assert 'x' in gdc_1d.dims
    assert 'y' in gdc_1d.dims

    for i in range(attrs_1d['grid_shape'][0]):
        assert i == gdc_1d.data.coords['x'][i]

    for i in range(attrs_1d['grid_shape'][1]):
        assert i == gdc_1d.data.coords['y'][i]
    
    # Assert that the data is correct and have the form:
    assert (gdc_1d == np.array([[0, 2, 4],
                                [1, 3, 5]])).all()

    # The format string should contains dimension information
    assert 'x: 2' in gdc_1d._format_info()
    assert 'y: 3' in gdc_1d._format_info()

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


    ### 2d data . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
    # Create some test data of the form (2,3,4)
    # Data in columns represents the time and data in rows the grid data
    data_2d = np.arange(2*3*4).reshape((2, 12))

    # Create a GridDC and assert that the shape is correct
    attrs_2d = dict(content="grid", grid_shape=(3, 4), index_order="F")
    gdc_2d = GridDC(name="data_2d", data=data_2d, attrs=attrs_2d)
    
    assert gdc_2d.shape == (2,3,4)
    assert 'time' in gdc_2d.dims
    assert 'x' in gdc_2d.dims
    assert 'y' in gdc_2d.dims
    
    for i in range(attrs_2d['grid_shape'][0]):
        assert i == gdc_2d.data.coords['x'][i]
    
    for i in range(attrs_2d['grid_shape'][1]):
        assert i == gdc_2d.data.coords['y'][i]

    # The format string should contains dimension information
    assert 'time: 2' in gdc_2d._format_info()
    assert 'x: 3' in gdc_2d._format_info()
    assert 'y: 4' in gdc_2d._format_info()

    # Assert that the data is correct and of the form
    expected_2d = np.array([[[ 0,  3,  6,  9],
                             [ 1,  4,  7, 10],
                             [ 2,  5,  8, 11]],
                            [[12, 15, 18, 21],
                             [13, 16, 19, 22],
                             [14, 17, 20, 23]]])
    assert (gdc_2d == expected_2d).all()

    ### 2d data (C-order) . . . . . . . . . . . . . . . . . . . . . . . . . . .
    data_2d_C = np.arange(2*3*4).reshape((2, 12))

    # Create a GridDC and assert that the shape is correct
    attrs_2d_C = dict(content="grid", grid_shape=(3, 4), index_order="C")
    gdc_2d_C = GridDC(name="data_2d_C", data=data_2d_C, attrs=attrs_2d_C)
    
    assert gdc_2d_C.shape == (2,3,4)
    expected_2d_C = np.array([[[ 0,  1,  2,  3],
                               [ 4,  5,  6,  7],
                               [ 8,  9, 10, 11]],
                              [[12, 13, 14, 15],
                               [16, 17, 18, 19],
                               [20, 21, 22, 23]]])
    assert (gdc_2d_C == expected_2d_C).all()


def test_griddc_integration():
    """Integration test for the GridDC using the ContDisease model"""

    # Configure the ModelTest class for ContDisease
    mtc = ModelTest("ContDisease", test_file=__file__)

    # Create and run a multiverse and load the data
    _, dm = mtc.create_run_load(from_cfg="cfg/griddc_cfg.yml",
                                parameter_space=dict(num_steps=3))

    # Get the data
    grid_data = dm['multiverse'][0]['data/ContDisease/kind']
    print("Grid data: ", grid_data)

    # Assert the type of the state is a GridDC
    assert isinstance(grid_data, GridDC)

    # See that grid shape, extent etc. is carried over and matches
    assert 'grid_shape' in grid_data.attrs
    assert 'space_extent' in grid_data.attrs

    # By default, this should be a proxy
    assert grid_data.data_is_proxy
    assert 'proxy' in grid_data._format_info()

    # Nevertheless, the properties should show the target values
    print("proxy:\n", grid_data._data)
    assert grid_data.ndim == 3
    assert grid_data.shape == (4,) + tuple(grid_data.attrs['grid_shape'])
    assert grid_data.grid_shape == tuple(grid_data.attrs['grid_shape'])

    # And the format string should show dimension information
    assert 'time: 4' in grid_data._format_info()
    assert 'x: 8' in grid_data._format_info()
    assert 'y: 4' in grid_data._format_info()
    assert 'ids' not in grid_data._format_info()

    # ... without resolving
    assert grid_data.data_is_proxy

    # Now resolve the proxy and check the properties again
    grid_data.data
    assert not grid_data.data_is_proxy
    assert 'proxy' not in grid_data._format_info()

    print("resolved:\n", grid_data._data)
    assert grid_data.ndim == 3
    assert grid_data.shape == (4,) + tuple(grid_data.attrs['grid_shape'])
    assert grid_data.grid_shape == tuple(grid_data.attrs['grid_shape'])
    
    assert 'time: 4' in grid_data._format_info()
    assert 'x: 8' in grid_data._format_info()
    assert 'y: 4' in grid_data._format_info()
    assert 'ids' not in grid_data._format_info()

    # Check that coordinate information is available
    assert (grid_data.coords['time'] == [0, 1, 2, 3]).all()
    assert np.isclose(grid_data.coords['x'],
                      np.linspace(0., 2., 8, False) + 2./(2*8)).all()
    assert np.isclose(grid_data.coords['y'],
                      np.linspace(0., 1., 4, False) + 1./(2*4)).all()

    # Try re-instating the proxy
    grid_data.reinstate_proxy()
    assert grid_data.data_is_proxy
    assert 'proxy' in grid_data._format_info()

    # And resolve it again
    grid_data.data
    assert not grid_data.data_is_proxy
    assert 'proxy' not in grid_data._format_info()
    
    print("reinstated:\n", grid_data._data)
    assert grid_data.ndim == 3
    assert len(grid_data['time']) == 4

    assert grid_data.shape == (4,) + tuple(grid_data.attrs['grid_shape'])
    assert grid_data.grid_shape == tuple(grid_data.attrs['grid_shape'])

    # Make sure the grid shape is a multiple of the space extent
    # Resolution 4 set in griddc_cfg.yml
    assert (grid_data.grid_shape == 4 * grid_data.space_extent).all()


    # Test some further things related to grid shape . . . . . . . . . . . . .
    cfg = dm['multiverse'][0]['cfg/ContDisease']
    space_extent = tuple(cfg['space']['extent'])
    resolution = cfg['cell_manager']['grid']['resolution']

    # Calculate the expected grid shape
    expected_grid_shape = (int(space_extent[0] * resolution),
                           int(space_extent[1] * resolution))

    assert grid_data.shape == (4, *expected_grid_shape)
    assert len(grid_data['x']) == expected_grid_shape[0]
    assert len(grid_data['y']) == expected_grid_shape[1]


    # Test coordinates . . . . . . . . . . . . . . . . . . . . . . . . . . . . 
    _, dm = mtc.create_run_load(from_cfg="cfg/griddc_cfg.yml",
                                parameter_space=dict(num_steps=11,
                                                     write_every=2))
    grid_data = dm['multiverse'][0]['data/ContDisease/kind']
    print("Grid data: ", grid_data)
    print("Attributes: ", dict(grid_data.attrs))

    # Assert the type of the state is a GridDC and that it's loaded as proxy
    assert isinstance(grid_data, GridDC)
    assert grid_data.data_is_proxy

    # Check the time coordinates
    assert (grid_data.coords['time'] == [0, 2, 4, 6, 8, 10]).all()
