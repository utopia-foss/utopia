"""Tests of the output of the CopyMeBare model"""

import numpy as np

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class
mtc = ModelTest("CopyMeBare", test_file=__file__)  # TODO set the model name


# Fixtures --------------------------------------------------------------------
# Define fixtures


# Tests -----------------------------------------------------------------------

def test_that_it_runs():
    """Tests that the model runs through with the default settings"""
    # Create a Multiverse using the default model configuration
    mv = mtc.create_mv()

    # Run a single simulation
    mv.run_single()

    # Load data using the DataManager and the default load configuration
    mv.dm.load_from_cfg(print_tree=True)
    # The `print_tree` flag creates output of which data was loaded
