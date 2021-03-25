"""Tests dynamics of the SandPile model"""

import numpy as np

import pytest

import xarray as xr

from utopya.testtools import ModelTest

# Configure the ModelTest class for SandPile
mtc = ModelTest("SandPile", test_file=__file__)

# Fixtures --------------------------------------------------------------------
# Define fixtures

def model_cfg(**params) -> dict:
    """Helper method to create a model configuration"""
    return dict(parameter_space=dict(SandPile=dict(**params)))

def cm_cfg(**params) -> dict:
    """Helper method to create a cell_manager configuration"""
    return model_cfg(cell_manager=dict(**params))

# Tests -----------------------------------------------------------------------

def test_dynamics():
    """
    Test the dynamic behavior of the model, i.e. that all methods works as 
    intended and the datasets are filled correctly. 
    """

    # Create a multiverse and data manager
    mv, dm = mtc.create_run_load(from_cfg="dynamics.yml")
    for uni in dm['multiverse'].values():
        data = uni['data/SandPile']
        
        # Get configurations
        uni_cfg = uni['cfg']
        
        # Get model parameter from config
        num_steps = uni_cfg['num_steps'] + 1
        critical_slope = uni_cfg['SandPile']['critical_slope']
        resolution = uni_cfg['SandPile']['cell_manager']['grid']['resolution']

        # Calculate coordinate for dataset comparision
        time_coords = range(0, num_steps)
        space_coords = np.linspace(1. / (2 * resolution), 
                (1. - 1. / (2 * resolution)), num=resolution
            )

        # Calculate test datasets

        # For this test all cells are in avalanche and topple. With the 
        # critical slope at 4 all slopes are between 0 and 4. In the following
        # slope dataset is tested against one with all datapoints equal to 
        # 2 and with a tolerance also of 2. 
        test_slope_data = xr.DataArray(
                num_steps * [resolution * [resolution * [critical_slope / 2]]], 
                dims=["time", "x", "y"],
                coords=dict(time=time_coords, x=space_coords, y=space_coords)
            )
            
        # All cells should be in avalanche and so all datapoints equal to one. 
        test_avalanche_data = xr.DataArray(
                num_steps * [resolution * [resolution * [0.5]]], 
                dims=["time", "x", "y"],
                coords=dict(time=time_coords, x=space_coords, y=space_coords)
            )
            
        # The number of cells in avalanche should be equal to the number
        # of cells.     
        test_avalanche_size_data = xr.DataArray(
                num_steps * [resolution**2 / 2], 
                dims=["time"], 
                coords=dict(time=time_coords)
            )

        
        # Performing the tests:
        # All slope values should be from [0, critical_slope]        
        xr.testing.assert_allclose(data['slope'].data, 
                test_slope_data, atol=critical_slope / 2.)
        # Cells should only contain values 0 or 1
        xr.testing.assert_allclose(data['avalanche'].data, 
                test_avalanche_data, atol=0.5)
        # Datapoints should be from [0, num_cells = resolution**2]
        xr.testing.assert_allclose(data['avalanche_size'].data, 
                test_avalanche_size_data, atol=resolution**2 / 2.)