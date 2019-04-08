"""Tests of the initialisation of the ForestFire"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class for ForestFire model
mtc = ModelTest("ForestFire", test_file=__file__)

# Fixtures --------------------------------------------------------------------
# Define fixtures

# Helpers ---------------------------------------------------------------------
# NOTE These helpers are also imported by other test modules for ForestFire

def model_cfg(**kwargs) -> dict:
    """Creates a dict that can update the config of the ForestFire"""
    return dict(parameter_space=dict(ForestFire=dict(**kwargs)))

def cell_params(**kwargs) -> dict:
    """Creates a dict that can update the config of the ForestFire"""
    return model_cfg(cell_manager=dict(cell_params=dict(**kwargs)))

def is_equal(x, y, *, tol=1e-16) -> bool:
    """Whether x is in interval [y-tol, y+tol]"""
    return y-tol <= x <= y+tol

# Tests -----------------------------------------------------------------------

def test_basics():
    """Test the most basic features of the model"""
    # Create a Multiverse using the default model configuration
    mv = mtc.create_mv()

    # Run a single simulation
    mv.run_single()

    # Load data using the DataManager and the default load configuration
    mv.dm.load_from_cfg(print_tree=True)
    # The `print_tree` flag creates output of which data was loaded
    # NOTE can also use a shortcut to do all of the above, see test_output

    # Assert that data was loaded, i.e. that data was written
    assert len(mv.dm)

def test_output(): 
    """Test that the output structure is correct"""
    # Create a Multiverse and let it run
    mv, dm = mtc.create_run_load(from_cfg="output.yml", perform_sweep=True)
    # NOTE this is a shortcut. It creates the mv, lets it run, then loads data

    # Get the meta-config from the DataManager
    mcfg = dm['cfg']['meta']
    print("meta config: ", mcfg)

    # Assert that the number of runs matches the specified ones
    assert len(dm['multiverse']) == mcfg['parameter_space'].volume

    # For each universe, iterate over the output data and assert the shape
    # and the content of the output data
    for uni_no, uni in dm['multiverse'].items():
        # Get the data
        data = uni['data']['ForestFire']

        # Get the config of this universe
        uni_cfg = uni['cfg']

        # Check that all datasets are available
        assert 'kind' in data
        assert 'tree_density' in data
        assert 'cluster_id' in data

def test_initial_state_random(): 
    """Test that the initial states are random and densities are carried over.

    The tests done here only perform the initial write operation.
    """
    # Use the config file for common settings, change via additional kwargs
    # Start with .5 tree density
    p_tree = 0.5
    _, dm = mtc.create_run_load(from_cfg="initial_state.yml",  # num_steps: 0
                                **cell_params(p_tree=p_tree))

    # For all universes, perform checks on the state
    for uni in dm['multiverse'].values():
        kind = uni['data/ForestFire/kind']
        tree_density = uni['data/ForestFire/tree_density']

        # Kind should be random; calculate the ratio and check limits
        assert is_equal(kind.mean(), p_tree, tol=0.05)

        # Check calculated tree density matches the given one
        assert is_equal(kind.mean(), tree_density.item())

    # Test again for another probability value
    p_tree = 0.2
    _, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                **cell_params(p_tree=p_tree))

    for uni in dm['multiverse'].values():
        kind = uni['data/ForestFire/kind']
        tree_density = uni['data/ForestFire/tree_density']
        assert is_equal(kind.mean(), p_tree, tol=0.05)
        assert is_equal(kind.mean(), tree_density.item())

    # Test again for another probability value
    p_tree = 0.0
    _, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                **cell_params(p_tree=p_tree))

    for uni in dm['multiverse'].values():
        kind = uni['data/ForestFire/kind']
        tree_density = uni['data/ForestFire/tree_density']

        # check, that no cell is a tree
        assert (kind == 0).all()
        assert is_equal(kind.mean(), tree_density.item())

    
    p_tree = 1.0
    _, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                **cell_params(p_tree=p_tree))

    for uni in dm['multiverse'].values():
        kind = uni['data/ForestFire/kind']
        tree_density = uni['data/ForestFire/tree_density']

        # check, that all cells are trees
        assert (kind == 1).all()
        assert is_equal(kind.mean(), tree_density.item())
