"""Tests the utopya testtools module"""

import pytest

import utopya.testtools as tt


# Fixtures --------------------------------------------------------------------


# Tests -----------------------------------------------------------------------

def test_init():
    """Tests the initialisation and properties of the ModelTest class"""
    # Initialize
    mtc = tt.ModelTest("dummy")

    # Non-existing model name should not be possible
    with pytest.raises(ValueError, match="No such model 'invalid'"):
        tt.ModelTest("invalid")

    # As well as changing the existing model name
    with pytest.raises(RuntimeError, match="A ModelTest's associated model "):
        mtc.model_name = "foo"

    # Assert that property access works
    assert mtc.model_name == "dummy"
    assert mtc.test_dir

    # No Multiverse's should be stored
    assert not mtc._mvs

def test_nonexisting_testdir():
    """Test for a non-existing test directory by fooling with MODELS dict"""
    # Change the contents of utopya.info.MODELS
    tt.MODELS["a_model"] = dict()

    # Initialization should fail
    with pytest.raises(ValueError, match="No test directory for model '"):
        mtc = tt.ModelTest("a_model")

@pytest.mark.skip()
def test_get_cfg_by_name():
    pass

@pytest.mark.skip()
def test_create_mv():
    pass
