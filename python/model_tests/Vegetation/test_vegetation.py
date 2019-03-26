"""Tests of the output of the vegetation model"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

mtc = ModelTest("Vegetation", test_file=__file__)

def test_basics():
    """Test the most basic features of the model"""
    # Create a Multiverse using the default model configuration
    mv = mtc.create_mv()

    # Run a single simulation
    mv.run_single()

    # Load data using the DataManager and the default load configuration
    mv.dm.load_from_cfg(print_tree=True)

    # Assert that data was loaded, i.e. that data was written
    assert len(mv.dm)

def test_output(): 
    """Test that the output structure is correct"""
    # Create a Multiverse and let it run
    mv, dm = mtc.create_run_load(from_cfg="output.yml", perform_sweep=True)

    # Get the meta-config from the DataManager
    mcfg = dm['cfg']['meta']
    print("meta config: ", mcfg)

    # Assert that the number of runs matches the specified ones
    assert len(dm['multiverse']) == mcfg['parameter_space'].volume

    # For each universe, iterate over the output data and assert the shape
    # and the content of the output data
    for uni_no, uni in dm['multiverse'].items():
        # Get the data
        data = uni['data']['Vegetation']

        # Check that all datasets are available
        assert 'plant_mass' in data

        # Check initial condition
        assert (data['plant_mass'][0] == 0.0).all()

        # Assert plant mass data is always positive
        assert not (data['plant_mass'] < 0).any()
