"""Tests the utopya testtools module"""

import os

import pytest

import utopya
import utopya.testtools as tt


# Fixtures --------------------------------------------------------------------


# Tests -----------------------------------------------------------------------

def test_init():
    """Tests the initialisation and properties of the ModelTest class"""
    # Initialize
    mtc = tt.ModelTest("dummy", test_file=__file__)

    # Non-existing model name should not be possible
    with pytest.raises(ValueError, match="No such model 'invalid'"):
        tt.ModelTest("invalid")

    # As well as changing the existing model name
    with pytest.raises(RuntimeError, match="A ModelTest's associated model "):
        mtc.model_name = "foo"

    # And a non-existing test_file path should not work out either
    with pytest.raises(ValueError, match="Could not extract a valid"):
        tt.ModelTest("dummy", test_file="/some/imaginary/path/to/a/testfile")

    # Assert that property access works
    assert mtc.model_name == "dummy"
    assert mtc.test_dir

    # No Multiverse's should be stored
    assert not mtc._mvs

def test_get_cfg_by_name():
    """Test the method to get a config by name"""
    # Get the paths of config files of other tests
    mtc = tt.ModelTest("dummy", test_file=__file__)

    assert mtc.get_cfg_by_name("cfg/run_cfg")

    with pytest.raises(FileNotFoundError, match="No config file found by"):
        mtc.get_cfg_by_name("non-existing-config-file")

    # It should not work without a test_file being passed during init
    mtc2 = tt.ModelTest("dummy")

    with pytest.raises(ValueError):
        mtc2.get_cfg_by_name("foo")

def test_create_mv():
    """Tests the creation of Multiverses using the ModelTest class"""
    mtc = tt.ModelTest("dummy", test_file=__file__)

    # Basic initialization
    mv1 = mtc.create_mv_from_cfg("cfg/run_cfg")
    assert isinstance(mv1, utopya.Multiverse)

    # Pass some more information to the function to call other branches of the
    # function where arguments are inserted (nonzero exit handling and the
    # temporary file path)
    mv2 = mtc.create_mv_from_cfg("cfg/run_cfg",
                                 worker_manager=dict(num_workers=1),
                                 paths=dict(model_note="foo"))
    assert isinstance(mv2, utopya.Multiverse)
    assert mv2.wm.num_workers == 1

def test_tmpdir_is_tmp():
    """This test is to assert that the temporary directory is really temporary"""
    mtc = tt.ModelTest("dummy", test_file=__file__)
    mv = mtc.create_mv_from_cfg("cfg/run_cfg")

    # Extract the path to the temporary directory and assert it was created
    tmpdir_path = mtc._mvs[0]['out_dir'].name
    assert os.path.isdir(tmpdir_path)

    # Let the Multiverse and the ModelTest class go out of scope
    del mtc
    del mv

    # Check if the directory was removed
    assert not os.path.exists(tmpdir_path)
