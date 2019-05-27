"""Tests of the dynamics of the ForestFire"""

import pytest
import numpy as np

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
    for uni_no, uni in dm['multiverse'].items():
        data = uni['data']['ForestFire']['kind']

        # Need the number of cells to calculate the density
        num_cells = data.shape[1] * data.shape[2]
        
        # all cells tree at time step 0
        density = np.sum(data[0])/num_cells
        assert density == 1.0

        # all cells burned + 1% growth
        density = data.mean(dim=['x', 'y'])
        assert 0 <= density[{'time': 1}].values <= 0.02

        # 1% growth
        density = data.mean(dim=['x', 'y'])
        assert 0.01 <= density[{'time': 2}].values <= 0.02 + 0.05


def test_percolation_mode():
    """Runs the model with the bottom row constantly ignited"""
    mv, dm = mtc.create_run_load(from_cfg="percolation_mode.yml")

    # Make sure the bottom row is always empty
    for uni_no, uni in dm['multiverse'].items():
        data = uni['data']['ForestFire']['kind']

        # All cells in the bottom row are always in state empty
        assert (data.isel(y=0, time=slice(1, None)) == 0).all()
        # NOTE For the initial state, this is not true.
