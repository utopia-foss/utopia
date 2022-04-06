"""Tests of the initialisation of the PredatorPreyPlant model"""

from itertools import chain

import h5py as h5
import numpy as np
import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class
mtc = ModelTest("PredatorPreyPlant", test_file=__file__)


# Fixtures --------------------------------------------------------------------
# Define fixtures here


@pytest.fixture
def tmp_h5data_fpath(tmpdir) -> str:
    """Creates temporary hdf5 file with data for test_cell_states_from_file"""
    fpath = str(tmpdir.join("init_data.h5"))

    with h5.File(fpath, "w") as f:
        f.create_dataset("predator", data=np.ones((21, 21)))
        f.create_dataset("prey", data=np.zeros((21, 21)))
        f.create_dataset("plant", data=np.ones((21, 21)))

    return fpath


@pytest.fixture
def tmp_h5data_fpath_invalid_data(tmpdir) -> str:
    """Creates temporary hdf5 file with data for test_invalid_data_from_h5_file"""
    # Crate invalid datasets
    predator_data = [2, 2]
    prey_data = [2, 2]
    plant_data = [2, 2]

    # Construct h5 file path
    fpath = str(tmpdir.join("invalid_init_data.h5"))
    with h5.File(fpath, "w") as f:
        f.create_dataset("predator", data=predator_data)
        f.create_dataset("prey", data=prey_data)
        f.create_dataset("plant", data=plant_data)

    return fpath


# Helpers ---------------------------------------------------------------------
# NOTE These helpers are also imported by other test modules for PredatorPreyPlant


def model_cfg(**kwargs) -> dict:
    """Creates a dict that can update the config of the PredatorPreyPlant model and
    the parameter space"""
    return dict(parameter_space=dict(PredatorPreyPlant=dict(**kwargs)))


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
            PredatorPreyPlant=dict(
                cell_states_from_file=dict(hdf5_file=tmp_h5data_fpath)
            )
        ),
    )

    for uni in dm["multiverse"].values():
        data = uni["data"]["PredatorPreyPlant"]
        assert (data["predator"] == 1).all()
        assert (data["prey"] == 0).all()
        assert (data["plant"] == 1).all()


def test_initial_state_random():
    """Test that the initial states are random.

    This also tests for the correct array shape, something that is not done in
    the other tests.
    """
    # Use the config file for common settings, change via additional kwargs
    _, dm = mtc.create_run_load(
        from_cfg="initial_state.yml", perform_sweep=True
    )

    # For all universes, perform checks on the data
    for uni in dm["multiverse"].values():
        data = uni["data"]["PredatorPreyPlant"]
        cfg = uni["cfg"]["PredatorPreyPlant"]
        cell_params = cfg["cell_manager"]["cell_params"]
        predator = data["predator"]
        prey = data["prey"]
        plant = data["plant"]

        # Get the number of predators and prey for the population of the cell
        num_predator = predator.sum(dim=["x", "y"])
        num_prey = prey.sum(dim=["x", "y"])
        num_plant = plant.sum(dim=["x", "y"])

        # Total number of cells can be extracted from shape
        num_cells = prey.sizes["x"] * prey.sizes["y"]

        # Get predator and prey probabilities
        p_predator = cell_params["p_predator"]
        p_prey = cell_params["p_prey"]

        # Number of cells should be prey + predators + empty, every cell should
        # either be empty or populated by either predator or prey
        assert num_cells >= num_prey
        assert num_cells >= num_predator
        assert num_cells >= num_plant

        # Check that only a single step was written
        assert data["prey"].sizes["time"] == 1
        assert data["predator"].sizes["time"] == 1
        assert data["plant"].sizes["time"] == 1
        assert data["resource_prey"].sizes["time"] == 1
        assert data["resource_predator"].sizes["time"] == 1

        # # Every individual gets between 2 resource units and 8
        res_prey = data["resource_prey"].sel({"time": 0})
        res_predator = data["resource_predator"].sel({"time": 0})

        # Valid values for species resources are 2 up to 8.
        # A zero indicates an empty space.
        assert (res_prey >= 0).all()
        assert (res_prey != 1).all()
        assert (res_prey <= 8).all()

        assert (res_predator >= 0).all()
        assert (res_predator != 1).all()
        assert (res_predator <= 8).all()

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


def test_invalid_argument():
    """Test if the simulation stops if the config file contains invalid
    arguments.
    """

    # Test invalid initial resources for predator and prey. For the test the
    # minimal initial resources are larger than the maximal, which should
    # result in a system exit.
    with pytest.raises(SystemExit):
        mtc.create_run_load(
            from_cfg="init_invalid_argument_init_resources.yml"
        )

    with pytest.raises(SystemExit):
        mtc.create_run_load(
            from_cfg="init_invalid_argument_repro_cost_and_requ.yml"
        )


def test_invalid_data_from_h5_file(tmp_h5data_fpath_invalid_data):
    """Test if the simulation stops if a h5 file contains data unequal 0 or 1."""

    with pytest.raises(SystemExit):
        mtc.create_run_load(
            from_cfg="init_invalid_data_from_h5_file.yml",
            perform_sweep=True,
            parameter_space=dict(
                PredatorPreyPlant=dict(
                    cell_states_from_file=dict(
                        hdf5_file=tmp_h5data_fpath_invalid_data
                    )
                )
            ),
        )
