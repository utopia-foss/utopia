"""Tests of the initialisation of the PredatorPrey model"""

from itertools import chain

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

def assert_eq(a, b, *, epsilon=1e-6):
    """Assert that two quantities are equal within a numerical epsilon range"""
    assert(abs(a-b) < epsilon)

# Tests -----------------------------------------------------------------------

def test_basics():
    """Test the most basic features of the model, e.g. that it runs"""
    # Create a Multiverse using the default model configuration, except for 
    # smaller grid size and one step
    mv, dm = mtc.create_run_load(from_cfg="initial_state.yml")

    # Assert that data was loaded, i.e. that data was written
    assert len(dm)


def test_initial_state_random(): 
    """Test that the initial states are random.

    This also tests for the correct array shape, something that is not done in
    the other tests.
    """

    # Choose the parameters for the random initial state
    prey_prob = 0.2
    pred_prob = 0.285
    predprey_prob = 0.1

    # Use the config file for common settings, change via additional kwargs
    mv, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                 perform_sweep=True,
                                 **model_cfg(initial_state='random', 
                                 prey_prob=prey_prob, pred_prob=pred_prob,
                                 predprey_prob=predprey_prob))

    # For all universes, perform checks on the data
    for uni in dm['multiverse'].values():
        

        data = uni['data']['PredatorPrey']

        # Get the grid size
        grid_size = uni['cfg']['PredatorPrey']['grid_size']
        num_cells = grid_size[0] * grid_size[1]

        # Get the number of predators and prey for the population of the cell
        num_predprey = np.sum(data['population'] == 3)
        num_prey = np.sum(data['population'] == 1)
        num_predator = np.sum(data['population'] == 2)
        num_empty = np.sum(data['population'] == 0)


        # Check that only a single step was written and the extent is correct
        assert data['population'].shape == (1, num_cells)
        assert data['resource_prey'].shape == (1, num_cells)
        assert data['resource_predator'].shape == (1, num_cells)

        # Number of cells should be prey + predators + empty, every cell should
        # either be empty or populated by either predator or prey
        assert num_cells == num_empty + num_prey + num_predator + num_predprey

        # Every individual gets 2 resource units
        assert num_prey + num_predprey == np.sum(data['resource_prey']) / 2
        assert (num_predator + num_predprey 
                == np.sum(data['resource_predator']) / 2)

        # Populaton should be random; calculate the ratio and check limits
        assert 0.15 <= num_prey / num_cells <= 0.25  # prey
        assert 0.235 <= num_predator / num_cells <= 0.335  # predator
        assert 0.05 <= num_predprey / num_cells <= 0.15  # predator and prey


    # Test again for another probability value
    prey_prob = 0.1
    pred_prob = 0.1
    predprey_prob = 0.0
    mv, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                 perform_sweep=True,
                                 **model_cfg(initial_state='random',
                                             prey_prob=prey_prob, 
                                             pred_prob=pred_prob,
                                             predprey_prob=predprey_prob))

    for uni in dm['multiverse'].values():
        data = uni['data']['PredatorPrey']

        # Get the grid size
        grid_size = uni['cfg']['PredatorPrey']['grid_size']
        num_cells = grid_size[0] * grid_size[1]

        # Get the number of predators and prey for the population of the cell
        num_prey = np.sum(data['population'] == 1)
        num_predator = np.sum(data['population'] == 2)
        num_empty = np.sum(data['population'] == 0)


        # Check that only a single step was written and the extent is correct
        assert data['population'].shape == (1, num_cells)
        assert data['resource_prey'].shape == (1, num_cells)
        assert data['resource_predator'].shape == (1, num_cells)

        # Number of cells should be prey + predators + empty, every cell should
        # either be empty or populated by either predator or prey
        assert num_cells == num_empty + num_prey + num_predator

        # Every individual gets 2 resource units
        assert num_prey == np.sum(data['resource_prey']) / 2
        assert num_predator == np.sum(data['resource_predator']) / 2

        # Strategies should be random; calculate the ratio and check limits
        assert 0.05 <= num_prey / num_cells <= 0.15  # prey
        assert 0.05 <= num_predator / num_cells <= 0.15
        


def test_initial_state_fraction():
    """Test that the initial state is set according to a fraction"""
    # Set the fraction to test
    prey_frac = 0.2
    pred_frac = 0.2
    predprey_frac = 0.1

    # Use the config file for common settings, change via additional kwargs
    mv, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                 perform_sweep=True,
                                 **model_cfg(initial_state='fraction',
                                             prey_frac=prey_frac, 
                                             pred_frac=pred_frac,
                                             predprey_frac=predprey_frac))

    # For all universes, check that the fraction is met
    for uni in dm['multiverse'].values():
        data = uni['data']['PredatorPrey']

        # Print the data (useful if something fails)
        print(data['population'].data)

        # Count the cells populated with prey and predators
        num_prey = np.sum(data['population'] == 1)
        num_pred = np.sum(data['population'] == 2)
        num_predprey = np.sum(data['population'] == 3)
        # Check that the correct number of cells was initiliazied in the
        # prescribed way, floor
        assert num_prey == int(prey_frac * data['population'].shape[1])
        assert num_pred == int(pred_frac * data['population'].shape[1])
        assert num_predprey == int(predprey_frac * data['population'].shape[1])



