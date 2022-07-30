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
    grp = dm["multiverse"][0]["data/SEIRD"]

    # Check if all cells are empty
    assert (grp["kind"] == 0).all()


def test_initial_inert():
    """
    Tests that if inert are activated any cells are "inert".
    """
    mv, dm = mtc.create_run_load(from_cfg="initial_inert.yml")

    # Get data
    grp = dm["multiverse"][0]["data/SEIRD"]

    # Check if any cell is in inert state
    assert (grp["kind"] == 7).any()


def test_initial_state_source_south():
    """
    Initial state is 'empty', but an infection source at the southern side
    is activated.
    """
    _, dm = mtc.create_run_load(
        from_cfg="initial_state_empty_source_south.yml"
    )

    # Get data
    grp = dm["multiverse"][0]["data/SEIRD"]
    data = grp["kind"]

    # Check if all cells are empty, apart from the lowest horizontal row
    assert (data.isel(y=slice(1, None)) == 0).all()

    # Check if the lowest row is an infection source for all times
    assert (data.isel(y=0) == 6).all()


def test_growing_dynamic():
    """
    Test that with a p_susceptible probability of 1 and an empty initial state,
    all cells become susceptible after one timestep.
    """

    mv, dm = mtc.create_run_load(from_cfg="growing_dynamic.yml")

    # Get data
    grp = dm["multiverse"][0]["data/SEIRD"]
    data = grp["kind"]

    # Check that all cells are susceptible after one timestep
    assert (data.isel(time=1) == 1).all()


def test_infection_dynamic():
    """
    Test that with a p_immunity and p_random_immunity probability of 0,
    a p_susceptible probability of 1
    and an empty initial state, with an infection source south the infection
    spreads according to the expectations.
    """

    mv, dm = mtc.create_run_load(from_cfg="infection_dynamic.yml")

    # Get data
    grp = dm["multiverse"][0]["data/SEIRD"]
    data = grp["kind"]

    # Set kinds
    empty = 0
    susceptible = 1
    exposed = 2
    infected = 3
    source = 6

    # Check that the last row is an infection source
    assert (data.isel(y=0) == source).all()

    # In the initial time step all cells are empty except for the source row
    assert (data.isel(time=0, y=slice(1, None)) == empty).all()

    # In the first time step all cells are susceptible except for the source row
    assert (data.isel(time=1, y=slice(1, None)) == susceptible).all()

    # Check the lowest two rows above the source after 2 time steps
    assert (data.isel(time=2, y=2) == susceptible).all()
    assert (data.isel(time=2, y=1) == exposed).all()

    # Check that the second row is empty after three timesteps and the
    # first is infected.
    assert (data.isel(time=3, y=3) == susceptible).all()
    assert (data.isel(time=3, y=2) == exposed).all()
    assert (data.isel(time=3, y=1) == infected).all()


def test_densities_calculation():
    mv, dm = mtc.create_run_load(from_cfg="densities_calculation.yml")

    # Get the 2D densities dataset
    densities = dm["multiverse/0/data/SEIRD/densities"]

    # Assert that the added up densities are approximately equal to 1
    assert np.isclose(densities.sum("kind"), 1.0).all()

    # Densities should always be in interval [0., 1.]
    assert (0.0 <= densities).all()
    assert (densities <= 1.0).all()

    # Initially, in this case, no cell should be exposed or infected
    assert densities.sel(time=0, kind="exposed").item() == 0
    assert densities.sel(time=0, kind="infected").item() == 0


def test_counters():
    mv, dm = mtc.create_run_load(from_cfg="counters.yml")

    # Get the 2D counters
    counters = dm["multiverse/0/data/SEIRD/counts"]

    # There should be 11 counters
    assert counters.coords["label"].size == 11
    # NOTE If this fails, make sure to adapt not only this test, but also the
    #      model documentation and make sure that the Counters struct contains
    #      the correct labels!

    # Counters should be non-negative and grow only monotonically
    assert (counters >= 0).all()
    assert (counters.diff("time") >= 0).all()

    # For this configuration, some counters can only have specific values
    assert np.isin(
        counters.sel(label="susceptible_to_exposed_controlled"), [0, 3]
    ).all()


def test_exposure_control_add_inf():
    """
    Test that the exposure control via adding of a fixed number of infected
    cells at provided iteration steps works correctly.
    """
    mv, dm = mtc.create_run_load(from_cfg="exposure_control_add_inf.yml")

    # Get the kind of the cells
    data = dm["multiverse/0/data/SEIRD/kind"]

    # Set kinds
    empty = 0
    susceptible = 1
    exposed = 2
    infected = 3
    recovered = 4
    deceased = 5
    source = 6
    inert = 7

    for t in range(20):
        s = data.isel(time=t)

        unique, counts = np.unique(s, return_counts=True)
        d_counts = {u: c for u, c in zip(unique, counts)}
        print(d_counts)

        # At the start there should only be susceptible cells and empty spots
        if t < 5:
            assert exposed not in unique
            assert infected not in unique
            assert recovered not in unique
            assert deceased not in unique
            assert source not in unique
            assert inert not in unique

        # The time step after the exposure control should have a lot fewer
        # susceptible cells than in the previous iteration step because in every
        # exposure control step all susceptible cells are getting infected
        # and only the newly appearing susceptible cells remain
        if t in [6, 21, 41]:
            s_prev = data.isel(time=t - 1)

            unique_prev, counts_prev = np.unique(s_prev, return_counts=True)
            d_counts_prev = {u: c for u, c in zip(unique_prev, counts_prev)}

            assert d_counts[susceptible] < d_counts_prev[susceptible]


