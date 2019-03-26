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

def test_basic_interactions(): 
    """Test the basic interaction functions of the PredatorPrey model"""

    # Test Cost of Living for Predator and Prey

    # Prey

    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="specific_scenario.yml", 
                                 perform_sweep=False,
                                 **model_cfg(prey_frac=1.0, 
                                             pred_frac=0.0, delta_e_pred=0,delta_e_prey=0))

    
    data = dm['multiverse'][0]['data']

    pop = data['PredatorPrey']['population']#.reshape(3, 1, 1)
    res_prey = data['PredatorPrey']['resource_prey']
    
    print(res_prey)

    assert res_prey[0, 0, 0] == 2
    assert res_prey[1, 0, 0] == 1
    assert res_prey[2, 0, 0] == 0

    # Predator

    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="specific_scenario.yml", 
                                 perform_sweep=False,
                                 **model_cfg(prey_frac=0.0, pred_frac=1.0, 
                                                    predprey_frac=0.0))

    data = dm['multiverse'][0]['data']

    pop = data['PredatorPrey']['population']
    res_pred = data['PredatorPrey']['resource_predator']

    assert res_pred[0, 0, 0] == [2]
    assert res_pred[1, 0, 0] == [1]
    assert res_pred[2, 0, 0] == [0]

    # Test the eating and the movement rule

    # Prey should just take up resources every step and spend 1 resource for 
    # its cost of living

    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="specific_scenario.yml", 
                                 perform_sweep=False, 
                                 **model_cfg(delta_e_pred=2.0,delta_e_prey=2.0, prey_frac=1.0, 
                                    pred_frac=0.0, predprey_frac=0.0))

    data = dm['multiverse'][0]['data']

    pop = data['PredatorPrey']['population']
    res_prey = data['PredatorPrey']['resource_prey']

    assert res_prey[0, 0, 0] == [2]
    assert res_prey[1, 0, 0] == [3]
    assert res_prey[2, 0, 0] == [4]

    # Predator should move to prey in his neighborhood and afterwards move 
    # around randomly to find prey

    # Create the model_config dict to enable updating the parameter_space dict
    model_conf = model_cfg(prey_frac=0.5, pred_frac=0.5, 
                            delta_e_pred=39,delta_e_prey=39, p_repro_pred=0.0,p_repro_prey=0.0, p_flee=0.0,
                            e_max_pred=200,e_max_prey=200, 
                            cell_manager=dict(resolution=1))

    # Add a specific number of steps for this test
    model_conf['parameter_space'].update({'num_steps':41})

    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="specific_scenario.yml", 
                                 perform_sweep=False, **model_conf)

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

    # Calculate the difference between succesive steps after the prey has been 
    # eaten
    diff = np.diff(pop[2:], axis=0)

    # Check whether at some point the predator moved to another cell
    assert np.any(diff == 2)
    # Check that the predator starves in the last step
    assert np.sum(diff, axis=1)[-1] == -2
    


    # Test whether the prey flees from the predator. Unfortunately, as the 
    # update order is random, the prey can only flee, if its own cell has not 
    # been updated, before a predator invades it.

    # Create the model_config dict to enable updating the parameter_space dict
    model_config = model_cfg(prey_frac=0.95, pred_frac=0.05, delta_e_pred=2,
                            delta_e_prey=2,
                             p_repro_pred=0.0,p_repro_prey=0, p_flee=1.0, 
                             space=dict(extent=[30,1]), 
                             cell_manager=dict(resolution=1))

    # Add a specific number of steps for this test
    model_config['parameter_space'].update({'num_steps':30})

    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="specific_scenario.yml", 
                                 perform_sweep=False, **model_config)
    
    data = dm['multiverse'][0]['data']

    pop = data['PredatorPrey']['population'].astype(int)

    diff = np.diff(pop, axis=0)

    assert np.any(diff == -1)

    # Test the reproduction

    #Prey

    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="specific_scenario.yml", 
                                 perform_sweep=False, 
                                 **model_cfg(prey_frac=0.5, 
                                             pred_frac=0, delta_e_pred=2,
                                             delta_e_prey=2, e_min_pred=3,
                                             e_min_prey=3, 
                                             p_repro_pred=1.0,p_repro_prey=1.0))

    data = dm['multiverse'][0]['data']

    pop = data['PredatorPrey']['population']
    res_prey = data['PredatorPrey']['resource_prey']
                         
    assert np.all(res_prey[: , : , :] == [[[2], [0]], [[1],[2]], [[2], [1]]]) \
        or np.all(res_prey[: , : , :] == [[[0], [2]], [[2],[1]], [[1], [2]]])
    assert np.all(pop[ : , :, :] == [[[1], [0]], [[1],[1]], [[1], [1]]]) \
        or np.all(pop[ : , :, :] == [[[0], [1]], [[1],[1]], [[1], [1]]]) 
    
    # The function for the predator is identical

    
