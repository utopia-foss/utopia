"""Tests of the dynamics of the ForestFire"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

from .test_init import model_cfg

# Configure the ModelTest class
mtc = ModelTest("ForestFire", test_file=__file__)

# Utility functions -----------------------------------------------------------

def assert_eq(a, b, *, epsilon=1e-6):
    """Assert that two quantities are equal within a numerical epsilon range"""
    assert(np.absolute(a-b) < epsilon)

# Tests -----------------------------------------------------------------------

def test_dynamics_two_state_model(): 
    """Test that the dynamics are correct."""

    # Use the config file for common settings, change via additional kwargs
    mv, dm = mtc.create_run_load(from_cfg="initial_dynamics_two_state.yml")

    ## For the universe with f=0, ignited bottom and two_state_model=true
    uni = dm['uni'][0]
    data = uni['data']['ForestFire']

    # Get the grid size
    grid_size = uni['cfg']['ForestFire']['grid_size']
    num_cells = grid_size[0] * grid_size[1]
    steps =  uni['cfg']['num_steps']
    new_shape = (steps+1, grid_size[1], grid_size[0])

    data_state = np.reshape(data['state'], new_shape)

    # all cells tree
    density = np.sum(data_state[0,:,:])/num_cells
    assert density==1.0

    # all cells burned + 1% growth
    density = np.sum(data_state[1,:,:])/num_cells
    assert 0 <= density <= 0.01

    # 1% growth
    density = np.sum(data_state[2,:,:])/num_cells
    assert 0.01 <= density <= 0.01 + 0.05