def test_exposure_control_change_p_random_inf():
    """
    Test that the exposure control via change of the random
    exposure probability works correctly.
    """
    mv, dm = mtc.create_run_load(
        from_cfg="exposure_control_change_p_random_inf.yml"
    )

    # Get the kind of the cells
    data = dm["multiverse/0/data/SEIRD/kind"]

    # Set the relevant kinds
    empty = 0
    susceptible = 1
    exposed = 2
    infected = 3

    for t in range(20):
        s = data.isel(time=t)

        unique, counts = np.unique(s, return_counts=True)
        d_counts = {u: c for u, c in zip(unique, counts)}

        # The probability for infection should be set to 0 the first 10 steps
        if t < 10:
            assert exposed not in unique
            assert infected not in unique

        # There are susceptible cells as well as exposed and infected cells
        # for 5 time steps.
        # However, there should be more susceptible cells than exposed or
        # infected ones
        if 11 == t:
            assert susceptible in unique
            assert exposed in unique
            assert d_counts[susceptible] > d_counts[exposed]

        # The exposure probability now is 1, so there should be more infected
        # cells than susceptible cells
        # NOTE that this test is a bit fragile in the sense that it can potentially
        #      be that it fails due to randomness.
        if t == 16:
            assert exposed in unique
            assert (d_counts[exposed] + d_counts[infected]) > d_counts[
                susceptible
            ]


def test_transmission_control():
    """
    Test that the transmission control via change of the probability p_transmit
    works correctly.
    """
    mv, dm = mtc.create_run_load(from_cfg="transmission_control.yml")

    # Get the kind of the cells
    data = dm["multiverse/0/data/SEIRD/kind"]

    # Set the relevant kinds
    empty = 0
    susceptible = 1
    exposed = 2
    infected = 3

    for t in range(20):
        s = data.isel(time=t)

        unique, counts = np.unique(s, return_counts=True)

        if t < 5:
            assert infected not in unique

        # Here, transmission control kicks in and leads to infected cells
        if t == 6:
            assert infected not in unique


def test_random_movement():
    """
    Test that cells move randomly around
    """
    mv, dm = mtc.create_run_load(from_cfg="movement_random.yml")

    # Get the kind of the cells
    data = dm["multiverse/0/data/SEIRD/kind"]

    # Set the relevant kinds
    empty = 0
    susceptible = 1
    exposed = 2
    infected = 3

    # A subset moves ...
    assert (data.isel(time=1) == susceptible).any()
    assert (data.isel(time=1) != data.isel(time=2)).any()
    assert (data.isel(time=2) != data.isel(time=3)).any()

    # ... and a subset of cells do not move
    assert (data.isel(time=1) == data.isel(time=2)).any()
    assert (data.isel(time=2) == data.isel(time=3)).any()


def test_directed_movement():
    """
    Test that cells move directed away from infected cells around them.
    """
    mv, dm = mtc.create_run_load(from_cfg="movement_directed.yml")

    # Get the kind of the cells
    data = dm["multiverse/0/data/SEIRD/kind"]

    infected = 3

    for t in range(10):
        if (data.isel(time=t) == infected).any():
            # If there are infected ones, there should be movement in the
            # high density population
            assert (data.isel(time=t) != data.isel(time=t + 1)).any()


def test_run_and_eval_cfgs():
    """Carries out all additional configurations that were specified alongside
    the default model configuration.

    This is done automatically for all run and eval configuration pairs that
    are located in subdirectories of the ``cfgs`` directory (at the same level
    as the default model configuration).
    If no run or eval configurations are given in the subdirectories, the
    respective defaults are used.

    See :py:meth:`~utopya.model.Model.run_and_eval_cfg_paths` for more info.
    """
    for cfg_name, cfg_paths in mtc.default_config_sets.items():
        print(f"\nRunning '{cfg_name}' configuration ...")

        mv, _ = mtc.create_run_load(
            from_cfg=cfg_paths.get("run"),
            parameter_space=dict(
                num_steps=50,
                SEIRD=dict(cell_manager=dict(grid=dict(resolution=48))),
            ),
        )
        mv.pm.plot_from_cfg(plots_cfg=cfg_paths.get("eval"))

        print(f"Succeeded running and evaluating '{cfg_name}'.\n")
