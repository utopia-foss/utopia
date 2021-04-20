"""Tests of the initialisation of the PredatorPrey model"""

import numpy as np
import h5py as h5
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
# TODO Improve these by using more xarray functionality, also to arrive at a
#      more readable representation (instead of the np.unique stuff)


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

def test_cost_of_living_prey():
    """Test the cost of living of the prey"""
    # Create, run, and load a universe
    mv, dm = mtc.create_run_load(from_cfg="cost_of_living_prey.yml")

    # Get the data
    data = dm['multiverse'][0]['data']['PredatorPrey']
    prey = data["prey"]

    prey.stack(dim=['x', 'y'])
    
    res_prey = data['resource_prey']
    res_prey.stack(dim=['x', 'y'])

    # Assert that life is costly...
    # NOTE Requires initial resources of 2 and cost of living of 1
    for i, r in enumerate(res_prey):
        unique = np.unique(r)
        if 2-i >= 0:
            assert unique == 2-i

    # Assert that in the end all prey is dead due to starvation
    assert np.unique(prey.isel(time=-1)) == 0


def test_cost_of_living_Predator():
    """Test the cost of living of the the predator"""
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="cost_of_living_predator.yml")

    # Get the data
    data = dm['multiverse'][0]['data']['PredatorPrey']
    res_predator = data['resource_predator']

    # Assert that life is costly
    for i, r in enumerate(res_predator):
        unique = np.unique(r)
        assert unique == 2-i


def test_eating_prey():
    """Test the basic eating functions of the prey"""

    # Prey should just take up resources every step and spend 1 resource for 
    # its cost of living

    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_eating_prey.yml")

    # Get the data
    data = dm['multiverse'][0]['data']['PredatorPrey']
    res_prey = data['resource_prey']

    # Assert that prey takes up resources every step and spends 1 resource
    # for its cost of living
    # NOTE Requires initial resources of 2 and cost of living of 1 and 
    #      resource intake of 2
    for i, r in enumerate(res_prey):
        unique = np.unique(r)
        assert unique == 2+i


def test_predator_movement():
    """Test the predator movement functions
    
        The predator should move to prey in his neighborhood and afterwards move 
        around randomly to find prey.
    """
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_predator_movement.yml")

    # Get the data
    data = dm['multiverse'][0]['data']['PredatorPrey']
    
    prey = data['prey']
    predator = data['predator']

    res_prey = data['resource_prey']
    res_pred = data['resource_predator']

    # Check that predators moves
    # Calculate the difference between successive steps
    diff = np.diff(predator, axis=0)

    # Check whether at some point the predator moved to another cell
    assert np.any(diff != 0)
    

def test_predator_conservation():
    """ If life costs and reproduction chance are both zero, the number of 
        predators should be conserved.
    """

    mv, dm = mtc.create_run_load(from_cfg="test_predator_conservation.yml")

    for uni in dm['multiverse'].values():
        data = uni['data']['PredatorPrey']
        predator = data['predator']
        sum_of_predators = predator.sum(dim=['x', 'y'])

        assert(sum_of_predators == sum_of_predators.sel({'time': 0})).all()
        

def test_prey_conservation():
    """ If life costs and reproduction chance are both zero, the number of 
        prey should be conserved if there are no predators around.
    """

    mv, dm = mtc.create_run_load(from_cfg="test_prey_conservation.yml")

    for uni in dm['multiverse'].values():
        data = uni['data']['PredatorPrey']
        prey = data['prey']
        sum_of_prey = prey.sum(dim=['x', 'y'])

        assert(sum_of_prey == sum_of_prey.sel({'time': 0})).all()


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
                                    PredatorPrey=dict(
                                        cell_states_from_file=dict(
                                            hdf5_file=tmp_h5data_fpath
                                        )
                                    )
                                )
                            )
    
    for uni in dm['multiverse'].values():
        data = uni['data']['PredatorPrey']
        predator = data['predator']
        prey = data['prey']

        sum_of_predators = predator.sum(dim=['x', 'y'])
        sum_of_prey = prey.sum(dim=['x', 'y'])

        assert(sum_of_predators == sum_of_predators.sel({'time': 0})).all()
        assert(sum_of_prey == sum_of_prey.sel({'time': 0})).all()    


def test_hunting():
    """ Test if prey completely vanishes over time, if the flee chance, 
        life costs, resource intake and reproduction chance are zero.
    """

    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_hunting.yml")
    
    for uni in dm['multiverse'].values():
        data = uni['data']['PredatorPrey']
        predator = data['predator']
        prey = data['prey']

        sum_of_predators = predator.sum(dim=['x', 'y'])
        sum_of_prey = prey.sum(dim=['x', 'y'])

        # The sum of predators should be conserved
        assert(sum_of_predators == sum_of_predators.sel({'time': 0})).all()
        
        # The sum of prey should (not necessarily strictly) monotonically
        # decrease
        assert(np.diff(sum_of_prey, axis=0) <= 0).all()
        # For sufficient long time prey should went extinct. 
        assert(sum_of_prey.sel({'time': 60}) == 0)


def test_prey_reproduction(): 
    """Test the basic interaction functions of the PredatorPrey model"""
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_prey_reproduction.yml")

    # Get the data
    uni = dm['multiverse'][0]
    data = uni['data']['PredatorPrey']
    prey = data['prey']
    predator = data['predator']

    # There are no predators
    assert np.all(predator) == 0

    # Count the number of prey for each time step
    num_prey = [np.count_nonzero(p) for p in prey]
    print(num_prey)

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
    """Test the basic interaction functions of the PredatorPrey model"""
    # Create a multiverse, run a single universe and save the data in the 
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="test_predator_reproduction.yml")

    # Get the data
    uni = dm['multiverse'][0]
    data = uni['data']['PredatorPrey']
    predator = data['predator']

    # Count the number of predators for each time step
    num_predator = [np.count_nonzero(p == 1) for p in predator]

    # Calculate total number of predator which can be created from the
    # amount of available resources
    # NOTE  The parameters are selected such that each predator can have one 
    #       offspring
    expected_final_num_predator = num_predator[0] * 2

    # Check that not all predators reproduce after the first iteration step
    assert expected_final_num_predator != num_predator[1]

    # Check that at the end every predator has reproduced
    assert(expected_final_num_predator == num_predator[-1])

