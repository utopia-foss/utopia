"""Tests of the initialisation of the SimpleEG model"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

from .test_init import model_cfg

# Configure the ModelTest class
mtc = ModelTest("SimpleEG", test_file=__file__)


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
            assert np.all(abs_diff_sum)

            print("  Check. :)\n")
