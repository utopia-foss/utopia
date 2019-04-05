"""Tests of the output of the HdfBench model"""

import numpy as np

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class
mtc = ModelTest("HdfBench", test_file=__file__)

# Fixtures --------------------------------------------------------------------
# Define fixtures


# Tests -----------------------------------------------------------------------

def test_default():
    """Test the default configuration for the HdfBench"""
    # Create a multiverse, run it and load the data
    mv, dm = mtc.create_run_load()

    for uni_no, uni in dm['multiverse'].items():
        times = uni['data/HdfBench/times']
        assert times.shape == (4, 3)
        assert np.min(times) > 0.
        assert len(uni['data/HdfBench']) == 3 + 1

def test_write():
    """Test the write functions"""
    mv, dm = mtc.create_run_load(from_cfg="write_funcs.yml",
                                 perform_sweep=True)

    for uni_no, uni in dm['multiverse'].items():
        times = uni['data/HdfBench/times']
        bench_cfg = uni['cfg']['HdfBench']
        benchmarks = bench_cfg['benchmarks']
        
        print("times data: ", times)
        assert times.shape == (11, len(benchmarks))

        print("benchmark coordinate: ", times.coords['benchmark'])
        for i, bname in enumerate(benchmarks):
            # Assert that all configured benchmarks were performed
            assert (times.coords['benchmark'][i]) == bname

        # Checks for specific benchmarks
        # write_const
        dset = uni['data/HdfBench/write_const']
        const_val = bench_cfg['write_const']['const_val']

        assert dset.shape == (11, 7)
        assert np.min(dset) == np.max(dset) == const_val
        assert dset.dtype == float
