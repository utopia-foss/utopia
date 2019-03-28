"""Tests of the initialisation of the PredatorPrey model"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

from .test_init import model_cfg

# Configure the ModelTest class
mtc = ModelTest("PredatorPrey", test_file=__file__)

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

def test_cost_of_living_prey():
    """Test the cost of living of the prey"""
    # Create, run, and load a universe
    mv, dm = mtc.create_run_load(from_cfg="cost_of_living_prey.yml")

    # Get the data
    data = dm['multiverse'][0]['data']
    res_prey = data['PredatorPrey']['resource_prey']

    # Assert that life is costly...
    assert res_prey[0, 0, 0] == 2
    assert res_prey[1, 0, 0] == 1
    assert res_prey[2, 0, 0] == 0


def test_cost_of_living_Predator():
    """Test the cost of living of the the predator"""
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="cost_of_living_predator.yml")

    # Get the data
    data = dm['multiverse'][0]['data']
    pop = data['PredatorPrey']['population']
    res_pred = data['PredatorPrey']['resource_predator']

    # Assert that life is costly
    assert res_pred[0, 0, 0] == [2]
    assert res_pred[1, 0, 0] == [1]
    assert res_pred[2, 0, 0] == [0]


def test_eating_prey():
    """Test the basic eating functions of the prey"""

    # Prey should just take up resources every step and spend 1 resource for 
    # its cost of living

    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_eating_prey.yml")

    # Get the data
    data = dm['multiverse'][0]['data']
    res_prey = data['PredatorPrey']['resource_prey']

    # Assert that prey takes up resources every step and spends 1 resource
    # for its cost of living
    assert res_prey[0, 0, 0] == [2]
    assert res_prey[1, 0, 0] == [3]
    assert res_prey[2, 0, 0] == [4]


def test_predator_movement():
    """Test the predator movement functions
    
        The predator should move to prey in his neighborhood and afterwards move 
        around randomly to find prey.
    """
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_predator_movement.yml")

    # Get the data
    data = dm['multiverse'][0]['data']
    
    pop = data['PredatorPrey']['population'].astype(int)
    res_prey = data['PredatorPrey']['resource_prey']
    res_pred = data['PredatorPrey']['resource_predator']

    if pop[0, 0, 0] == 2:
        # in the last time step, depending on the update order, 
        # the predator may move twice
        assert np.all(pop[ :2, :, :] == [[[2], [1]], [[0],[2]]])
        assert np.all(res_pred[ :2, : , :] == [[[2], [0]], [[0],[40]]])
        assert np.all(res_prey[ :2, : , :] == [[[0], [2]], [[0],[0]]])
    
    else:
        # in the last time step, depending on the update order, 
        # the predator may move twice
        assert np.all(pop[ :2, :, :] == [[[1], [2]], [[2],[0]]])
        assert np.all(res_pred[ :2, : , :] == [[[0], [2]], [[40],[0]]]) 
        assert np.all(res_prey[ :2, : , :] == [[[2], [0]], [[0],[0]]])

    # Calculate the difference between successive steps after the prey has been 
    # eaten
    diff = np.diff(pop[2:], axis=0)

    # Check whether at some point the predator moved to another cell
    assert np.any(diff == 2)
    # Check that the predator starves in the last step
    assert np.sum(diff, axis=1)[-1] == -2
    

def test_prey_flee():
    """ Test whether the prey flees from the predator. Unfortunately, as the 
        update order is random, the prey can only flee, if its own cell has not 
        been updated, before a predator invades it.
    """
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_prey_flee.yml")
    
    data = dm['multiverse'][0]['data']
    pop = data['PredatorPrey']['population'].astype(int)

    diff = np.diff(pop, axis=0)

    assert np.any(diff == -1)
    

def test_prey_reproduction(): 
    """Test the basic interaction functions of the PredatorPrey model"""
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_prey_reproduction.yml")

    # Get the data
    uni = dm['multiverse'][0]
    data = uni['data']
    pop = data['PredatorPrey']['population']

    # Count the number of prey for each time step
    num_prey = [np.count_nonzero(p == 1) for p in pop]

    # Calculate total number of prey which can be created from the
    # amount of available resources
    # NOTE  The parameters are selected such that each prey can have one 
    #       offspring
    final_num_prey = num_prey[0] * 2

    # Check that not all preys reproduce after the first iteration step
    assert final_num_prey != num_prey[1]

    # Check that at the end every prey has reproduced
    assert(final_num_prey == num_prey[-1])