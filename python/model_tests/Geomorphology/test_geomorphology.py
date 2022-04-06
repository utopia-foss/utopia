"""Tests of the output of the Geomorphology model"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

mtc = ModelTest("Geomorphology", test_file=__file__)


def test_basics():
    """Test the most basic features of the model"""
    # Create a Multiverse using the default model configuration
    mv = mtc.create_mv(from_cfg="basics.yml")

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
    mcfg = dm["cfg"]["meta"]

    # Assert that the number of runs matches the specified ones
    assert len(dm["multiverse"]) == mcfg["parameter_space"].volume

    # For each universe, iterate over the output data and assert the shape
    # and the content of the output data
    for uni_no, uni in dm["multiverse"].items():
        # Get the data
        data = uni["data"]["Geomorphology"]

        # Get the config of this universe
        uni_cfg = uni["cfg"]

        # Check that all datasets are available
        assert "watercolumn" in data
        assert "height" in data
        assert "drainage_area" in data


def test_drainage_network():
    """Test the correct initialization of the drainage network

    The drainage of water must be a conserved quantity. The (instantly
    cumulative) drainage area at the southern (outflow) boundary must equal the
    number of cells in the system (note it is not normalized to the space units).
    Hence the sum over the cells with index 'y'=0 in dim 'x' must equal the
    number of cells.
    """
    mv, dm = mtc.create_run_load(from_cfg="network.yml", perform_sweep=True)

    for uni_no, uni in dm["multiverse"].items():
        data = uni["data"]["Geomorphology"]
        uni_cfg = uni["cfg"]

        network_data = data["drainage_area"]
        grid_shape = network_data.shape[1:]
        num_cells = grid_shape[0] * grid_shape[1]

        assert network_data.isel({"y": 0, "time": 0}).sum(dim="x") == num_cells
        assert network_data.isel({"y": 0, "time": 1}).sum(dim="x") == num_cells
