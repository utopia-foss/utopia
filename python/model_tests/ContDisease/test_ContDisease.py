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
    for the initial state empty and no infection source activated.
    """
    mv, dm = mtc.create_run_load(from_cfg="initial_state_empty.yml")

    # Get data
    grp = dm['multiverse'][0]['data/ContDisease']
    data_ = grp["state"]

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

    # Check if any cell is in stone state
    assert (data_ == 4).any()

def test_initial_state_source_south():
    """
    Initial state is 'empty', but an infection source at the southern side
    is activated.
    """

    mv, dm = mtc.create_run_load(from_cfg="initial_state_empty_source_south.yml")

    # Get data
    grp = dm['multiverse'][0]['data/ContDisease']
    data = grp["state"]

    # Check if all Cells are empty, apart from the lowest horizontal row
    assert (data.isel(y=slice(1, None)) == 0).all()

    # Check if the lowest row is an infection source
    assert (data.isel(y=0) == 3).all()

def test_growing_dynamic():
    """
    Test that with a p_growth probability of 1 and an empty initial state,
    all cells become trees after one timestep.
    """

    mv, dm = mtc.create_run_load(from_cfg="growing_dynamic.yml")

    # Get data
    grp = dm['multiverse'][0]['data/ContDisease']
    data_ = grp["state"]

    # Check that all cells are trees after one timestep
    assert (data_.isel(time=1) == 1).all()

def test_infection_dynamic():
    """
    Test that with a p_infect probability of 1, a p_growth probability of 1
    and an empty initial state, with an infection source south the infection
    spreads according to the expectations.
    """

    mv, dm = mtc.create_run_load(from_cfg="infection_dynamic.yml")

    # Get data
    grp = dm['multiverse'][0]['data/ContDisease']
    data = grp["state"]

    # Check that the last row is an infection source
    assert (data.isel(y=0) == 3).all()

    # Check that only the second row is infected after two timesteps and the
    # first are trees.
    assert (data.isel(time=2, y=2) == 1).all()
    assert (data.isel(time=2, y=1) == 2).all()
    assert (data.isel(time=2, y=0) == 3).all()

    # Check that the second row is empty after three timesteps and the
    # first is infected.
    assert (data.isel(time=3, y=2) == 2).all()
    assert (data.isel(time=3, y=1) == 0).all()
    assert (data.isel(time=3, y=0) == 3).all()


def test_densities_calculation():
    mv, dm = mtc.create_run_load(from_cfg="densities_calculation.yml")

    # Get the data
    densities = dm['multiverse/0/data/ContDisease/densities']
    d_infected = densities['infected']
    d_tree = densities['tree']
    d_empty = densities['empty']
    d_stones = densities['stone']
    d_source = densities['source']

    # Assert that the added up densities are approximately equal to 1
    d_partial_sum = d_empty.data + d_tree.data + d_infected.data
    assert [dps + d_stones.data + d_source.data == pytest.approx(1.) for dps in d_partial_sum]

    # Densities should always be in the range 0<=d<=1
    assert [0 <= d <= 0 for d in d_tree]
    assert [0 <= d <= 0 for d in d_empty]
    assert [0 <= d <= 0 for d in d_infected]
    assert 0 <= d_stones <= 1
    assert 0 <= d_source <= 1

    # Initially, no tree should be infected
    assert d_infected.isel(time=0) == 0
