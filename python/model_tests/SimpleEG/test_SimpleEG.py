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

def model_cfg(**kwargs) -> dict:
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
                                **model_cfg(initial_state='random'))

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
                                **model_cfg(initial_state='fraction',
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
    # Create a few Multiverses with different initial states
    mvs = []

    # Use the config file for common settings, change via additional kwargs
    mvs.append(mtc.create_mv_from_cfg("initial_state.yml",
                                      **model_cfg(initial_state='single_s0')))

    mvs.append(mtc.create_mv_from_cfg("initial_state.yml",
                                      **model_cfg(initial_state='single_s1')))

    # For all: Run the simulations (initial step only) and load the data
    for mv in mvs:
        mv.run()
        mv.dm.load_from_cfg(print_tree=True)

    # For all multiverses, go over all universes and check that all cells are
    # of the desired strategy
    for uni in chain(*[mv.dm['uni'].values() for mv in mvs]):
        # Get the data
        data = uni['data']['SimpleEG']

        # Get the grid size
        grid_size = uni['cfg']['SimpleEG']['grid_size']

        # For even grid sizes, this should fail
        if (grid_size[0] % 2 == 0) or (grid_size[1] % 2 == 0):
            # Check that no data was written
            assert not len(data)

            # Cannot continue
            continue

        # For others, calculate the central cell (integer division!)
        c_idx = (grid_size[0]//2) * grid_size[1] + grid_size[1]//2

        # Check the center cell strategy
        print(data['strategy'].data)

        # Check the data, depending on what the strategy ought to be
        if uni['cfg']['SimpleEG']['initial_state'] == "single_s0":
            # Central 0, all others 1
            assert data['strategy'][0, c_idx] == 0
            assert np.sum(data['strategy']) == grid_size[0] * grid_size[1] - 1
        
        else:
            # Central 1, all others 0
            assert data['strategy'][0, c_idx] == 1
            assert np.sum(data['strategy']) == 1
        
        # Finally, all payoffs should be zero
        assert not np.any(data['payoff'])


    # Create a few more Multiverses; these should fail due to at least one
    # grid_size extension being an even value, where no central cell can be
    # calculated ...
    with pytest.raises(SystemExit, match="1"):
        mtc.create_mv_from_cfg("initial_state.yml",
                               perform_sweep=False,
                               **model_cfg(initial_state='single_s0',
                                           grid_size=[10, 10])
                               ).run()

    with pytest.raises(SystemExit, match="1"):
        mtc.create_mv_from_cfg("initial_state.yml",
                               perform_sweep=False,
                               **model_cfg(initial_state='single_s0',
                                           grid_size=[10, 10])
                               ).run()
