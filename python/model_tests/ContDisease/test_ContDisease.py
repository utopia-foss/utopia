"""Tests of the output of the ContDisease model"""

import numpy as np

import pytest

from utopya.testtools import ModelTest


# Configure the ModelTest class for ContDisease
mtc = ModelTest("ContDisease", test_file=__file__)

# Fixtures --------------------------------------------------------------------
# Define fixtures


# Tests -----------------------------------------------------------------------


def test_initial_state_empty():
    """
    Tests that the initial states are all empty,
    for the initial state empty and no infection herd activated.
    """
    mv, dm = mtc.create_run_load(from_cfg="initial_state_empty.yml")

    # Get data
    grp = dm['uni'][0]['data/ContDisease']
    data_ = grp["state"]

    # Get the grid size
    uni_cfg = dm['uni'][0]['cfg']
    grid_size = uni_cfg['ContDisease']['grid_size']

    num_cells = grid_size[0] * grid_size[1]

    # Check that only one step was performed and written

    assert data_.shape == (1, num_cells)

    # Check if all Cells are empty

def test_initial_stones():
    """
    Tests that if stones are activated any cells are "stone".
    """
    mv, dm = mtc.create_run_load(from_cfg="initial_stones.yml")

    # Get data
    grp = dm['uni'][0]['data/ContDisease']
    data_ = grp["state"]

    # Get the grid size
    uni_cfg = dm['uni'][0]['cfg']
    grid_size = uni_cfg['ContDisease']['grid_size']

    num_cells = grid_size[0] * grid_size[1]

    # Check that only one step was performed and written

    assert data_.shape == (1, num_cells)

    # Check if all Cells are empty

    assert data_.any() == (np.zeros(num_cells)+4).any()



def test_initial_state_herd_south():
    """
    Initial state is 'empty', but an infection herd at the southern side
    is activated.
    """

    mv, dm = mtc.create_run_load(from_cfg="initial_state_empty_herd_south.yml")

    # Get data
    grp = dm['uni'][0]['data/ContDisease']
    data_ = grp["state"]

    # Get the grid size
    uni_cfg = dm['uni'][0]['cfg']
    grid_size = uni_cfg['ContDisease']['grid_size']
    num_cells = grid_size[0] * grid_size[1]

    num_steps = uni_cfg['num_steps']
    data = np.reshape(data_, (num_steps+1, grid_size[1], grid_size[0]))


    # Check that only one step was written

    assert data_.shape == (1, num_cells)

    # Check if all Cells are empty, apart from the lowest horizontal row

    assert data[0][:-1].all() == np.zeros((grid_size[1],grid_size[0]-1)).all()

    # Check if the lowest row is an infection herd

    assert data[0][-1].all() == (np.zeros(grid_size[1]+3)).all()

def test_growing_dynamic():
    """
    Test that with a p_growth probability of 1 and an empty initial state,
    all cells become trees after one timestep.
    """

    mv, dm = mtc.create_run_load(from_cfg="growing_dynamic.yml")

    # Get data
    grp = dm['uni'][0]['data/ContDisease']
    data_ = grp["state"]

    # Get the grid size
    uni_cfg = dm['uni'][0]['cfg']
    grid_size = uni_cfg['ContDisease']['grid_size']
    num_cells = grid_size[0] * grid_size[1]

    # Check that only two steps were written

    assert data_.shape == (2, num_cells)

    # Check that all cells are trees after one timestep

    assert data_[1].all() == (np.full((grid_size[1],grid_size[0]),1)).all()

def test_infection_dynamic():
    """
    Test that with a p_infect probability of 1, a p_growth probability of 1
    and an empty initial state, with an infection herd south the infection spreads
    according to the expectations.
    """

    mv, dm = mtc.create_run_load(from_cfg="infection_dynamic.yml")

    # Get data
    grp = dm['uni'][0]['data/ContDisease']
    data_ = grp["state"]

    # Get the grid size
    uni_cfg = dm['uni'][0]['cfg']
    grid_size = uni_cfg['ContDisease']['grid_size']
    num_cells = grid_size[0] * grid_size[1]

    num_steps = uni_cfg['num_steps']
    data = np.reshape(data_, (num_steps+1, grid_size[1], grid_size[0]))

    # Check that only three steps were written

    assert data_.shape == (4, num_cells)

    #Check that the last row is an infection herd

    assert data[0][-1].all() == (np.zeros(grid_size[1]+3)).all()

    # Check that only the second row is infected after two timesteps and the
    # first are trees.

    assert data[2][0].all() == (np.zeros(grid_size[1])+1).all()
    assert data[2][1].all() == (np.zeros(grid_size[1])+2).all()
    assert data[2][-1].all() == (np.zeros(grid_size[1])+3).all()

    # Check that the second row is empty after three timesteps and the
    # first is infected.

    assert data[3][0].all() == (np.zeros(grid_size[1])+2).all()
    assert data[3][1].all() == (np.zeros(grid_size[1])).all()
    assert data[3][-1].all() == (np.zeros(grid_size[1])+3).all()
