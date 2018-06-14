"""Tests of the output of the dummy model"""

import pytest

from ..testtools import *

# Configure the ModelTest class for dummy
mtc = ModelTest("dummy")

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
