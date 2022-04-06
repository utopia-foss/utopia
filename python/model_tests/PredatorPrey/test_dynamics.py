"""Tests of the initialisation of the PredatorPrey model"""

import h5py as h5
import numpy as np
import pytest
import xarray as xr

from utopya.testtools import ModelTest

from .test_init import model_cfg

# Configure the ModelTest class
mtc = ModelTest("PredatorPrey", test_file=__file__)

# Utility functions -----------------------------------------------------------


# Test all the basic functions that determine the dynamics of the model:
# - cost_of_living
# - predators move to prey and prey tries to flee
# - taking up resources
# - reproduction


# Fixtures --------------------------------------------------------------------
# Define fixtures here


@pytest.fixture
def tmp_h5data_fpath(tmpdir) -> str:
    """Creates temporary hdf5 file with data for test_prey_flee"""
    # Create predator and prey data
    predator_data = [1, 0]
    prey_data = [0, 1]
    fpath = str(tmpdir.join("flee_data.h5"))
    with h5.File(fpath, "w") as f:
        f.create_dataset("predator", data=predator_data)
        f.create_dataset("prey", data=prey_data)
    return fpath


# Tests -----------------------------------------------------------------------


def test_cost_of_living_prey():
    """Test the cost of living of the prey"""
    # Create, run, and load a universe
    mv, dm = mtc.create_run_load(from_cfg="cost_of_living_prey.yml")

    for uni in dm["multiverse"].values():
        # Get the data
        data = uni["data"]["PredatorPrey"]
        prey = data["prey"]
        res_prey = data["resource_prey"]

        # Since no predator is present, prey does not flee and does not move.
        # So the resources of every prey, staying on its cell should decrease
        # over time.

        # Assert decreasing resources of prey
        assert (res_prey.diff("time") <= 0).all()

        # Assert that in the end all prey is dead due to starvation
        assert (prey.isel(time=-1) == 0).all()


def test_cost_of_living_Predator():
    """Test the cost of living of the the predator"""
    # Create a multiverse, run a single universe and save the data in the
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="cost_of_living_predator.yml")

    for uni in dm["multiverse"].values():
        # Get the data
        data = uni["data"]["PredatorPrey"]
        predator = data["predator"]
        res_predator = data["resource_predator"]

        # Assert that life is costly
        assert (res_predator.diff("time") <= 0).all()

        # Assert that in the end all predators starved to death
        assert (predator.isel(time=-1) == 0).all()


def test_eating_prey():
    """Test the basic eating functions of the prey"""

    # Prey should just take up resources every step and spend 1 resource for
    # its cost of living

    # Create a multiverse, run a single universe and save the data in the
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_eating_prey.yml")

    for uni in dm["multiverse"].values():
        # Get the data
        data = uni["data"]["PredatorPrey"]
        res_prey = data["resource_prey"]

        # Assert that prey takes up exactly one net resources every step.
        # NOTE Requires initial resources of 2 and cost of living of 1 and
        #      resource intake of 2, resulting in 1 net resource.
        assert (res_prey.diff("time") == 1).all()


def test_predator_movement():
    """Test the predator movement functions

    The predator should move to prey in his neighborhood and afterwards move
    around randomly to find prey.
    """
    # Create a multiverse, run a single universe and save the data in the
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_predator_movement.yml")

    # Get the data
    for uni in dm["multiverse"].values():
        data = uni["data"]["PredatorPrey"]

        prey = data["prey"]
        predator = data["predator"]

        res_prey = data["resource_prey"]
        res_pred = data["resource_predator"]

        # Check that predators moves
        # Calculate the difference between successive steps
        assert (predator.diff("time") != 0).any()


def test_predator_conservation():
    """If life costs and reproduction chance are both zero, the number of
    predators should be conserved.
    """

    mv, dm = mtc.create_run_load(from_cfg="test_predator_conservation.yml")

    for uni in dm["multiverse"].values():
        data = uni["data"]["PredatorPrey"]
        predator = data["predator"]

        # Assert that in all timesteps the sum of predators is conserved
        assert (predator.sum(dim=["x", "y"]).diff("time") == 0).all()


