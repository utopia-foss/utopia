"""Tests of the dynamics of the ForestFireModel"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

from .test_init import model_cfg

# Configure the ModelTest class
mtc = ModelTest("ForestFireModel", test_file=__file__)

# Utility functions -----------------------------------------------------------

def assert_eq(a, b, *, epsilon=1e-6):
    """Assert that two quantities are equal within a numerical epsilon range"""
    assert(np.absolute(a-b) < epsilon)

# Tests -----------------------------------------------------------------------

def test_dynamics_three_state_model(): 
    """Test that the dynamics are correct."""

    # Use the config file for common settings, change via additional kwargs
    mv, dm = mtc.create_run_load(from_cfg="initial_dynamics_three_state.yml",
                                 perform_sweep=False,
                                 **model_cfg(lightning_frequency=0, light_bottom_row=True))

    ## For the universe with f=0, ignited bottom and two_state_model=false
    uni = dm['uni'][0]
    data = uni['data']['ForestFireModel']

    # Get the grid size
    grid_size = uni['cfg']['ForestFireModel']['grid_size']
    num_cells = grid_size[0] * grid_size[1]
    steps =  uni['cfg']['num_steps']
    new_shape = (steps+1, grid_size[1], grid_size[0])

    data_state = np.reshape(data['state'], new_shape)

    # bottom ligne ignited
    density = np.sum(data_state[1,0,:])/grid_size[1]
    assert density == 2.0

    # first line tree
    density = np.sum(data_state[1,1,:])/grid_size[1]
    assert density == 1.0

    # bottom ligne burned
    density = np.sum(data_state[2,0,:])/grid_size[1]
    assert density == 0.0

    # first line ignited
    density = np.sum(data_state[2,1,:])/grid_size[1]
    assert density == 2.0

    # Use the config file for common settings, change via additional kwargs
    mv, dm = mtc.create_run_load(from_cfg="initial_dynamics_three_state.yml",
                                 perform_sweep=False,
                                 **model_cfg(lightning_frequency=0.01, light_bottom_row=False))

    ## For the universe with f=0.1, non-ignited bottom and two_state_model=false
    uni = dm['uni'][0]
    data = uni['data']['ForestFireModel']

    # Get the grid size
    grid_size = uni['cfg']['ForestFireModel']['grid_size']
    num_cells = grid_size[0] * grid_size[1]
    steps =  uni['cfg']['num_steps']
    new_shape = (steps+1, grid_size[1], grid_size[0])

    data_state = np.reshape(data['state'], new_shape)

    # 1% of cells ignited
    density = np.sum(data_state[1,:,:])/num_cells
    assert 1 + 0.01 - 0.05 <= density <= 1 + 0.01 + 0.05

    # 4 neighbors ignited, 1% burned
    cnt_ignited = 0
    cnt_grass = 0
    cnt_tree = 0
    for i in range(grid_size[0]):
        for j in range(grid_size[1]):
            if data_state[2,i,j] == 0:
                cnt_grass += 1
            elif data_state[2,i,j] == 1:
                cnt_tree += 1
            else:
                cnt_ignited += 1
    assert cnt_grass == 16
    assert 7*16 <=  cnt_ignited <= 8*16
    density = np.sum(data_state[2,:,:])/num_cells
    assert 1 - 0.01 + 8*0.01 - 0.05 <= density <= 1 - 0.01 + 8*0.01 + 0.05

def test_dynamics_two_state_model(): 
    """Test that the dynamics are correct."""

    # Use the config file for common settings, change via additional kwargs
    mv, dm = mtc.create_run_load(from_cfg="initial_dynamics_two_state.yml")

    ## For the universe with f=0, ignited bottom and two_state_model=true
    uni = dm['uni'][0]
    data = uni['data']['ForestFireModel']

    # Get the grid size
    grid_size = uni['cfg']['ForestFireModel']['grid_size']
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