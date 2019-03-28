"""Tests of the output of the Geomorphology model"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

mtc = ModelTest("Geomorphology", test_file=__file__)

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
    # NOTE this is a shortcut. It creates the mv, lets it run, then loads data

    # Get the meta-config from the DataManager
    mcfg = dm['cfg']['meta']
    print("meta config: ", mcfg)

    # Assert that the number of runs matches the specified ones
    assert len(dm['multiverse']) == mcfg['parameter_space'].volume

    # For each universe, iterate over the output data and assert the shape
    # and the content of the output data
    for uni_no, uni in dm['multiverse'].items():
        # Get the data
        data = uni['data']['Geomorphology']

        # Get the config of this universe
        uni_cfg = uni['cfg']

        # Check that all datasets are available
        assert 'water_content' in data
        assert 'height' in data


def test_drainage_network():
    mv, dm = mtc.create_run_load(from_cfg="network.yml", perform_sweep=True)

    for uni_no, uni in dm['multiverse'].items():
        data = uni['data']['Geomorphology']
        uni_cfg = uni['cfg']

        network_data = data['drainage_area']
        grid_shape = network_data.shape[1:]
        num_cells = grid_shape[0] * grid_shape[1]

        assert np.sum(network_data, axis=2)[0][0] == num_cells