def test_prey_conservation():
    """If life costs and reproduction chance are both zero, the number of
    prey should be conserved if there are no predators around.
    """

    mv, dm = mtc.create_run_load(from_cfg="test_prey_conservation.yml")

    for uni in dm["multiverse"].values():
        data = uni["data"]["PredatorPrey"]
        prey = data["prey"]

        # Assert that in all timesteps the sum of prey is conserved
        assert (prey.sum(dim=["x", "y"]).diff("time") == 0).all()


def test_prey_flee(tmp_h5data_fpath):
    """Test whether the prey flees from the predator. If the flee chance is
    100% and the resource intake, reproduction chance and cost of living
    are zero, predator and prey are just switching places for ever. So
    the number of predators and prey is conserved in this system.
    """

    # Create a multiverse, run a single universe and save the data in the
    # DataManager dm
    mv, dm = mtc.create_run_load(
        from_cfg="test_prey_flee.yml",
        parameter_space=dict(
            PredatorPrey=dict(
                cell_states_from_file=dict(hdf5_file=tmp_h5data_fpath)
            )
        ),
    )

    for uni in dm["multiverse"].values():
        data = uni["data"]["PredatorPrey"]
        predator = data["predator"]
        prey = data["prey"]

        # Assert that prey and predator both are conserved for this scenario
        assert (prey.sum(dim=["x", "y"]).diff("time") == 0).all()
        assert (predator.sum(dim=["x", "y"]).diff("time") == 0).all()


def test_hunting():
    """Test if prey completely vanishes over time, if the flee chance,
    life costs, resource intake and reproduction chance are zero.
    """

    # Create a multiverse, run a single universe and save the data in the
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_hunting.yml")

    for uni in dm["multiverse"].values():
        data = uni["data"]["PredatorPrey"]
        predator = data["predator"]
        prey = data["prey"]

        # The sum of predators should be conserved
        assert (predator.sum(dim=["x", "y"]).diff("time") == 0).all()

        # The sum of prey should (not necessarily strictly) monotonically
        # decrease
        assert (prey.sum(dim=["x", "y"]).diff("time") <= 0).all()

        # For sufficient long time prey should went extinct.
        assert (prey.isel(time=-1) == 0).all()


def test_prey_reproduction():
    """Test the basic interaction functions of the PredatorPrey model"""
    # Create a multiverse, run a single universe and save the data in the
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_prey_reproduction.yml")

    for uni in dm["multiverse"].values():
        data = uni["data"]["PredatorPrey"]
        prey = data["prey"]
        predator = data["predator"]

        # There are no predators
        assert (predator == 0).all()

        # NOTE  The parameters are selected such that each prey can have one
        #       offspring. So the amount of prey in the last step should be
        #       twice the amount in the initial step.

        # Check that not all preys reproduce after the first iteration step
        assert 2 * prey.isel(time=0).sum(dim=["x", "y"]) != prey.isel(
            time=1
        ).sum(dim=["x", "y"])

        # Check that at the end every prey has reproduced
        assert 2 * prey.isel(time=0).sum(dim=["x", "y"]) == prey.isel(
            time=-1
        ).sum(dim=["x", "y"])


def test_predator_reproduction():
    """Test the basic interaction functions of the PredatorPrey model"""
    # Create a multiverse, run a single universe and save the data in the
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_predator_reproduction.yml")

    for uni in dm["multiverse"].values():
        data = uni["data"]["PredatorPrey"]
        predator = data["predator"]

        # NOTE  The parameters are selected such that each predator can have
        #       one offspring. So the amount of predators in the last step
        #       should be twice the amount in the initial step.

        # Check that not all predators reproduce after the first iteration step
        assert 2 * predator.isel(time=0).sum(dim=["x", "y"]) != predator.isel(
            time=1
        ).sum(dim=["x", "y"])

        # Check that at the end every predator has reproduced
        assert 2 * predator.isel(time=0).sum(dim=["x", "y"]) == predator.isel(
            time=-1
        ).sum(dim=["x", "y"])
