"""Tests of the output of the SEIRD model"""

import numpy as np

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class for SEIRD
mtc = ModelTest("SEIRD", test_file=__file__)

# Tests -----------------------------------------------------------------------

def test_initial_state_empty():
    """
    Tests that the initial states are all empty,
    for the initial state empty and no infection source activated.
    """
    mv, dm = mtc.create_run_load(from_cfg="initial_state_empty.yml")

    # Get data
    grp = dm['multiverse'][0]['data/SEIRD']

    # Check if all cells are empty
    assert (grp["kind"] == 0).all()

def test_initial_stones():
    """
    Tests that if stones are activated any cells are "stone".
    """
    mv, dm = mtc.create_run_load(from_cfg="initial_stones.yml")

    # Get data
    grp = dm['multiverse'][0]['data/SEIRD']

    # Check if any cell is in stone state
    assert (grp["kind"] == 4).any()

def test_initial_state_source_south():
    """
    Initial state is 'empty', but an infection source at the southern side
    is activated.
    """
    _,dm = mtc.create_run_load(from_cfg="initial_state_empty_source_south.yml")

    # Get data
    grp = dm['multiverse'][0]['data/SEIRD']
    data = grp["kind"]

    # Check if all cells are empty, apart from the lowest horizontal row
    assert (data.isel(y=slice(1, None)) == 0).all()

    # Check if the lowest row is an infection source for all times
    assert (data.isel(y=0) == 3).all()

def test_growing_dynamic():
    """
    Test that with a p_growth probability of 1 and an empty initial state,
    all cells become trees after one timestep.
    """

    mv, dm = mtc.create_run_load(from_cfg="growing_dynamic.yml")

    # Get data
    grp = dm['multiverse'][0]['data/SEIRD']
    data = grp["kind"]

    # Check that all cells are trees after one timestep
    assert (data.isel(time=1) == 1).all()

def test_infection_dynamic():
    """
    Test that with a p_immunity probability of 0, a p_growth probability of 1
    and an empty initial state, with an infection source south the infection
    spreads according to the expectations.
    """

    mv, dm = mtc.create_run_load(from_cfg="infection_dynamic.yml")

    # Get data
    grp = dm['multiverse'][0]['data/SEIRD']
    data = grp["kind"]

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

    # Get the 2D densities dataset
    densities = dm['multiverse/0/data/SEIRD/densities']

    # Assert that the added up densities are approximately equal to 1
    assert np.isclose(densities.sum('kind'), 1.).all()

    # Densities should always be in interval [0., 1.]
    assert (0. <= densities).all()
    assert (densities <= 1.).all()

    # Initially, in this case, no tree should be infected -> density zero
    assert densities.sel(time=0, kind="infected").item() == 0


def test_infection_control_add_inf():
    """
    Test that the infection control via adding of a fixed number of infected
    cells at provided iteration steps works correctly.
    """
    mv, dm = mtc.create_run_load(from_cfg="infection_control_add_inf.yml")

    # Get the kind of the cells
    data = dm['multiverse/0/data/SEIRD/kind']

    for t in range(20):
        s = data.isel(time=t)
        
        unique, counts = np.unique(s, return_counts=True)
        d_counts = {u:c for u, c in zip(unique, counts)}
        print(d_counts)

        # At the start there should only be trees and empty spots
        if t < 5:
            assert 2 not in unique  # no infected
            assert 3 not in unique  # no infection source
            assert 4 not in unique  # no stones

        # The time step after the infection control should have a lot fewer
        # trees than in the previous iteration step because in every infection 
        # control step all trees are getting infected and only the newly grown 
        # trees remain
        if t in [6, 11, 16]:
            s_prev = data.isel(time=t-1)
        
            unique_prev, counts_prev = np.unique(s_prev, return_counts=True)
            d_counts_prev = {u:c for u, c in zip(unique_prev, counts_prev)}

            assert d_counts[1] < d_counts_prev[1]


def test_infection_control_change_p_random_inf():
    """
    Test that the infection control via change of the random
    infection probability works correctly.
    """
    mv, dm = mtc.create_run_load(from_cfg="infection_control_change_p_random_inf.yml")

    # Get the kind of the cells
    data = dm['multiverse/0/data/SEIRD/kind']

    for t in range(20):
        s = data.isel(time=t)
        
        unique, counts = np.unique(s, return_counts=True)
        d_counts = {u:c for u, c in zip(unique, counts)}

        # The probability for infection should be set to 0 the first 10 steps
        if t < 10:
            assert 2 not in unique

        # There are tree cells as well as infected cells for 5 time steps
        # However, there should be more tree cells than infected once
        if 10 < t < 15:
            assert 2 in unique
            assert 1 in unique
            assert d_counts[1] > d_counts[2]

        # The infection probability now is 1, so there should be more infected
        # cells than tree cells
        # NOTE that this test is a bit fragile in the sense that it can potentially
        #      be that it fails due to randomness.
        if t == 16:
            assert 2 in unique
            assert d_counts[2] > d_counts[1]
