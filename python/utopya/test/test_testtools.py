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
from utopya.cfg import write_to_cfg_dir, load_from_cfg_dir


# Fixtures --------------------------------------------------------------------

@pytest.fixture
def tmp_utopya_cfg():
    """Removes the current utopya_cfg.yml and puts it back upon teardown. This
    fixture allows putting custom content in that place without persistently
    changing the test-executing user's local configuration.
    """
    previous_state = load_from_cfg_dir("utopya")
    write_to_cfg_dir("utopya", dict())
    yield
    write_to_cfg_dir("utopya", previous_state)



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

def test_ModelTest_default_config_sets():
    """Tests the default config sets"""
    # The dummy model has no config sets specified
    dummy_mtc = ModelTest("dummy")
    dummy_cfgs = dummy_mtc.default_config_sets
    assert not dummy_cfgs
    assert isinstance(dummy_cfgs, dict)

    # The ForestFire model has
    ff_mtc = ModelTest("ForestFire")
    ff_cfgs = ff_mtc.default_config_sets
    assert ff_cfgs
    assert 'multiverse_example' in ff_cfgs
    assert 'universe_example' in ff_cfgs
    assert os.path.isdir(ff_cfgs['multiverse_example']['dir'])
    assert os.path.isfile(ff_cfgs['multiverse_example']['run'])
    assert os.path.isfile(ff_cfgs['multiverse_example']['eval'])

    mv_set = ff_mtc.get_config_set('multiverse_example')
    assert mv_set == ff_cfgs['multiverse_example']

def test_ModelTest_config_sets_custom_search_dirs(tmp_utopya_cfg, tmpdir):
    """Tests that users can specify a custom search directory via the utopya
    configuration file. The ``tmp_utopya_cfg`` fixture removes the current
    user's config file for the duration of the test.
    """
    ff_mtc = ModelTest("ForestFire")

    # With the user-specified file remove, there should definitely be no
    # _custom_ search directories, only the one in the model source directory
    sdirs = ff_mtc.default_config_set_search_dirs
    assert len(sdirs) == 1
    assert sdirs[0].endswith("src/utopia/models/ForestFire/cfgs")

    # Now add some paths to the utopya config file
    custom_search_dirs = [
        "/some/absolute/path",
        "some/relative/path",
        "~/foo/bar",
        str(tmpdir.join("foo")),
        str(tmpdir.join("bar/{model_name:}/spam")),
    ]
    write_to_cfg_dir("utopya", dict(config_set_search_dirs=custom_search_dirs))

    # These should all appear in the default config set search directories
    sdirs = ff_mtc.default_config_set_search_dirs

    assert "/some/absolute/path" in sdirs
    assert "some/relative/path" in sdirs
    assert "~/foo/bar" in sdirs
    assert str(tmpdir.join("foo")) in sdirs
    assert str(tmpdir.join("bar/ForestFire/spam")) in sdirs

    # Try again with bad type
    write_to_cfg_dir("utopya", dict(config_set_search_dirs="not a list"))
    with pytest.raises(TypeError, match="needs to be a list!"):
        ff_mtc.default_config_set_search_dirs


def test_ModelTest_get_config_set_extra_dir(tmp_utopya_cfg, tmpdir):
    """Tests config set retrieval (except for custom search directories, which
    are already tested separately above.
    """
    ff_mtc = ModelTest("ForestFire")

    custom_cfgs = tmpdir.mkdir("custom_configs")
    extra_dir = custom_cfgs.mkdir("foo")

    # Without the files existing, won't find anything
    with pytest.raises(ValueError, match="No config set with name 'foo'"):
        ff_mtc.get_config_set(extra_dir)

    # Now add the custom configuration files
    with extra_dir.join("run.yml").open("w+") as f:
        f.write("some run config")
    with extra_dir.join("eval.yml").open("w+") as f:
        f.write("some eval config")

    cfg_set = ff_mtc.get_config_set(extra_dir)
    assert cfg_set["run"] == extra_dir.join("run.yml")
    assert cfg_set["eval"] == extra_dir.join("eval.yml")
    assert cfg_set["dir"] == extra_dir

    # Can also be a relative path
    cfg_set = ff_mtc.get_config_set(os.path.relpath(extra_dir))
    assert cfg_set["run"] == extra_dir.join("run.yml")
    assert cfg_set["eval"] == extra_dir.join("eval.yml")
    assert cfg_set["dir"] == extra_dir

    # Extra files are ignored
    with extra_dir.join("also_run.yml").open("w+") as f:
        f.write("another run config, ignored")
    with extra_dir.join("more_eval.yml").open("w+") as f:
        f.write("another eval config, ignored")

    cfg_set = ff_mtc.get_config_set(extra_dir)
    assert cfg_set["run"] == extra_dir.join("run.yml")
    assert cfg_set["eval"] == extra_dir.join("eval.yml")
    assert cfg_set["dir"] == extra_dir

    # With a file removed, it no longer appears in the config set
    os.remove(extra_dir.join("run.yml"))
    cfg_set = ff_mtc.get_config_set(extra_dir)
    assert "run" not in cfg_set
    assert cfg_set["eval"] == extra_dir.join("eval.yml")
    assert cfg_set["dir"] == extra_dir

    with extra_dir.join("run.yml").open("w+") as f:
        f.write("some run config")
    os.remove(extra_dir.join("eval.yml"))
    cfg_set = ff_mtc.get_config_set(extra_dir)
    assert cfg_set["run"] == extra_dir.join("run.yml")
    assert "eval" not in cfg_set
    assert cfg_set["dir"] == extra_dir

    # All removed again
    os.remove(extra_dir.join("run.yml"))
    with pytest.raises(ValueError, match="No config set with name 'foo'"):
        ff_mtc.get_config_set(extra_dir)

    # Providing a path to a directory that does not exist
    with pytest.raises(ValueError, match="No config set .*'/some/weird/path'"):
        ff_mtc.get_config_set("/some/weird/path")

    # Providing a path to a directory that *does* exist but contains no files
    # NOTE The error here references `i_exist` by name, because the directory
    #      actually exists but contains no run.yml or eval.yml file
    tmpdir.mkdir("i_exist")
    assert os.path.isdir(tmpdir.join("i_exist"))
    with pytest.raises(ValueError, match="No config set with name 'i_exist'"):
        ff_mtc.get_config_set(tmpdir.join("i_exist"))

    # Providing a config set with a name that already exists will yield the new
    # one (and log a warning)
    extra_dir = custom_cfgs.mkdir("multiverse_example")
    with extra_dir.join("run.yml").open("w+") as f:
        f.write("some run config")

    cfg_set = ff_mtc.get_config_set(extra_dir)
    assert cfg_set["run"] == extra_dir.join("run.yml")
    assert "eval" not in cfg_set
    assert cfg_set["dir"] == extra_dir
