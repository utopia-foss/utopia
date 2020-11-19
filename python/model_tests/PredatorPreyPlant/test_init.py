"""Tests of the initialisation of the PredatorPreyPlant model"""

from itertools import chain

import numpy as np
import h5py as h5

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
    f = h5.File(fpath, "w")

    f.create_dataset("predator", data=np.ones((21, 21)))
    f.create_dataset("prey", data=np.zeros((21, 21)))
    f.create_dataset("plant", data=np.ones((21, 21)))

    # Need to close the file such that the model can read it
    f.close()
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
    _, dm = mtc.create_run_load(from_cfg="init_from_file.yml",
                                parameter_space=dict(
                                    PredatorPreyPlant=dict(
                                        cell_states_from_file=dict(
                                            hdf5_file=tmp_h5data_fpath))))

    for uni in dm['multiverse'].values():
        data = uni['data']['PredatorPreyPlant']
        assert (data['predator'] == 1).all()
        assert (data['prey'] == 0).all()
        assert (data['plant'] == 1).all()

def test_initial_state_random(): 
    """Test that the initial states are random.

    This also tests for the correct array shape, something that is not done in
    the other tests.
    """
    # Use the config file for common settings, change via additional kwargs
    _, dm = mtc.create_run_load(from_cfg="initial_state.yml")

    # For all universes, perform checks on the data
    for uni in dm['multiverse'].values():
        data = uni['data']['PredatorPreyPlant']

        # Get the number of predators and prey for the population of the cell
        num_prey = np.sum(data['prey'])
        num_predator = np.sum(data['predator'])

        # Total number of cells can be extracted from shape
        num_cells = np.prod(data['prey'].shape[1:])

        # Number of cells should be prey + predators + empty, every cell should
        # either be empty or populated by either predator or prey
        assert num_cells > num_prey + num_predator

        # Check that only a single step was written
        assert data['prey'].shape[0] == 1
        assert data['predator'].shape[0] == 1
        assert data['resource_prey'].shape[0] == 1
        assert data['resource_predator'].shape[0] == 1

        # # Every individual gets between 2 resource units and 8
        res_prey = data['resource_prey'].values
        res_pred = data['resource_predator'].values
        assert np.all(res_prey[res_prey != 0] >= 2)
        assert np.all(data['resource_predator']>= 0)
        assert np.all(res_pred[res_pred != 0] >= 2)
        assert np.all(data['resource_predator']<= 8)

        # Population should be random; calculate the ratio and check limits
        assert 0.15 <= num_prey / num_cells <= 0.25  # prey
        assert 0.235 <= num_predator / num_cells <= 0.335  # predator


    # Test again for other probability values
    _, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                parameter_space=dict(
                                    PredatorPreyPlant=dict(
                                        cell_manager=dict(
                                            cell_params=dict(
                                                p_prey=0.1, p_predator=0.1)))))
                                    
    for uni in dm['multiverse'].values():
        data = uni['data']['PredatorPreyPlant']

        # Get the number of predators and prey for the population of the cell
        num_prey = np.sum(data['prey'])
        num_predator = np.sum(data['predator'])

        # Total number of cells can be extracted from shape
        num_cells = np.prod(data['prey'].shape[1:])

        # Number of cells should be prey + predators + empty, every cell should
        # either be empty or populated by either predator or prey
        assert num_cells > num_prey + num_predator

        res_prey = data['resource_prey'].values
        res_pred = data['resource_predator'].values
        assert np.all(res_prey[res_prey != 0] >= 2)
        assert np.all(data['resource_predator']>= 0)
        assert np.all(res_pred[res_pred != 0] >= 2)
        assert np.all(data['resource_predator']<= 8)
        
        # Strategies should be random; calculate the ratio and check limits
        assert 0.05 <= num_prey / num_cells <= 0.15
        assert 0.05 <= num_predator / num_cells <= 0.15
