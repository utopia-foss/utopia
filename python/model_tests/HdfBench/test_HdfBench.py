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

    for uni_no, uni in dm['uni'].items():
        assert uni['data/HdfBench/times'].shape == (4, 3)
        assert len(uni['data/HdfBench']) == 3 + 1
