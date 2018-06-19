"""Tests of the output of the SimpleEG model"""

from itertools import chain

import numpy as np

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class
mtc = ModelTest("SimpleEG", test_file=__file__)

# Fixtures --------------------------------------------------------------------
# Define fixtures here


# Helpers ---------------------------------------------------------------------

def update_model_cfg(**kwargs) -> dict:
    """Creates a dict that can update the config of the SimpleEG model"""
    return dict(parameter_space=dict(SimpleEG=dict(**kwargs)))


# Tests -----------------------------------------------------------------------

def test_basics():
    """Test the most basic features of the model, e.g. that it runs"""
    # Create a Multiverse using the default model configuration
    mv = mtc.create_mv()

    # Run a single simulation
    mv.run_single()

    # Load data using the DataManager
    mv.dm.load_from_cfg(print_tree=True)
    # The `print_tree` flag creates output of which data was loaded

    # Assert that data was loaded, i.e. that data was written
    assert len(mv.dm)


def test_initial_state_random(): 
    """Test that the initial states are random.

    This also tests for the correct array shape, something that is not done in
    the other tests.
    """
    # Use the config file for common settings, change via additional kwargs
    mv = mtc.create_mv_from_cfg("initial_state.yml",
                                **update_model_cfg(initial_state='random'))

    # Run the simulation (initial step only)
    mv.run_sweep()

    # Load data
    mv.dm.load_from_cfg(print_tree=True)

    # For all universes, perform checks on the payoff and strategy data
    for uni in mv.dm['uni'].values():
        data = uni['data']['SimpleEG']

        # Get the grid size
        grid_size = uni['cfg']['SimpleEG']['grid_size']
        num_cells = grid_size[0] * grid_size[1]

        # Check that only a single step was written and the extent is correct
        assert data['payoff'].shape == (1, num_cells)
        assert data['strategy'].shape == (1, num_cells)

        # All payoffs should be zero
        assert not np.any(data['payoff'])

        # Strategies should be random; calculate the ratio and check limits
        s1_fraction = np.sum(data['strategy'])/data['strategy'].shape[1]
        assert 0.45 <= s1_fraction <= 0.55  # TODO values ok?


def test_initial_state_fraction():
    """Test that the initial state is set according to a fraction"""
    # Set the fraction to test
    s1_fraction = 0.1

    # Use the config file for common settings, change via additional kwargs
    mv = mtc.create_mv_from_cfg("initial_state.yml",
                                **update_model_cfg(initial_state='fraction',
                                                   s1_fraction=s1_fraction))

    # Run the simulation (initial step only) and load data
    mv.run_sweep()
    mv.dm.load_from_cfg(print_tree=True)

    # For all universes, check that the fraction is met
    for uni in mv.dm['uni'].values():
        data = uni['data']['SimpleEG']

        # All payoffs should be zero
        assert not np.any(data['payoff'])

        # The fraction of s1 strategy
        num_s1 = np.sum(data['strategy'])
        assert num_s1 == int(s1_fraction * data['strategy'].shape[1])  # floor


def test_initial_state_single(): 
    """Test that the initial state are """
    # Use the config file for common settings, change via additional kwargs
    # Create a few Multiverses with different initial states
    mvs = []

    mvs.append(mtc.create_mv_from_cfg("initial_state.yml",
                                      **update_model_cfg(initial_state='single_s0')))

    mvs.append(mtc.create_mv_from_cfg("initial_state.yml",
                                      **update_model_cfg(initial_state='single_s1')))

    # TODO add test for even grid_size extensions

    # For all: Run the simulations (initial step only) and load the data
    for mv in mvs:
        mv.run()
        mv.dm.load_from_cfg(print_tree=True)

    # For all multiverses, go over all universes and check that all cells are
    # of the desired strategy
    for uni in chain(*[mv.dm['uni'].values() for mv in mvs]):
        # Get the data
        data = uni['data']['SimpleEG']

        # All payoffs should be zero
        assert not np.any(data['payoff'])

        # Find out what the strategy ought to be
        if uni['cfg']['SimpleEG']['initial_state'] == "single_s0":
            c_strategy = 0
        else:
            c_strategy = 1

        # Calculate the index of the central cell (integer divison!)
        grid_size = uni['cfg']['SimpleEG']['grid_size']
        c_idx = (grid_size[0]//2) * grid_size[1] + grid_size[1]//2

        # Check the center cell strategy
        print(data['strategy'].data)
        assert data['strategy'][0, c_idx] == c_strategy

        # Check that all others have the other strategy
        if c_strategy == 0:
            # All strategy 1 except central cell
            sum_total = grid_size[0] * grid_size[1] - 1
        
        else:
            # All strategy 0 except central cell
            sum_total = 1

        assert np.sum(data['strategy']) == sum_total
