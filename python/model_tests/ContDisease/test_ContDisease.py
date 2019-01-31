"""Tests of the output of the ContDisease model"""

import numpy as np

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class for ContDisease
mtc = ModelTest("ContDisease", test_file=__file__)

# Tests -----------------------------------------------------------------------

def test_initial_state_empty():
    """
    Tests that the initial states are all empty,
    for the initial state empty and no infection herd activated.
    """
    mv, dm = mtc.create_run_load(from_cfg="initial_state_empty.yml")

    # Get data
    grp = dm['multiverse'][0]['data/ContDisease']
    data_ = grp["state"]

    # Get the grid size
    uni_cfg = dm['multiverse'][0]['cfg']
    grid_size = uni_cfg['ContDisease']['grid_size']

    # Check that only one step was performed and written
    assert data_.shape == (1, grid_size[1], grid_size[0])

    # Check if all cells are empty
    assert (data_ == 0).all()

def test_initial_stones():
    """
    Tests that if stones are activated any cells are "stone".
    """
    mv, dm = mtc.create_run_load(from_cfg="initial_stones.yml")

    # Get data
    grp = dm['multiverse'][0]['data/ContDisease']
    data_ = grp["state"]

    # Get the grid size
    uni_cfg = dm['multiverse'][0]['cfg']
    grid_size = uni_cfg['ContDisease']['grid_size']

    # Check that only one step was performed and written
    assert data_.shape == (1, grid_size[1], grid_size[0])

    # Check if any cell is in stone state
    assert (data_ == 4).any()

def test_initial_state_herd_south():
    """
    Initial state is 'empty', but an infection herd at the southern side
    is activated.
    """

    mv, dm = mtc.create_run_load(from_cfg="initial_state_empty_herd_south.yml")

    # Get data
    grp = dm['multiverse'][0]['data/ContDisease']
    data = grp["state"]

    # Get the grid size
    uni_cfg = dm['multiverse'][0]['cfg']
    grid_size = uni_cfg['ContDisease']['grid_size']

    # Check that only one step was written
    assert grp["state"].shape == (1, grid_size[1], grid_size[0])

    # Check if all Cells are empty, apart from the lowest horizontal row
    assert (data[0][1:] == 0).all()

    # Check if the lowest row is an infection herd
    assert (data[0][0] == 3).all()

def test_growing_dynamic():
    """
    Test that with a p_growth probability of 1 and an empty initial state,
    all cells become trees after one timestep.
    """

    mv, dm = mtc.create_run_load(from_cfg="growing_dynamic.yml")

    # Get data
    grp = dm['multiverse'][0]['data/ContDisease']
    data_ = grp["state"]

    # Get the grid size
    uni_cfg = dm['multiverse'][0]['cfg']
    grid_size = uni_cfg['ContDisease']['grid_size']

    # Check that only two steps were written
    assert data_.shape == (2, grid_size[1], grid_size[0])

    # Check that all cells are trees after one timestep
    assert (data_[1] == 1).all()

def test_infection_dynamic():
    """
    Test that with a p_infect probability of 1, a p_growth probability of 1
    and an empty initial state, with an infection herd south the infection
    spreads according to the expectations.
    """

    mv, dm = mtc.create_run_load(from_cfg="infection_dynamic.yml")

    # Get data
    grp = dm['multiverse'][0]['data/ContDisease']
    data = grp["state"]

    # Get the grid size
    uni_cfg = dm['multiverse'][0]['cfg']
    grid_size = uni_cfg['ContDisease']['grid_size']

    # Check that only three steps were written
    assert data.shape == (4, grid_size[1], grid_size[0])

    #Check that the last row is an infection herd
    assert (data[0][0] == 3).all()

    # Check that only the second row is infected after two timesteps and the
    # first are trees.
    assert (data[2][-1] == 1).all()
    assert (data[2][1] == 2).all()
    assert (data[2][0] == 3).all()

    # Check that the second row is empty after three timesteps and the
    # first is infected.
    assert (data[3][-1] == 2).all()
    assert (data[3][1] == 0).all()
    assert (data[3][0] == 3).all()


def test_densities_calculation():
    mv, dm = mtc.create_run_load(from_cfg="densities_calculation.yml")

    # # Get the data
    # data = dm['multiverse/0/data/ContDisease']
    # d_infected = data['density_infected']
    # d_tree = data['density_tree']

    # # The densities for herds and stones are not written out,
    # # so, we have to approximate them from the configuration
    # cfg = dm['multiverse/0/cfg']
    # grid_size = dm['multiverse/0/cfg/ContDisease']['grid_size']

    # d_stones = cfg['stone_density'] + cfg['stone_cluster']

    # # The herd is a line at the southern border, so equals the number of cells 
    # # in one line divided by the total number of cells
    # d_herd = grid_size[-1] / np.prod(grid_size)

    # # Assert that the added up densities are smaller or equal to 1.
    # # NOTE that the empty cells are not known here, thus, this should always be
    # # the case
    # assert([0 < (d_inf + d_t + d_stones + d_herd) < 1 for d_inf, d_t in zip(d_infected, d_tree)])

    # # There should always be some trees
    # assert([d > 0 for d in d_tree])

    # # Initially, no tree should be infected
    # assert(d_infected[0])