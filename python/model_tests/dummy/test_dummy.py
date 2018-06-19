"""Tests of the output of the dummy model"""

import numpy as np

import pytest

from utopya.testtools import ModelTest
from utopya.tools import read_yml

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

    # Load data using the DataManager
    mv.dm.load_from_cfg(print_tree=True)
    # The `print_tree` flag creates output of which data was loaded

    # Assert that data was loaded, i.e. that data was written
    assert len(mv.dm)

    
def test_output(): 
    """Test that the output structure is correct"""
    # Create a Multiverse using a specific run configuration
    mv = mtc.create_mv_from_cfg("output.yml")
    cfg = read_yml(mtc.get_file_path("output.yml"))

    print(cfg)
    print(mv.meta_config['parameter_space']._init_dict)

    # Run a simulation
    mv.run_sweep()

    # Load data
    mv.dm.load_from_cfg(print_tree=True)

    # Assert that four runs finished successfully
    assert len(mv.dm['uni']) == 4

    # For each universe, iterate over the output data and assert the shape
    # and the content of the output data
    for uni_no, uni in mv.dm['uni'].items():
        # Get the data
        data = uni['data']['dummy']

        # Assert correct length (100 steps + initial state)
        assert len(data) == 101
        
        # Check the shape content of each dataset
        for dset_name, dset in data.items():
            assert dset.shape == (1000,)
            assert 0.45 <= np.mean(dset.data) <= 0.55
