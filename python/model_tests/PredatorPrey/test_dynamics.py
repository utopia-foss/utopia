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
# - preators move to prey and prey tries to flee
# - taking up resources
# - reproduction

def test_basic_interactions():
    """Test the basic interaction functions of the PredatorPrey model"""

    # Test Cost of Living for Predator and Prey

    # Prey

    # Create a multiverse, run a single universe and save the data in the DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="specific_scenario.yml", perform_sweep=False,
                                **model_cfg(grid_size=[1, 1], prey_frac=1.0, 
                                            pred_frac=0, delta_e=0))

    pop = dm['uni'][0]['data']['PredatorPrey']['Population'].reshape(3, 1, 1)
    resource_prey = dm['uni'][0]['data']['PredatorPrey']['resource_prey'].reshape(3, 1, 1)

    assert resource_prey[1, 0, 0] == 1
    assert resource_prey[2, 0, 0] == 0
    assert np.all(pop[ : , 0, 0] == [1, 1, 0])

    # Predator

    # Create a multiverse, run a single universe and save the data in the DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="specific_scenario.yml", perform_sweep=False,
                                **model_cfg(grid_size=[1, 1], prey_frac=0.0, 
                                            pred_frac=1.0, delta_e=0))

    pop = dm['uni'][0]['data']['PredatorPrey']['Population'].reshape(3, 1, 1)
    resource_pred = dm['uni'][0]['data']['PredatorPrey']['resource_pred'].reshape(3, 1, 1)

    assert resource_pred[1, 0, 0] == 1
    assert resource_pred[2, 0, 0] == 0
    assert np.all(pop[ : , 0, 0] == [2, 2, 0])

    # Test the eating and the movement rule

    # Prey should just take up resources every step and spend 1 resource for its cost of living

    # Create a multiverse, run a single universe and save the data in the DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="specific_scenario.yml", perform_sweep=False, 
                                **model_cfg(grid_size=[1, 1], prey_frac=1.0, 
                                            pred_frac=0.0, delta_e=2))

    pop = dm['uni'][0]['data']['PredatorPrey']['Population'].reshape(3, 1, 1)
    resource_prey = dm['uni'][0]['data']['PredatorPrey']['resource_prey'].reshape(3, 1, 1)

    assert resource_prey[1, 0, 0] == 3
    assert resource_prey[2, 0, 0] == 4
    assert np.all(pop[ : , 0, 0] == [1, 1, 1])

    # Predator should move from one cell to the next until it succumbs to starvation

    # Create a multiverse, run a single universe and save the data in the DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="specific_scenario.yml", perform_sweep=False, 
                                **model_cfg(grid_size=[2, 1], prey_frac=0.0, pred_frac=0.5,
                                            delta_e=2, p_repro=0.0))

    pop = dm['uni'][0]['data']['PredatorPrey']['Population'].reshape(3, 2, 1)
    resource_pred = dm['uni'][0]['data']['PredatorPrey']['resource_pred'].reshape(3, 2, 1)
    resource_prey = dm['uni'][0]['data']['PredatorPrey']['resource_prey'].reshape(3, 2, 1)

    assert np.all(resource_pred[: , : , :] == [[[2], [0]], [[0],[1]], [[0], [0]]]) \
            or np.all(resource_pred[: , : , :] == [[[0], [2]], [[1],[0]], [[0], [0]]])

    assert np.all(resource_prey[: , :, :] == 0)

    assert np.all(pop[ : , :, :] == [[[2], [0]], [[0],[2]], [[0], [0]]]) \
            or np.all(pop[ : , :, :] == [[[0], [2]], [[2], [0]], [[0], [0]]])

    # Predator should move to prey in his neighborhood, as the order of the update is random, 
    # two cases are to be considered

    # Create a multiverse, run a single universe and save the data in the DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="specific_scenario.yml", perform_sweep=False, 
                                **model_cfg(grid_size=[2, 1], prey_frac=0.5, pred_frac=0.5, 
                                            delta_e=2, p_repro=0.0, p_flee=0.0))

    pop = dm['uni'][0]['data']['PredatorPrey']['Population'].reshape(3, 2, 1)
    resource_prey = dm['uni'][0]['data']['PredatorPrey']['resource_prey'].reshape(3, 2, 1)
    resource_pred = dm['uni'][0]['data']['PredatorPrey']['resource_pred'].reshape(3, 2, 1)

    if pop[0, 0, 0] == 2:
        assert np.all(pop[ : , :, :] == [[[2], [1]], [[0],[2]], [[2], [0]]]) 
        assert np.all(resource_pred[: , : , :] == [[[2], [0]], [[0],[3]], [[2], [0]]])
        assert np.all(resource_prey[: , : , :] == [[[0], [2]], [[0],[0]], [[0], [0]]])
    
    else:
        # in the last time step, depending on the update order, 
        # the predator may move twice
        assert np.all(pop[ : , :, :] == [[[1], [2]], [[2],[0]], [[0], [2]]]) \
                or np.all(pop[ : , :, :] == [[[1], [2]], [[2],[0]], [[2], [0]]]) 
        assert np.all(resource_pred[: , : , :] == [[[0], [2]], [[3],[0]], [[0], [2]]]) \
                or np.all(resource_pred[: , : , :] == [[[0], [2]], [[3],[0]], [[2], [0]]])
        assert np.all(resource_prey[: , : , :] == [[[2], [0]], [[0],[0]], [[0], [0]]])

    # Test wether the prey flees from the predator. Unfortunately, as the update
    # order is random, the prey can only flee, if its own cell has not been updated,
    # before a predator invades it.

    # Create the model_config dict to be able to update the parameter_space dict
    model_config = model_cfg(grid_size=[20, 1], prey_frac=0.95, pred_frac=0.05, delta_e=2, 
                             p_repro=0.0, p_flee=1.0)

    # Add a specific number of steps for this test
    model_config['parameter_space'].update({'num_steps':20})

    # Create a multiverse, run a single universe and save the data in the DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="specific_scenario.yml", perform_sweep=False, 
                                num_steps=20, **model_config)

    pop = dm['uni'][0]['data']['PredatorPrey']['Population'].reshape(21, 20, 1)
    pop = pop.astype(int)

    diff = np.diff(pop, axis=0)

    assert np.any(diff == -1)

    # Test the reproduction

    #Prey

    # Create a multiverse, run a single universe and save the data in the DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="specific_scenario.yml", perform_sweep=False, 
                                **model_cfg(grid_size=[2, 1], prey_frac=0.5, pred_frac=0, 
                                            delta_e=2, e_min=3, p_repro=1.0))

    pop = dm['uni'][0]['data']['PredatorPrey']['Population'].reshape(3, 2, 1)
    resource_prey = dm['uni'][0]['data']['PredatorPrey']['resource_prey'].reshape(3, 2, 1)

    assert np.all(resource_prey[: , : , :] == [[[2], [0]], [[1],[2]], [[2], [1]]]) \
            or np.all(resource_prey[: , : , :] == [[[0], [2]], [[2],[1]], [[1], [2]]])
    assert np.all(pop[ : , :, :] == [[[1], [0]], [[1],[1]], [[1], [1]]]) \
            or np.all(pop[ : , :, :] == [[[0], [1]], [[1],[1]], [[1], [1]]]) 
    
    # The function for the predator is identical

    
