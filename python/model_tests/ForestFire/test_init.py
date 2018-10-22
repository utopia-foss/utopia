"""Tests of the initialisation of the ForestFire"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class for ForestFire model
mtc = ModelTest("ForestFire", test_file=__file__)

# Fixtures --------------------------------------------------------------------
# Define fixtures

# Helpers ---------------------------------------------------------------------
# NOTE These helpers are also imported by other test modules for ForestFire

def model_cfg(**kwargs) -> dict:
    """Creates a dict that can update the config of the ForestFire"""
    return dict(parameter_space=dict(ForestFire=dict(**kwargs)))

# Tests -----------------------------------------------------------------------

def test_basics():
    """Test the most basic features of the model"""
    # Create a Multiverse using the default model configuration
    mv = mtc.create_mv()

    # Run a single simulation
    mv.run_single()

    # Load data using the DataManager and the default load configuration
    mv.dm.load_from_cfg(print_tree=True)
    # The `print_tree` flag creates output of which data was loaded
    # NOTE can also use a shortcut to do all of the above, see test_output

    # Assert that data was loaded, i.e. that data was written
    assert len(mv.dm)

def test_output(): 
    """Test that the output structure is correct"""
    # Create a Multiverse and let it run
    mv, dm = mtc.create_run_load(from_cfg="output.yml", perform_sweep=True)
    # NOTE this is a shortcut. It creates the mv, lets it run, then loads data

    # Get the meta-config from the DataManager
    mcfg = dm['cfg']['meta']
    print("meta config: ", mcfg)

    # Assert that the number of runs matches the specified ones
    assert len(dm['uni']) == mcfg['parameter_space'].volume

    # For each universe, iterate over the output data and assert the shape
    # and the content of the output data
    for uni_no, uni in dm['uni'].items():
        # Get the data
        data = uni['data']['ForestFire']

        # Get the config of this universe
        uni_cfg = uni['cfg']

        # Calculate the number of cells
        grid_size = uni_cfg['ForestFire']['grid_size']
        num_cells = grid_size[0] * grid_size[1]

        # Check that all datasets are available
        assert 'state' in data

        # Assert they have the correct shape
        assert data['state'].shape == (uni_cfg['num_steps'] + 1,
                                            num_cells)

def test_initial_state_random(): 
    """Test that the initial states are random.

    This also tests for the correct array shape, something that is not done in
    the other tests.
    """
    # Use the config file for common settings, change via additional kwargs
    mv, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                 perform_sweep=True)

    # For all universes, perform checks on the state
    for uni in dm['uni'].values():
        data = uni['data']['ForestFire']

        # Get the grid size
        grid_size = uni['cfg']['ForestFire']['grid_size']
        num_cells = grid_size[0] * grid_size[1]

        # Check that only a single step was written and the extent is correct
        assert data['state'].shape == (1, num_cells)

        # check, that no cell is burning
        assert 0 <= np.amax(data['state']) <= 1

        # Strategies should be random; calculate the ratio and check limits
        density = np.sum(data['state'])/data['state'].shape[1]
        assert 0.45 <= density <= 0.55  # TODO values ok?


    # Test again for another probability value
    initial_density = 0.2
    mv, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                 perform_sweep=True,
                                 **model_cfg(initial_density=initial_density))

    for uni in dm['uni'].values():
        data = uni['data']['ForestFire']

        # check, that no cell is burning
        assert 0 <= np.amax(data['state']) <= 1

        # Calculate fraction and compare to desired probability
        density = np.sum(data['state'])/data['state'].shape[1]
        assert initial_density - 0.05 <= density <= initial_density + 0.05

    # Test again for another probability value - implies _set_initial_state_empty function
    initial_density = 0.0
    mv, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                 perform_sweep=True,
                                 **model_cfg(initial_density=initial_density))

    for uni in dm['uni'].values():
        data = uni['data']['ForestFire']

        # check, that no cell is burning
        assert 0 <= np.amax(data['state']) <= 1

        # Calculate fraction and compare to desired probability
        density = np.sum(data['state'])/data['state'].shape[1]
        assert density == initial_density

    initial_density = 1.0
    mv, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                 perform_sweep=True,
                                 **model_cfg(initial_density=initial_density))

    for uni in dm['uni'].values():
        data = uni['data']['ForestFire']

        # check, that no cell is burning
        assert 0 <= np.amax(data['state']) <= 1

        # Calculate fraction and compare to desired probability
        density = np.sum(data['state'])/data['state'].shape[1]
        assert density == initial_density