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
    attrs_1d = dict(content="grid", grid_shape=(2,3))
    gdc_1d = GridDC(name="data_1d", data=data_1d, attrs=attrs_1d)
    assert(gdc_1d.shape == (2,3))
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

    # Test the case where the grid_size 
    with pytest.raises(ValueError, match="Reshaping failed! "):
        data_1d = np.arange(5)
        gdc_1d = GridDC(name="data_1d", data=data_1d, attrs=attrs_1d)


    ### 2d data ---------------------------------------------------------------
    # Create some test data of the form
    # [[ 0  1  2  3  4  5 ]
    #  [ 6  7  8  9 10 11 ]]
    # Data in columns represents the time and data in rows the grid data
    data_2d = np.arange(12).reshape((2,6))

    # Create a GridDC and assert that the shape is correct
    attrs_2d = dict(content="grid", grid_shape=(2,3))
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
    _, dm = mtc.create_run_load(from_cfg="cfg/griddc_cfg.yml")

    # Assert the type of the state is a GridDC
    assert(type(dm['multiverse'][0]['data/ContDisease/state']) == GridDC)