"""Tests of the output of the CopyMe model"""

import numpy as np

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class for CopyMe
mtc = ModelTest("CopyMe", test_file=__file__)  # TODO set your model name here

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

# NOTE The below test is an example of how to access data and write a test.
#      You can adjust this to your needs and then remove the decorator to
#      enable the test.
@pytest.mark.skip("Need to adapt this to the CopyMe model")
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
        data = uni['data']['CopyMe']  # TODO change this to your model name

        # Get the config of this universe
        uni_cfg = uni['cfg']

        # Assert correct length (100 steps + initial state)
        assert len(data) == uni_cfg['num_steps'] + 1
        
        # # Check the content of all available datasets
        # for dset_name, dset in data.items():
        #     # Assert dataset shape is correct
        #     assert dset.shape == (1000,)  
