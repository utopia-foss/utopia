"""Tests of the dynamics of the ForestFire"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

from .test_init import model_cfg

# Configure the ModelTest class
mtc = ModelTest("ForestFire", test_file=__file__)

# Tests -----------------------------------------------------------------------


def test_dynamics():
    """Test that the dynamics are correct."""

    # Use the config file for common settings, change via additional kwargs
    mv, dm = mtc.create_run_load(from_cfg="dynamics.yml")

    # For the universe with f=0, ignited bottom and two_state_model=true
    for uni_no, uni in dm["multiverse"].items():
        data = uni["data"]["ForestFire"]["kind"]

        # Need the number of cells to calculate the density
        num_cells = data.shape[1] * data.shape[2]

        # all cells tree at time step 0
        density = np.sum(data[0]) / num_cells
        assert density == 1.0

        # all cells burned + 1% growth
        density = data.mean(dim=["x", "y"])
        assert 0 <= density[{"time": 1}].values <= 0.02

        # 1% growth
        density = data.mean(dim=["x", "y"])
        assert 0.01 <= density[{"time": 2}].values <= 0.02 + 0.05


def test_immunity():
    """ """
    pass


def test_fire_source():
    """Runs the model with the bottom row constantly ignited"""
    mv, dm = mtc.create_run_load(from_cfg="fire_source.yml")

    # Make sure the bottom row is always marked as source
    for uni_no, uni in dm["multiverse"].items():
        data = uni["data"]["ForestFire"]["kind"]

        # All cells in the bottom row are always in state "source", i.e.: 3
        assert (data.isel(y=0) == 3).all()


# SPHINX-MARKER (you can ignore this)
def test_stones():
    """Tests that stones are created and are constant over time"""
    # Create the multiverse, run it with the stones config, and load data
    mv, dm = mtc.create_run_load(from_cfg="stones.yml")

    # Go over all the created universes
    for uni_no, uni in dm["multiverse"].items():
        # Get the kind of each cell
        data = uni["data"]["ForestFire"]["kind"]

        # There is a non-zero number of stone cells (encoded by kind == 4)
        assert (data == 4).sum() > 0

        # ... which is constant over time
        assert ((data == 4).sum(["x", "y"]).diff("time") == 0).all()
