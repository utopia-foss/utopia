"""Tests of the output of the SimpleEG model"""

import pytest

from ..testtools import *

# Configure the ModelTest class for dummy
mtc = ModelTest("SimpleEG")

# Fixtures --------------------------------------------------------------------
# Define fixtures


# Tests -----------------------------------------------------------------------

def test_basic():
    """Test that the SimpleEG model basically runs"""
    mv = mtc.create_mv_from_cfg("basic")
    mv.run()

    # Load data using data manager
    mv.dm.load_from_cfg(print_tree=True)

    # Assert that data was written
    assert len(mv.dm) > 0

    # TODO add more assertions here

def test_init():
    """Test that the initial conditions are implemented as desired"""
    # Single S0 in the center .................................................
    # Initialize Multiverse, run simulation, load data using DataManager
    mv_single_s0 = mtc.create_mv_from_cfg("single_s0")
    mv_single_s0.run()
    dm_s0 = mv_single_s0.dm
    dm_s0.load_from_cfg()

    # Now have all written data available under dict-like dm_s0
    # TODO do assertions here ...

    # Single S1 in the center .................................................
    mv_single_s1 = mtc.create_mv_from_cfg("single_s1")    
    # TODO continue as above ...
