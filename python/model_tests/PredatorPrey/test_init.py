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
    # Use the config file for common settings, change via additional kwargs
    mv, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                 perform_sweep=True)

    # For all universes, perform checks on the data
    for uni in dm['multiverse'].values():
        data = uni['data']['PredatorPrey']

        # Get the number of predators and prey for the population of the cell
        num_empty = np.sum(data['population'] == 0)
        num_prey = np.sum(data['population'] == 1)
        num_predator = np.sum(data['population'] == 2)
        num_predprey = np.sum(data['population'] == 3)

        # Total number of cells can be extracted from shape
        num_cells = data['population'].shape[1] * data['population'].shape[2]

        # Number of cells should be prey + predators + empty, every cell should
        # either be empty or populated by either predator or prey
        assert num_cells == num_empty + num_prey + num_predator + num_predprey

        # Check that only a single step was written
        assert data['population'].shape[0] == 1
        assert data['resource_prey'].shape[0] == 1
        assert data['resource_predator'].shape[0] == 1

        # Every individual gets 2 resource units
        assert num_prey + num_predprey == np.sum(data['resource_prey']) / 2
        assert (num_predator + num_predprey 
                == np.sum(data['resource_predator']) / 2)

        # Population should be random; calculate the ratio and check limits
        assert 0.15 <= num_prey / num_cells <= 0.25  # prey
        assert 0.235 <= num_predator / num_cells <= 0.335  # predator
        assert 0.05 <= num_predprey / num_cells <= 0.15  # predator and prey


    # Test again for another probability value
    mv, dm = mtc.create_run_load(from_cfg="initial_state_2.yml",
                                 perform_sweep=True)
                                    
    for uni in dm['multiverse'].values():
        data = uni['data']['PredatorPrey']

        # Get the number of predators and prey for the population of the cell
        num_empty = np.sum(data['population'] == 0)
        num_prey = np.sum(data['population'] == 1)
        num_predator = np.sum(data['population'] == 2)
        num_predprey = np.sum(data['population'] == 3)

        # Total number of cells can be extracted from shape
        num_cells = data['population'].shape[1] * data['population'].shape[2]

        # Number of cells should be prey + predators + empty, every cell should
        # either be empty or populated by either predator or prey
        assert num_cells == num_empty + num_prey + num_predator

        # Every individual gets 2 resource units
        assert num_prey == np.sum(data['resource_prey']) / 2
        assert num_predator == np.sum(data['resource_predator']) / 2

        # Strategies should be random; calculate the ratio and check limits
        assert 0.05 <= num_prey / num_cells <= 0.15  # prey
        assert 0.05 <= num_predator / num_cells <= 0.15
