"""Tests of the output of the dummy model"""

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

    # Load data using the DataManager
    mv.dm.load_from_cfg(print_tree=True)
    # The `print_tree` flag creates output of which data was loaded

    # Assert that data was loaded, i.e. that data was written
    assert len(mv.dm)
    
    
def test_output(): 
    """Test that the output structure is correct"""
    # Create a Multiverse using a specific run configuration
    mv = mtc.create_mv_from_cfg("output.yml")

    # Run a simulation
    mv.run_sweep()

    # Load data
    mv.dm.load_from_cfg(print_tree=True)

    # TODO add assertions below ...
