"""Tests of the output of the dummy model"""

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class for dummy
mtc = ModelTest("dummy", test_file=__file__)

# Fixtures --------------------------------------------------------------------
# Define fixtures


# Tests -----------------------------------------------------------------------

def test_single():
    """Test initialisation"""
    mv = mtc.create_mv_from_cfg("single")
    mv.run()

    # Load data using data manager
    mv.dm.load_from_cfg(print_tree=True)

    # Assert that data was written
    assert len(mv.dm) > 0

    # TODO add more assertions here
    
