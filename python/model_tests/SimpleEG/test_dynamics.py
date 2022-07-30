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
    assert np.absolute(a - b) < epsilon


# Tests -----------------------------------------------------------------------


def test_nonstatic():
    """Check that a randomly initialized grid generates non-static output."""
    mv, dm = mtc.create_run_load(from_cfg="nonstatic.yml")

    for uni_name, uni in dm["multiverse"].items():
        # For debugging, print the IA matrix value
        print("Testing that output is non-static for:")
        print("  Initial state: ", uni["cfg"]["SimpleEG"]["initial_state"])
        print("  IA Matrix:     ", uni["cfg"]["SimpleEG"]["ia_matrix"])

        # Test for both datasets
        for dset_name in ["payoff", "strategy"]:
            dset = uni["data"]["SimpleEG"][dset_name]
            print("  Dataset:       ", dset)

            # Calculate a diff along the time dimension
            diff = np.diff(dset, axis=0)

            # Sum up absolute differences along grid index axes
            abs_diff_sum = np.sum(np.abs(diff), axis=(1, 2))
            # Is now a 1D-array of length (num_steps-1)

            # Assert that all elements are non-zero, i.e. that the sum of the
            # absolute differences in each cell were non-zero for each step
            assert np.all(abs_diff_sum)

            print("  Check. :)\n")


def test_specific_scenario():
    """Test a specific case of the SimpleEG model"""
    # Create a multiverse, run a single univers and save the data in the DataManager dm
    mv, dm = mtc.create_run_load(
        from_cfg="specific_scenario.yml", perform_sweep=False
    )

    for uni in dm["multiverse"].values():
        payoff = uni["data"]["SimpleEG"]["payoff"]
        strategy = uni["data"]["SimpleEG"]["strategy"]

        ### Check specific values
        ## First iteration
        p1 = payoff[1]
        s1 = strategy[1]

        # The centred cell should have a payoff 8*2 and keep strategy S1
        assert_eq(p1[5][5], 8 * 2)
        assert_eq(s1[5][5], 1)

        # Its neighbors should have a payoff 7*1 + 1*0.1 and change their strategy to S1
        for dx in [-1, 0, 1]:
            for dy in [-1, 0, 1]:
                if dx != 0 and dy != 0:
                    assert_eq(p1[5 + dx][5 + dy], 7.0 * 1.0 + 1.0 * 0.1)
                    assert_eq(s1[5 + dx][5 + dy], 1)

        # The cell in the upper left corner should have a payoff 8*1 and startegy S0
        assert_eq(p1[0][0], 8.0)
        assert_eq(s1[0][0], 0)

        ## Second iteration
        p2 = payoff[2]
        s2 = strategy[2]

        # The centred cell should have a payoff 8*0.2 and keep strategy S1
        assert_eq(p2[5][5], 8 * 0.2)
        assert_eq(s2[5][5], 1)

        # Its neighbors on the side should have a payoff 5*0.2 + 3*2 and keep their strategy to S1
        # Its neighbors in the corners should have a payoff 3*0.2 + 5*2 and change their strategy to S0
        for dx in [-1, 0, 1]:
            for dy in [-1, 0, 1]:
                if dx != 0 and dy != 0:
                    if dx == 0 or dy == 0:
                        print(dx, dy)
                        assert_eq(p2[5 + dx][5 + dy], 5.0 * 0.2 + 3.0 * 2.0)
                        assert_eq(s2[5 + dx][5 + dy], 1)
                    else:
                        assert_eq(p2[5 + dx][5 + dy], 3.0 * 0.2 + 5.0 * 2.0)
                        assert_eq(s2[5 + dx][5 + dy], 1)


def test_macroscopic_values():
    """Test macroscopic values taken and adapted from Nowak & May 1992"""
    # Create a multiverse, run a single universe and load the data into the
    # DataManager dm
    mv, dm = mtc.create_run_load(from_cfg="macroscopic_value.yml")

    for uni_no, uni in dm["multiverse"].items():
        cfg = uni["cfg"]["SimpleEG"]

        # Get the strategy
        strategy = uni["data"]["SimpleEG"]["strategy"]

        # Get the number of grid cells
        num_cells = strategy.shape[1] * strategy.shape[2]

        # Calculate the frequency of S0 and S1 for the last five time steps
        counts = [
            np.bincount(strategy.stack(grid=["x", "y"]).isel(time=i))
            for i in [-5, -4, -3, -2, -1]
        ]
        frequencies = [c / num_cells for c in counts]

        print(
            "Frequencies of S0 in last five time steps:  {}"
            "".format(frequencies)
        )

        # Assert that the frequencies of S0 (here: cooperators) is ~ 0.41+-0.2
        for f in frequencies:
            assert_eq(f[0], 0.41, epsilon=0.1)

        # NOTE In the paper of Nowak & May 1992 the final frequency is at
        #      about 0.31. However, they have self-interactions included we do
        #      not want to have.  Nevertheless, this test should check
        #      whether a rather stable final frequency is reached and does not
        #      change a lot within the last five time steps
