"""Tests of the initialisation of the SimpleEG model"""

from itertools import chain

import numpy as np

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class
mtc = ModelTest("SimpleEG", test_file=__file__)

# Fixtures --------------------------------------------------------------------
# Define fixtures here


# Helpers ---------------------------------------------------------------------
# NOTE These helpers are also imported by other test modules for SimpleEG

def model_cfg(**kwargs) -> dict:
    """Creates a dict that can update the config of the SimpleEG model"""
    return dict(parameter_space=dict(SimpleEG=dict(**kwargs)))

def ia_matrix_from_b(b):
    """Creates an interaction matrix from the benefit parameter b"""
    return [[1, 0], [b, 0]]

def ia_matrix_from_bc(*, b, c):
    """Creates an itneraction matrix from a bc-pair"""
    return [[b-c, -c], [b, 0]]

def assert_eq(a, b, *, epsilon=1e-6):
    """Assert that two quantities are equal within a numerical epsilon range"""
    assert(abs(a-b) < epsilon)

def check_ia_matrices(m1, m2):
    """Check that the matrix elements are equal"""
    assert_eq(m1[0][0], m2[0][0])
    assert_eq(m1[0][1], m2[0][1])
    assert_eq(m1[1][0], m2[1][0])
    assert_eq(m1[1][1], m2[1][1])

# Tests -----------------------------------------------------------------------

def test_basics():
    """Test the most basic features of the model, e.g. that it runs"""
    # Create a Multiverse using the default model configuration
    mv, dm = mtc.create_run_load()

    # Assert that data was loaded, i.e. that data was written
    assert len(dm)


def test_initial_state_random(): 
    """Test that the initial states are random.

    This also tests for the correct array shape, something that is not done in
    the other tests.
    """
    # Use the config file for common settings, change via additional kwargs
    mv, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                 perform_sweep=True,
                                 **model_cfg(initial_state='random'))

    # For all universes, perform checks on the payoff and strategy data
    for uni in dm['multiverse'].values():
        data = uni['data']['SimpleEG']

        # Get the grid size
        grid_size = uni['cfg']['SimpleEG']['grid_size']

        # Check that only a single step was written and the extent is correct
        assert data['payoff'].shape == (1, grid_size[1], grid_size[0])
        assert data['strategy'].shape == (1, grid_size[1], grid_size[0])

        # All payoffs should be zero
        assert not np.any(data['payoff'])

        # Strategies should be random; calculate the ratio and check limits
        shape = (data['strategy'].shape[1], data['strategy'].shape[2])
        s1_fraction = np.sum(data['strategy'])/(shape[0] * shape[1])
        assert 0.45 <= s1_fraction <= 0.55  # TODO values ok?


    # Test again for another probability value
    s1_prob = 0.2
    mv, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                 perform_sweep=True,
                                 **model_cfg(initial_state='random',
                                             s1_prob=s1_prob))

    for uni in dm['multiverse'].values():
        data = uni['data']['SimpleEG']

        # All payoffs should be zero
        assert not np.any(data['payoff'])

        # Calculate fraction and compare to desired probability
        shape = data['strategy'].shape[1:]
        s1_fraction = np.sum(data['strategy']) / (shape[0] * shape[1])
        assert s1_prob - 0.05 <= s1_fraction <= s1_prob + 0.05


def test_initial_state_fraction():
    """Test that the initial state is set according to a fraction"""
    # Set the fraction to test
    s1_fraction = 0.1

    # Use the config file for common settings, change via additional kwargs
    mv, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                 perform_sweep=True,
                                 **model_cfg(initial_state='fraction',
                                             s1_fraction=s1_fraction))

    # For all universes, check that the fraction is met
    for uni in dm['multiverse'].values():
        data = uni['data']['SimpleEG']

        # All payoffs should be zero
        assert not np.any(data['payoff'])

        # Print the data (useful if something fails)
        print(data['strategy'].data)

        # Count the cells with strategy 1
        num_s1 = np.sum(data['strategy'])
        shape = (data['strategy'].shape[1], data['strategy'].shape[2])
        assert num_s1 == int(s1_fraction * shape[0] * shape[1])  # floor


