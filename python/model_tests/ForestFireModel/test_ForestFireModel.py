"""Tests of the output of the CopyMe model"""

import numpy as np

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class for ForestFireModel
mtc = ModelTest("ForestFireModel", test_file=__file__)

# Fixtures --------------------------------------------------------------------
# Define fixtures


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
        data = uni['data']['ForestFireModel']

        # Get the config of this universe
        uni_cfg = uni['cfg']

        # Calculate the number of cells
        grid_size = uni_cfg['ForestFireModel']['grid_size']
        num_cells = grid_size[0] * grid_size[1]

        # Check that all datasets are available
        assert 'state' in data

        # Assert they have the correct shape
        assert data['state'].shape == (uni_cfg['num_steps'] + 1,
                                            num_cells)

        # Can do further tests here ...
