"""Tests the utopya.testtools modules and the Model class

The reason why the Model class is tested alongside the ModelTest class is
partly historical; however, the overlap is large and the ModelTest class has
the advantage of already working on temporary directories.
"""

import os
import time

import pytest

import utopya
from utopya.testtools import ModelTest


# Fixtures --------------------------------------------------------------------


# Tests -----------------------------------------------------------------------

def test_ModelTest_init():
    """Tests the initialisation and properties of the ModelTest class"""
    # Initialize
    mtc = ModelTest("dummy", test_file=__file__)


    # Assert that property access works
    assert mtc.name == "dummy"
    assert mtc.base_dir

    # No Multiverse's should be stored
    assert not mtc._mvs

    # String representation
    assert "dummy" in str(mtc)

    # Get the default model configuration
    assert isinstance(mtc.default_model_cfg, dict)

    # Non-registered model names should not be possible
    with pytest.raises(KeyError, match="No model with name 'invalid' found"):
        ModelTest("invalid")

    # And a non-existing test_file path should not work out either
    with pytest.raises(ValueError, match="Given base_dir path /some/imag"):
        ModelTest("dummy", test_file="/some/imaginary/path/to/a/testfile")

def test_ModelTest_create_mv():
    """Tests the creation of Multiverses using the ModelTest class"""
    mtc = ModelTest("dummy", test_file=__file__)

    # Basic initialization
    mv1 = mtc.create_mv(from_cfg="cfg/run_cfg.yml")
    assert isinstance(mv1, utopya.Multiverse)

    # Pass some more information to the function to call other branches of the
    # function where arguments are inserted (nonzero exit handling and the
    # temporary file path)
    mv2 = mtc.create_mv(from_cfg="cfg/run_cfg.yml",
                        worker_manager=dict(num_workers=1),
                        paths=dict(model_note="foo"))
    assert isinstance(mv2, utopya.Multiverse)
    assert mv2.wm.num_workers == 1


def test_ModelTest_create_run_load():
    """Tests the chained version of create_mv"""
    mtc = ModelTest("dummy", test_file=__file__)

    # Run with different arguments:
    # Without
    mtc.create_run_load()

    # With a config file in the test directory
    mtc.create_run_load(from_cfg="cfg/run_cfg.yml")

    # With run_cfg_path
    mtc.create_run_load(run_cfg_path=os.path.join(mtc.base_dir,
                                                  "cfg", "run_cfg.yml"))

    # With both
    with pytest.raises(ValueError, match="Can only pass either"):
        mtc.create_run_load(from_cfg="foo", run_cfg_path="bar")

def test_ModelTest_create_frozen_mv():
    """Tests the creation of a FrozenMultiverse using the ModelTest class"""
    mtc = ModelTest("dummy", test_file=__file__)

    # First, create a regular multiverse and run it
    mv = mtc.create_mv(from_cfg="cfg/run_cfg.yml")
    mv.run()

    # Now, load it as a frozen multiverse
    # Need to wait a sec to have a different timestamp for the eval directory
    time.sleep(1.1)
    fmv = mtc.create_frozen_mv(run_dir=mv.dirs['run'])
    assert fmv.dirs['run'] == mv.dirs['run']

def test_ModelTest_tmpdir_is_tmp():
    """This test is to assert that the temporary directory is really temporary"""
    mtc = ModelTest("dummy", test_file=__file__)
    mv = mtc.create_mv(from_cfg="cfg/run_cfg.yml")

    # Extract the path to the temporary directory and assert it was created
    tmpdir_path = mtc._mvs[0]['out_dir'].name
    assert os.path.isdir(tmpdir_path)

    # Let the Multiverse and the ModelTest class go out of scope
    del mtc
    del mv

    # Check if the directory was removed
    assert not os.path.exists(tmpdir_path)

def test_ModelTest_run_and_eval_cfg_paths(tmpdir):
    """Tests the Model.run_and_eval_cfg_paths method works as expected"""
    # The dummy model has no config paisr specified
    dummy_mtc = ModelTest("dummy")
    dummy_cfgs = dummy_mtc.run_and_eval_cfg_paths()
    assert not dummy_cfgs
    assert isinstance(dummy_cfgs, dict)

    # The ForestFire model has
    ff_mtc = ModelTest("ForestFire")
    ff_cfgs = ff_mtc.run_and_eval_cfg_paths()
    assert ff_cfgs
    assert 'multiverse_example' in ff_cfgs
    assert 'universe_example' in ff_cfgs
    assert os.path.isfile(ff_cfgs['multiverse_example']['run'])
    assert os.path.isfile(ff_cfgs['multiverse_example']['eval'])

    # Can also specify an absolute search path
    ff_search_dir = os.path.join(ff_mtc.info_bundle.paths['source_dir'],'cfgs')
    assert os.path.isabs(ff_search_dir)
    ff_cfgs_abs = ff_mtc.run_and_eval_cfg_paths(search_dir=ff_search_dir)
    assert ff_cfgs == ff_cfgs_abs

    # Now, to test error messages, fill some temporary directory with paths
    tmp_search_dir = tmpdir.join("cfgs")
    assert not tmp_search_dir.exists()
    assert not ff_mtc.run_and_eval_cfg_paths(search_dir=tmp_search_dir)

    # Create the directory
    tmp_search_dir.mkdir()
    assert tmp_search_dir.isdir()
    assert not ff_mtc.run_and_eval_cfg_paths(search_dir=tmp_search_dir)

    # Add a subdirectory and some files
    for cfg_name, fnames in (('foo', ('run.yml', 'eval.yml')),
                             ('bar', ('bar_run.yml', 'bar_eval.yml')),
                             ('baz', ('BAZeval.yml',)),
                             ('spam', ('spam_run.yml',)),
                             ('zab', ())):
        cfg_dir = tmp_search_dir.mkdir(cfg_name)

        for fname in fnames:
            with cfg_dir.join(fname).open('w+') as f:
                f.write("hey there")

    # This should work
    ff_cfgs_tmp = ff_mtc.run_and_eval_cfg_paths(search_dir=tmp_search_dir)
    assert len(ff_cfgs_tmp) == 4
    assert 'run' in ff_cfgs_tmp['foo']
    assert 'eval' in ff_cfgs_tmp['foo']

    assert 'run' in ff_cfgs_tmp['bar']
    assert 'eval' in ff_cfgs_tmp['bar']

    assert 'run' not in ff_cfgs_tmp['baz']
    assert 'eval' in ff_cfgs_tmp['baz']

    assert 'run' in ff_cfgs_tmp['spam']
    assert 'eval' not in ff_cfgs_tmp['spam']

    assert 'zab' not in ff_cfgs_tmp

    # Now, add some surplus eval file, which should fail
    extra_eval_cfg_path = tmp_search_dir.join('baz', 'baz2eval.yml')
    with extra_eval_cfg_path.open('w+') as f:
        f.write("I shouldn't be here; there already is an eval config file!")

    with pytest.raises(ValueError, match=r"Can have at most one `\*eval\.yml"):
        ff_mtc.run_and_eval_cfg_paths(search_dir=tmp_search_dir)

    # Remove it again, and add an extra run configuration
    os.remove(extra_eval_cfg_path)
    extra_run_cfg_path = tmp_search_dir.join('spam', 'spam2run.yml')
    with extra_run_cfg_path.open('w+') as f:
        f.write("I shouldn't be here; there already is a run config file!")

    with pytest.raises(ValueError, match=r"Can have at most one `\*run\.yml"):
        ff_mtc.run_and_eval_cfg_paths(search_dir=tmp_search_dir)
