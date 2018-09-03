"""Tests of the output of the dummy model"""

import numpy as np

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class for dummy
mtc = ModelTest("dummy", test_file=__file__)

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

    # Get the meta-config from the DataManager
    mcfg = dm['cfg']['meta']
    print("meta config: ", mcfg)

    # Assert that four runs finished successfully, as configured
    assert len(dm['uni']) == mcfg['parameter_space'].volume

    # For each universe, iterate over the output data and assert the shape
    # and the content of the output data
    for uni_no, uni in dm['uni'].items():
        # Assert that data was written
        assert 'state' in uni['data']['dummy']

        # Get the data
        state = uni['data']['dummy']['state']

        # Get the config of this universe
        uni_cfg = uni['cfg']

        # Assert correct length (100 steps + initial state)
        assert state.shape == (uni_cfg['num_steps'] + 1, 1000)
        
        # Check that the rows are as expected
        for step in range(state.shape[0]):
            # Assert correctness of data, depending on step
            assert step*0.4 <= np.mean(state[step,:]) <= step*0.6
            # NOTE the dummy model adds random numbers between 0 and 1 to its
            # state vector each time step, thus increasing the mean by 0.5
            # each time step. With a state vector of length 1000, the mean
            # should thus always be within these limits.
