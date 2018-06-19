"""Tests of the output of the CopyMe model"""

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class
mtc = ModelTest("dummy", test_file=__file__) # TODO set model name here!

# Fixtures --------------------------------------------------------------------
# Define fixtures here


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

    # TODO Add more assertions below ...


# Example for another test function. If you want to use it, remove the
# decorator, rename it to be more descriptive, and adjust the docstring
@pytest.mark.skip()
def test_advanced_stuff(): 
    """Test that ... TODO"""
    # Create a Multiverse using a specific run configuration
    mv = mtc.create_mv_from_cfg("advanced.yml")
    # TODO rename the file to match the name of the test it is used in

    # Run a simulation (here: a sweep, as configured by `perform_sweep` entry)
    mv.run()

    # Load data
    mv.dm.load_from_cfg(print_tree=True)

    # TODO add assertions below ...

