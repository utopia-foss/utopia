"""Tests initialization of the SandPile model"""

import numpy as np

import pytest

import xarray as xr

from utopya.parameter import ValidationError

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
def test_initial_datasets():
    """Test to check for the correct initialization of the datasets"""

    mv, dm = mtc.create_run_load(from_cfg="initial_datasets.yml", 
            perform_sweep=True
        )

    for uni in dm['multiverse'].values():
        data = uni['data/SandPile']

        # Check that all datasets exists
        assert 'slope' in data
        assert 'avalanche' in data
        assert 'avalanche_size' in data

        # Check that only a single time step is written
        assert data['slope'].time.size == 1
        assert data['avalanche'].time.size == 1
        assert data['avalanche_size'].shape == (1,)

def test_initial_no_toppling(): 
    """Test initial condition with non critical slopes."""

    mv, dm = mtc.create_run_load(from_cfg="initial_no_toppling.yml", 
            perform_sweep=True
        )

    for uni in dm['multiverse'].values():
        data = uni['data/SandPile']

        # Get configurations
        uni_cfg = uni['cfg']
        cell_manager_cfg = uni_cfg['SandPile']['cell_manager']

        # Get model parameter from config
        critical_slope = uni_cfg['SandPile']['critical_slope']
        resolution = cell_manager_cfg['grid']['resolution']

        # Calculate dataset coordinates
        time_coords = [0]
        space_coords = np.linspace(1. / (2 * resolution), 
                (1. - 1. / (2 * resolution)), num=resolution
            )
        
        # Calculate test datasets

        # Since the initial slope should be between 0 and 1 and one sand grain
        # is added in the initial phase, the resulting data should fit in the 
        # interval [0, 2].  
        test_slope_data = xr.DataArray([resolution * [resolution * [1]]], 
                dims=["time", "x", "y"],
                coords=dict(time=time_coords, x=space_coords, y=space_coords)
            )
        # In this test only one cell is in avalanche through the initial added 
        # sand grain. 
        test_avalanche_data = xr.DataArray([resolution * [resolution * [0.5]]], 
                dims=["time", "x", "y"],
                coords=dict(time=time_coords, x=space_coords, y=space_coords)
            )    
        test_avalanche_size_data = xr.DataArray([1], 
                dims=["time"], 
                coords=dict(time=time_coords)
            )

        # Check that all cells topple in the initial step
        assert np.all(data['slope'] <= critical_slope)
        assert np.any(data['avalanche'] == 1)

        xr.testing.assert_allclose(data['slope'].data, 
                test_slope_data, atol=1)
        xr.testing.assert_allclose(data['avalanche'].data, 
                test_avalanche_data, atol=0.5)
        xr.testing.assert_equal(data['avalanche_size'].data, 
                test_avalanche_size_data)


def test_initial_toppling():
    """Test initial condition with critical slopes in every cell."""

    mv, dm = mtc.create_run_load(from_cfg="initial_toppling.yml", 
            perform_sweep=True
        )

    for uni in dm['multiverse'].values():
        data = uni['data/SandPile']

        # Get configurations
        uni_cfg = uni['cfg']
        cell_manager_cfg = uni_cfg['SandPile']['cell_manager']

        # Get model parameter from config
        critical_slope = uni_cfg['SandPile']['critical_slope']
        resolution = cell_manager_cfg['grid']['resolution']

        # Calculate coordinate for dataset comparision
        time_coords = [0]
        space_coords = np.linspace(1. / (2. * resolution), 
                (1. - 1. / (2. * resolution)), num=resolution
            )

        
        # Calculate test datasets

        # For this test all cells are in avalanche and topple. With the 
        # critical slope at 4 all slopes are between 0 and 4. In the following
        # slope dataset is tested against one with all datapoints equal to 
        # 2 and with a tolerance of also 2. 
        test_slope_data = xr.DataArray(
                [resolution * [resolution * [critical_slope / 2.]]], 
                dims=["time", "x", "y"],
                coords=dict(time=time_coords, x=space_coords, y=space_coords)
            )
            
        # All cells should be in avalanche and so all datapoints equal to one.
        test_avalanche_data = xr.DataArray([resolution * [resolution * [1]]], 
                dims=["time", "x", "y"],
                coords=dict(time=time_coords, x=space_coords, y=space_coords)
            )
            
        # The number of cells in avalanche should be equal to the number
        # of cells.     
        test_avalanche_size_data = xr.DataArray([resolution**2], 
                dims=["time"], 
                coords=dict(time=time_coords)
            )

        
        # Performing the tests:
        assert np.all(data['slope'] <= critical_slope)
        assert np.all(data['avalanche'] == 1)

        xr.testing.assert_allclose(data['slope'].data, 
                test_slope_data, atol=critical_slope / 2.)
        xr.testing.assert_allclose(data['avalanche'].data, 
                test_avalanche_data)
        xr.testing.assert_equal(data['avalanche_size'].data, 
                test_avalanche_size_data)


def test_invalid_parameters():
    """Make sure invalid arguments lead to an error"""

    # Bad initial slope values limits
    with pytest.raises(SystemExit):
        mtc.create_run_load(from_cfg="initial_invalid_argument.yml", 
                perform_sweep=False,
                **cm_cfg(
                    cell_params=dict(
                        initial_slope_lower_limit=6,
                        initial_slope_upper_limit=5
                    )
                )
            )
    
    with pytest.raises(SystemExit):
        mtc.create_run_load(from_cfg="initial_invalid_argument.yml", 
                perform_sweep=False,
                **cm_cfg(
                    cell_params=dict(
                        initial_slope_lower_limit=5,
                        initial_slope_upper_limit=5
                    )
                )
            )

    with pytest.raises(ValidationError):
        mtc.create_run_load(from_cfg="initial_invalid_argument.yml", 
                perform_sweep=False,
                **cm_cfg(
                    cell_params=dict(
                        initial_slope_lower_limit=-5,
                        initial_slope_upper_limit=-1
                    )
                )
            )

    with pytest.raises(ValidationError):
        mtc.create_run_load(from_cfg="initial_invalid_argument.yml",
                perform_sweep=False,
                **model_cfg(critical_slope=-1)
            )


    # Bad neighborhood mode
    with pytest.raises(SystemExit):
        mtc.create_run_load(from_cfg="initial_invalid_argument.yml", 
                perform_sweep=False,
                **cm_cfg(neighborhood=dict(mode="Moore"))
            )
