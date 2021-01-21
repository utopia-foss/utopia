"""Tests of the initialisation of the PredatorPreyPlant model"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

from .test_init import model_cfg

# Configure the ModelTest class
mtc = ModelTest("PredatorPreyPlant", test_file=__file__)

# Utility functions -----------------------------------------------------------

def assert_eq(a, b, *, epsilon=1e-6):
    """Assert that two quantities are equal within a numerical epsilon range"""
    assert(np.absolute(a-b) < epsilon)

# Tests -----------------------------------------------------------------------

# Test all the basic functions that determine the dynamics of the model:
# - cost_of_living
# - predators move to prey and prey tries to flee
# - taking up resources
# - reproduction

# TODO Use xarray for all tests!

def test_cost_of_living_prey():
    """Test the cost of living of the prey"""
    # Create, run, and load a universe
    mv, dm = mtc.create_run_load(from_cfg="cost_of_living_prey.yml")

    for uni_no, uni in dm['multiverse'].items():
        # Get the data
        data = uni['data']['PredatorPreyPlant']
        prey = data["prey"]
        res_prey = data['resource_prey']    

        # Assert that in the end all prey is dead due to starvation
        assert np.all(prey.isel(time=-1) == 0)
        assert np.all(res_prey.diff(dim="time") <= 1.)


def test_cost_of_living_Predator():
    """Test the cost of living of the the predator"""
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="cost_of_living_predator.yml")

    for uni_no, uni in dm['multiverse'].items():
        # Get the data
        data = uni['data']['PredatorPreyPlant']
        pred = data['predator']
        res_pred = data['resource_predator']

        # Assert that life is costly and all predators die in the end
        assert np.all(pred.isel(time=-1) == 0)
        assert np.all(res_pred.diff(dim="time") <= 1.)


def test_eating_prey():
    """Test the basic eating functions of the prey"""

    # Prey should just take up resources every step and spend 1 resource for 
    # its cost of living

    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_eating_prey.yml")

    for uni_no, uni in dm['multiverse'].items():
        # Get the data
        data = uni['data']['PredatorPreyPlant']
        res_prey = data['resource_prey']

        # Assert that prey takes up resources every step and spends 1 resource
        # for its cost of living
        # NOTE Requires initial resources of 2 and cost of living of 1 and 
        #      resource intake of 2
        for i, r in enumerate(res_prey):
            unique = np.unique(r)
            assert unique == 2+i
    

def test_prey_reproduction(): 
    """Test the basic interaction functions of the PredatorPreyPlant model"""
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_prey_reproduction.yml")

    for uni_no, uni in dm['multiverse'].items():           
        # Get the data
        data = uni['data']['PredatorPreyPlant']
        prey = data['prey']
        predator = data['predator']

        # There are no predators
        assert np.all(predator) == 0

        # Count the number of prey for each time step
        num_prey = prey.sum(dim=['x', 'y'])
        # Calculate total number of prey which can be created from the
        # amount of available resources
        # NOTE  The parameters are selected such that each prey can have one 
        #       offspring
        expected_final_num_prey = num_prey[0] * 2

        # Check that not all preys reproduce after the first iteration step
        assert expected_final_num_prey != num_prey[1]

        # Check that at the end every prey has reproduced
        assert expected_final_num_prey == num_prey[-1]


def test_predator_reproduction(): 
    """Test the basic interaction functions of the PredatorPreyPlant model"""
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_predator_reproduction.yml")

    for uni_no, uni in dm['multiverse'].items():
        # Get the data
        data = uni['data']['PredatorPreyPlant']
        predator = data['predator']

        # Count the number of predators for each time step
        num_predator = predator.sum(dim=['x', 'y'])

        # Calculate total number of predator which can be created from the
        # amount of available resources
        # NOTE  The parameters are selected such that each predator can have one 
        #       offspring
        expected_final_num_predator = num_predator[0] * 2

        # Check that not all predators reproduce after the first iteration step
        assert expected_final_num_predator != num_predator[1]

        # Check that at the end every predator has reproduced
        assert(expected_final_num_predator == num_predator[-1])


def test_plant_deterministic_mode(): 
    """Test the basic interaction functions of the PredatorPreyPlant model"""
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_plant_deterministic_mode.yml")

    for uni_no, uni in dm['multiverse'].items():
        # Get the data
        data = uni['data']['PredatorPreyPlant']
        plant = data['plant']

        # Count the number of predators for each time step
        num_plant = plant.sum(dim=['x', 'y'])

        assert num_plant[20] == 0
        assert num_plant[21] == 100


def test_plant_stochastic_mode(): 
    """Test the basic interaction functions of the PredatorPreyPlant model"""
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_plant_stochastic_mode_1.yml")
    
    for uni_no, uni in dm['multiverse'].items():
        # Get the data
        data = uni['data']['PredatorPreyPlant']
        plant = data['plant']

        # Count the number of predators for each time step
        num_plant = plant.sum(dim=['x', 'y'])
        assert num_plant[-1] == 0


    mv, dm = mtc.create_run_load(from_cfg="test_plant_stochastic_mode_2.yml")
    
    for uni_no, uni in dm['multiverse'].items():
        # Get the data
        data = uni['data']['PredatorPreyPlant']
        plant = data['plant']

        # Count the number of predators for each time step
        num_plant = plant.sum(dim=['x', 'y'])
        assert num_plant[-1] == 100
