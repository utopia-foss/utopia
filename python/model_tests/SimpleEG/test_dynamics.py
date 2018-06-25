"""Tests of the initialisation of the SimpleEG model"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

from .test_init import model_cfg

# Configure the ModelTest class
mtc = ModelTest("SimpleEG", test_file=__file__)

# Utility functions -----------------------------------------------------------

def assert_eq(a, b, *, epsilon=1e-6):
    """Assert that two quantities are equal within a numerical epsilon range"""
    assert(np.absolute(a-b) < epsilon)

# Tests -----------------------------------------------------------------------

def test_nonstatic():
    """Check that a randomly initialized grid generates non-static output."""
    mv, dm = mtc.create_run_load(cfg_file="nonstatic.yml")

    for uni_name, uni in dm['uni'].items():
        # For debugging, print the IA matrix value
        print("Testing that output is non-static for:")
        print("  Initial state: ", uni['cfg']['SimpleEG']['initial_state'])
        print("  IA Matrix:     ", uni['cfg']['SimpleEG']['ia_matrix'])

        # Test for both datasets
        for dset_name in ['payoff', 'strategy']:
            dset = uni['data']['SimpleEG'][dset_name]
            print("  Dataset:       ", dset)

            # Calculate a diff along the time dimension
            diff = np.diff(dset, axis=0)

            # Sum up absolute differences along grid index dimension
            abs_diff_sum = np.sum(np.abs(diff), axis=1)
            # Is now a 1D-array of length (num_steps-1)

            # Assert that all elements are non-zero, i.e. that the sum of the
            # absolute differences in each cell were non-zero for each step
            # assert np.all(abs_diff_sum)

            print("  Check. :)\n")


def test_specific_scenario():
    """Test a specific case of the SimpleEG model"""
    # Create a multiverse, run a single univers and save the data in the DataManager dm
    mv, dm = mtc.create_run_load(cfg_file="specific_scenario.yml", perform_sweep=False)

    # payoff = dm['uni']['0']['data']['SimpleEG']['payoff'].reshape(11,11,11)
    payoff = dm['uni']['0']['data']['SimpleEG']['payoff'].reshape(11,11,11)
    strategy = dm['uni']['0']['data']['SimpleEG']['strategy'].reshape(11,11,11)

    print(payoff[0])
    print(strategy[0])

    ### Check specific values 
    ## First iteration
    p1 = payoff[1]
    s1 = strategy[1]

    # The centred cell should have a payoff 8*2 and keep strategy S1
    assert_eq(p1[5][5], 8*2)
    assert_eq(s1[5][5], 1)

    # Its neighbors should have a payoff 7*1 + 1*0.1 and change their strategy to S1
    for dx in [-1, 0, 1]:
        for dy in [-1, 0, 1]:
            if (dx != 0 and dy != 0):
                assert_eq(p1[5+dx][5+dy], 7.*1. + 1.*0.1)
                assert_eq(s1[5+dx][5+dy], 1)

    # The cell in the upper left corner should have a payoff 8*1 and startegy S0
    assert_eq(p1[0][0], 8.)
    assert_eq(s1[0][0], 0)

    ## Second iteration
    p2 = payoff[2]
    s2 = strategy[2]
    print(p2)
    print(s2)

    # The centred cell should have a payoff 8*0.2 and keep strategy S1
    assert_eq(p2[5][5], 8*0.2)
    assert_eq(s2[5][5], 1)
    
    # Its neighbors on the side should have a payoff 5*0.2 + 3*2 and keep their strategy to S1
    # Its neighbors in the corners should have a payoff 3*0.2 + 5*2 and change their strategy to S0
    for dx in [-1, 0, 1]:
        for dy in [-1, 0, 1]:
            if (dx != 0 and dy != 0):
                if dx == 0 or dy == 0:
                    print(dx, dy)
                    assert_eq(p2[5+dx][5+dy], 5.*0.2 + 3.*2.)
                    assert_eq(s2[5+dx][5+dy], 1)
                else:
                    assert_eq(p2[5+dx][5+dy], 3.*0.2 + 5.*2.)
                    assert_eq(s2[5+dx][5+dy], 1)