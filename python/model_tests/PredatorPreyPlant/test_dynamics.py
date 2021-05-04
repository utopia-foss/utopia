"""Tests of the initialisation of the PredatorPreyPlant model"""

import numpy as np
import pytest
import h5py as h5

from utopya.testtools import ModelTest

from .test_init import model_cfg

# Configure the ModelTest class
mtc = ModelTest("PredatorPreyPlant", test_file=__file__)

# Utility functions -----------------------------------------------------------

def assert_eq(a, b, *, epsilon=1e-6):
    """Assert that two quantities are equal within a numerical epsilon range"""
    assert(np.absolute(a-b) < epsilon)


# Fixtures --------------------------------------------------------------------
# Define fixtures here

@pytest.fixture
def tmp_h5data_fpath(tmpdir) -> str:
    """Creates temporary hdf5 file with data for test_prey_flee"""
    # Create predator and prey data
    predator_data = ([1, 0])
    prey_data = ([0, 1])
    fpath = str(tmpdir.join("flee_data.h5"))
    with h5.File(fpath, "w") as f:
        f.create_dataset("predator", data=predator_data)
        f.create_dataset("prey", data=prey_data)
    return fpath


# Tests -----------------------------------------------------------------------

# Test all the basic functions that determine the dynamics of the model:
# - cost_of_living
# - predators move to prey and prey tries to flee
# - taking up resources
# - reproduction

def test_cost_of_living_prey():
    """Test the cost of living of the prey"""
    # Create, run, and load a universe
    mv, dm = mtc.create_run_load(from_cfg="cost_of_living_prey.yml")

    for uni in dm['multiverse'].values():
        # Get the data
        data = uni['data']['PredatorPreyPlant']
        prey = data["prey"]
        res_prey = data['resource_prey']    

        # Assert that in the end all prey is dead due to starvation
        assert(prey.isel(time=-1) == 0).all()
        assert(res_prey.diff(dim="time") <= 1.).all()


def test_cost_of_living_Predator():
    """Test the cost of living of the the predator"""
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="cost_of_living_predator.yml")

    for uni in dm['multiverse'].values():
        # Get the data
        data = uni['data']['PredatorPreyPlant']
        pred = data['predator']
        res_pred = data['resource_predator']

        # Assert that life is costly and all predators die in the end
        assert(pred.isel(time=-1) == 0).all()
        assert(res_pred.diff(dim="time") <= 1.).all()


def test_prey_flee(tmp_h5data_fpath):
    """ Test whether the prey flees from the predator. If the flee chance is 
        100% and the resource intake, reproduction chance and cost of living 
        are zero, predator and prey are just switching places for ever. So 
        the number of predators and prey is conserved in this system.
    """

    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_prey_flee.yml",
                                parameter_space=dict(
                                    PredatorPreyPlant=dict(
                                        cell_states_from_file=dict(
                                            hdf5_file=tmp_h5data_fpath
                                        )
                                    )
                                )
                            )
    
    for uni in dm['multiverse'].values():
        data = uni['data']['PredatorPreyPlant']
        predator = data['predator']
        prey = data['prey']

        sum_of_predators = predator.sum(dim=['x', 'y'])
        sum_of_prey = prey.sum(dim=['x', 'y'])

        assert(sum_of_predators == sum_of_predators.sel({'time': 0})).all()
        assert(sum_of_prey == sum_of_prey.sel({'time': 0})).all()


def test_eating_prey():
    """Test the basic eating functions of the prey"""

    # Prey should just take up resources every step and spend 1 resource for 
    # its cost of living

    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_eating_prey.yml")

    for uni in dm['multiverse'].values():
        # Get the data
        data = uni['data']['PredatorPreyPlant']
        res_prey = data['resource_prey']

        # Assert that prey takes up resources every step and spends 1 resource
        # for its cost of living
        # NOTE Requires initial resources of 2 and cost of living of 1 and 
        #      resource intake of 2
        assert(res_prey.diff('time') == 1).all() 
    

