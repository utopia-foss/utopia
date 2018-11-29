"""Test the data container""" 

import numpy as np
import pytest

from utopya.datacontainer import GridDC

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
    
    # Assert that the data is correct and have the form
    # [[ 0  1  2 ]
    #  [ 3  4  5 ]]
    for i in range(6):
        assert(gdc_1d[i//3][i%3] == i)

    # Test the case where the grid_size 
    with pytest.raises(ValueError, match="Cannot reshape array of size"):
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

    # Assert that the data is correct and of the form
    # [[[ 0  1  2 ]
    #  [ 3  4  5 ]]
    # [[ 6  7  8 ]
    #  [ 9 10 11 ]]]
    for i in range(12):
        if (i//6 == 0 ):
            assert(gdc_2d[0][i//3][i%3] == i)
        else:
            tmp = i - 6
            assert(gdc_2d[1][tmp//3][tmp%3] == i)


    ### 3d data ---------------------------------------------------------------    
    # Create some test data of the form
    # [[[ 0  1  2  3  4  5 ]
    #  [ 6  7  8  9 10 11 ]]
    # [[ 0  1  2  3  4  5 ]
    #  [ 6  7  8  9 10 11 ]]]
    # Data in columns represents the time and data in rows the grid data
    data_3d = np.arange(24).reshape((2,2,6))

    # Create a GridDC and assert that the shape is correct
    attrs_3d = dict(content="grid", grid_shape=(2,3))
    gdc_3d = GridDC(name="data_3d", data=data_3d, attrs=attrs_3d)
    assert(gdc_3d.shape == (2,2,2,3))
