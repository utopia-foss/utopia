"""Tests of the initialisation of the PredatorPrey model"""

from itertools import chain

import h5py as h5
import numpy as np
import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class
mtc = ModelTest("PredatorPrey", test_file=__file__)

# Fixtures --------------------------------------------------------------------
# Define fixtures here


# Helpers ---------------------------------------------------------------------
# NOTE These helpers are also imported by other test modules for PredatorPrey


def model_cfg(**kwargs) -> dict:
    """Creates a dict that can update the config of the PredatorPrey model and
    the parameter space"""
    return dict(parameter_space=dict(PredatorPrey=dict(**kwargs)))


@pytest.fixture
def tmp_h5data_fpath(tmpdir) -> str:
    """Creates temporary hdf5 file with data for test_cell_states_from_file"""
    fpath = str(tmpdir.join("init_data.h5"))
    with h5.File(fpath, "w") as f:
        f.create_dataset("predator", data=np.ones((21, 21)))
        f.create_dataset("prey", data=np.zeros((21, 21)))
    return fpath


# Tests -----------------------------------------------------------------------


def test_basics():
    """Test the most basic features of the model, e.g. that it runs"""
    # Create a Multiverse using the default model configuration, except for
    # smaller grid size and one step
    mv, dm = mtc.create_run_load(from_cfg="initial_state.yml")

    # Assert that data was loaded, i.e. that data was written
    assert len(dm)


def test_cell_states_from_file(tmp_h5data_fpath):
    """Test setup with states coming from a file"""
    _, dm = mtc.create_run_load(
        from_cfg="init_from_file.yml",
        parameter_space=dict(
            PredatorPrey=dict(
                cell_states_from_file=dict(hdf5_file=tmp_h5data_fpath)
            )
        ),
    )

    for uni in dm["multiverse"].values():
        data = uni["data"]["PredatorPrey"]
        assert (data["predator"] == 1).all()
        assert (data["prey"] == 0).all()


def test_initial_state_random():
    """Test that the initial states are random.

    This also tests for the correct array shape, something that is not done in
    the other tests.
    """
    # Use the config file for common settings, change via additional kwargs
    mv, dm = mtc.create_run_load(
        from_cfg="initial_state.yml", perform_sweep=True
    )

    # For all universes, perform checks on the data
    for uni in dm["multiverse"].values():
        data = uni["data"]["PredatorPrey"]
        predator = data["predator"]
        prey = data["prey"]
        res_predator = data["resource_predator"]
        res_prey = data["resource_prey"]
        cell_params = uni["cfg"]["PredatorPrey"]["cell_manager"]["cell_params"]
        # Get the number of predators and prey for the population of the cell
        num_predator = predator.sum(dim=["x", "y"])
        num_prey = prey.sum(dim=["x", "y"])

        # Total number of cells can be extracted from dimension sizes
        num_cells = prey.sizes["x"] * prey.sizes["y"]

        # Get predator and prey probabilities
        p_predator = cell_params["p_predator"]
        p_prey = cell_params["p_prey"]

        # Number of cells should be larger or equal to the number of
        # prey or predators
        assert num_cells >= num_predator
        assert num_cells >= num_prey

        # Check that only a single step was written
        assert predator.sizes["time"] == 1
        assert prey.sizes["time"] == 1
        assert res_predator.sizes["time"] == 1
        assert res_prey.sizes["time"] == 1

        # Every individual gets 2 resource units
        assert num_predator == res_predator.sum(dim=["x", "y"]) / 2
        assert num_prey == res_prey.sum(dim=["x", "y"]) / 2

        # Population should be random; calculate the ratio and check if it is
        # within its standard deviation, which is 1/np.sqrt(12) for an
        # uniform distribution
        assert (
            p_predator * (1 - 1 / np.sqrt(12.0)) <= num_predator / num_cells
        ) and (
            num_predator / num_cells <= p_predator * (1 + 1 / np.sqrt(12.0))
        )
        assert (p_prey * (1 - 1 / np.sqrt(12.0)) <= num_prey / num_cells) and (
            num_prey / num_cells <= p_prey * (1 + 1 / np.sqrt(12.0))
        )


def test_invalid_arguments():
    """Test for correct behavior if the config file contains invalid
    arguments.
    """

    # repro_cost > repro_resource_requ for predators
    with pytest.raises(SystemExit):
        mtc.create_run_load(
            parameter_space=dict(
                PredatorPrey=dict(
                    predator=dict(repro_resource_requ=4.0, repro_cost=6.0)
                )
            )
        )

    # repro_cost > repro_resource_requ for prey
    with pytest.raises(SystemExit):
        mtc.create_run_load(
            parameter_space=dict(
                PredatorPrey=dict(
                    prey=dict(repro_resource_requ=4.0, repro_cost=6.0)
                )
            )
        )