def test_initial_state_single(): 
    """Test that the initial state are """
    # Create a few Multiverses with different initial states and store the
    # resulting DataManagers
    dms = []

    for initial_state in ['single_s0', 'single_s1']:
        _, dm = mtc.create_run_load(from_cfg="initial_state.yml",
                                    **model_cfg(initial_state=initial_state))
        dms.append(dm)

    # For all multiverses, go over all universes and check that all cells are
    # of the desired strategy
    for uni in chain(*[dm['multiverse'].values() for dm in dms]):
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

        # Check the center cell strategy
        print(data['strategy'].data)

        # Check the data, depending on what the strategy ought to be
        if uni['cfg']['SimpleEG']['initial_state'] == "single_s0":
            # Central 0, all others 1
            assert data['strategy'][0, grid_size[1]//2, grid_size[0]//2] == 0
            assert np.sum(data['strategy']) == grid_size[0] * grid_size[1] - 1
        
        else:
            # Central 1, all others 0
            assert data['strategy'][0, grid_size[1]//2, grid_size[0]//2] == 1
            assert np.sum(data['strategy']) == 1
        
        # Finally, all payoffs should be zero
        assert not np.any(data['payoff'])


    # Create a few more Multiverses; these should fail due to at least one
    # grid_size extension being an even value, where no central cell can be
    # calculated ...
    with pytest.raises(SystemExit, match="1"):
        mtc.create_run_load(from_cfg="initial_state.yml",
                            perform_sweep=False,
                            **model_cfg(initial_state='single_s0',
                                        grid_size=[10, 10])
                            )

    with pytest.raises(SystemExit, match="1"):
        mtc.create_run_load(from_cfg="initial_state.yml",
                            perform_sweep=False,
                            **model_cfg(initial_state='single_s0',
                                        grid_size=[10, 11])
                            )

def test_ia_matrix_extraction():
    """Test that the ia_matrix is extracted correctly from the config"""
    
    # Test all possible combinations of specifying input parameters
    # The different yaml config files provide the following different cases:
    #   0:        ia_matrix: [[1, 2], [3, 4]]
    #   1:        bc_pair: [4, 5]
    #   2:        b: 1.9
    #   3:        b: 1.9
    #             bc_pair: [4, 5]
    #   4:        b: 1.9
    #             ia_matrix: [[1, 2], [3, 4]]
    #   5:        ia_matrix: [[1, 2], [3, 4]]
    #             bc_pair: [4, 5]
    #   6:        b: 1.9
    #             bc_pair: [4, 5]
    #             ia_matrix: [[1, 2], [3, 4]]
    #   7:        -
    #
    # The default parameter is: b = 1.58

    # Define the interaction matrix values to test against
    ia_matrices = []
    ia_matrices.append([[1,2],[3,4]])                 # case 0 
    ia_matrices.append(ia_matrix_from_bc(b=4,c=5))    # case 1
    ia_matrices.append(ia_matrix_from_b(1.9))         # case 2
    ia_matrices.append(ia_matrix_from_bc(b=4,c=5))    # case 3
    ia_matrices.append([[1,2],[3,4]])                 # case 4  
    ia_matrices.append([[1,2],[3,4]])                 # case 5 
    ia_matrices.append([[1,2],[3,4]])                 # case 6
    ia_matrices.append(ia_matrix_from_b(1.58))        # case 7

    # For each of these cases, create, run, and load a Multiverse; then test
    # against the expected ia_matrices above.
    for i, expected_matrix in enumerate(ia_matrices):
        _, dm = mtc.create_run_load(from_cfg="ia_matrix_case{}.yml".format(i))

        # Get default universe from multiverse
        uni = dm['multiverse'][0]

        # Now, check whether the ia_matrix is correct
        check_ia_matrices(uni['data']['SimpleEG'].attrs['ia_matrix'],
                          expected_matrix)