def test_prey_reproduction(): 
    """Test the basic interaction functions of the PredatorPreyPlant model"""
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_prey_reproduction.yml")

    for uni in dm['multiverse'].values():           
        # Get the data
        data = uni['data']['PredatorPreyPlant']
        prey = data['prey']
        predator = data['predator']

        # There are no predators
        assert(predator == 0).all()

        # Count the number of prey for each time step
        num_prey = prey.sum(dim=['x', 'y'])

        # Calculate total number of prey which can be created from the
        # amount of available resources
        # NOTE  The parameters are selected such that each prey can have one 
        #       offspring
        expected_final_num_prey = num_prey.sel({'time': 0}) * 2

        # Check that not all preys reproduce after the first iteration step
        assert expected_final_num_prey != num_prey.sel({'time': 1})

        # Check that at the end every prey has reproduced
        assert expected_final_num_prey == num_prey.isel(time=-1)


def test_prey_conservation():
    """ If life costs and reproduction chance are both zero, the number of 
        prey should be conserved if there are no predators around.
    """

    mv, dm = mtc.create_run_load(from_cfg="test_prey_conservation.yml")

    for uni in dm['multiverse'].values():
        data = uni['data']['PredatorPreyPlant']
        prey = data['prey']
        sum_of_prey = prey.sum(dim=['x', 'y'])

        assert(sum_of_prey == sum_of_prey.sel({'time': 0})).all()


def test_predator_reproduction(): 
    """Test the basic interaction functions of the PredatorPreyPlant model"""
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_predator_reproduction.yml")

    for uni in dm['multiverse'].values():
        # Get the data
        data = uni['data']['PredatorPreyPlant']
        predator = data['predator']

        # Count the number of predators for each time step
        num_predator = predator.sum(dim=['x', 'y'])

        # Calculate total number of predator which can be created from the
        # amount of available resources
        # NOTE  The parameters are selected such that each predator can have one 
        #       offspring
        expected_final_num_predator = num_predator.sel({'time': 0}) * 2

        # Check that not all predators reproduce after the first iteration step
        assert(expected_final_num_predator != num_predator.sel({'time': 1}))

        # Check that at the end every predator has reproduced
        assert(expected_final_num_predator == num_predator.isel(time=-1))


def test_predator_conservation():
    """ If life costs and reproduction chance are both zero, the number of 
        predators should be conserved.
    """

    mv, dm = mtc.create_run_load(from_cfg="test_predator_conservation.yml")

    for uni in dm['multiverse'].values():
        data = uni['data']['PredatorPreyPlant']
        predator = data['predator']
        sum_of_predators = predator.sum(dim=['x', 'y'])

        assert(sum_of_predators == sum_of_predators.sel({'time': 0})).all()


def test_plant_deterministic_mode(): 
    """ Test the regrowth of the plant species for the deterministic 
        regrowth model. Every cell has a prey entity on it, consuming all
        plants in the first step and then starving to death. 
        After 20 steps all plants should be regrown.
    """

    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_plant_deterministic_mode.yml")

    for uni in dm['multiverse'].values():
        # Get the data
        data = uni['data']['PredatorPreyPlant']
        plant = data['plant']

        # Count the number of plants for each time step
        num_plant = plant.sum(dim=['x', 'y'])

        # Total number of cells can be extracted from data shape
        num_cells = plant.sizes['x'] * plant.sizes['y']

        assert num_plant.sel({'time': 20}) == 0
        assert num_plant.sel({'time': 21}) == num_cells


def test_plant_stochastic_mode(): 
    """ Test the regrowth of the plant species for the stochastic mode. 
        Every cell include a prey, consuming all plants. 
        In the first sweep the regrowth probability is 0%, resulting in all 
        prey starves to death. Because there is no regrowth there should be
        no plants at the last time step.
        In the second sweep the regrowth probability is 100%. This way cells
        has always plants on it. 
    """
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_plant_stochastic_mode.yml", 
                                perform_sweep=True)
    
    for uni in dm['multiverse'].values():
        # Get the data
        data = uni['data']['PredatorPreyPlant']
        plant = data['plant']
        plant_cfg = uni['cfg']['PredatorPreyPlant']['plant']

        # Get plant regeneration probability from cfg
        p_plant_reg = plant_cfg['regen_prob']

        # Total number of cells can be extracted from data shape
        num_cells = plant.sizes['x'] * plant.sizes['y']

        # Count the number of plants for each time step
        num_plant = plant.sum(dim=['x', 'y'])
        assert num_plant.isel(time=-1) == num_cells * p_plant_reg