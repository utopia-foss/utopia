"""Test the Multiverse class initialization and workings.

As the Multiverse will always generate a folder structure, it needs to be taken care that these output folders are temporary and are deleted after the tests. This can be done with the tmpdir fixture of pytest.
"""

import os
import uuid
import pkg_resources

import pytest

from utopya import Multiverse
from utopya.multiverse import distribute_user_cfg

# Get the test resources
RUN_CFG_PATH = pkg_resources.resource_filename('test', 'cfg/run_cfg.yml')
USER_CFG_PATH = pkg_resources.resource_filename('test', 'cfg/user_cfg.yml')
SWEEP_CFG_PATH = pkg_resources.resource_filename('test', 'cfg/sweep_cfg.yml')

# Fixtures ----------------------------------------------------------------
@pytest.fixture
def mv_kwargs(tmpdir) -> dict:
    """Returns a dict that can be passed to Multiverse for initialisation.

    This uses the `tmpdir` fixture provided by pytest, which creates a unique
    temporary directory that is removed after the tests ran through.
    """
    # Create a dict that specifies a unique testing path.
    # The str cast is needed for python version < 3.6
    rand_str = "test_" + uuid.uuid4().hex
    unique_paths = dict(out_dir=tmpdir, model_note=rand_str)

    return dict(model_name="dummy",
                run_cfg_path=RUN_CFG_PATH,
                user_cfg_path=USER_CFG_PATH,
                update_meta_cfg=dict(paths=unique_paths))

@pytest.fixture
def default_mv(mv_kwargs) -> Multiverse:
    """Initialises a unique default configuration of the Multiverse to test everything beyond initialisation.

    Using the mv_kwargs fixture, it is assured that the output directory is
    unique.
    """
    return Multiverse(**mv_kwargs)

# Initialisation tests --------------------------------------------------------

def test_simple_init(mv_kwargs):
    """Tests whether initialisation works for all basic cases."""
    Multiverse(**mv_kwargs)

def test_invalid_model_name_and_operation(default_mv, mv_kwargs):
    """Tests for correct behaviour upon invalid model names"""
    # Try to change the model name
    with pytest.raises(RuntimeError):
        default_mv.model_name = "dummy"

    # Try to instantiate with invalid model name
    mv_kwargs['model_name'] = "invalid_model_RandomShit_bgsbjkbkfvwuRfopiwehGP"
    with pytest.raises(ValueError):
        Multiverse(**mv_kwargs)

def test_config_handling(mv_kwargs):
    """Tests the config handling of the Multiverse"""
    # Multiverse that does not load the default user config
    mv_kwargs['user_cfg_path'] = False
    Multiverse(**mv_kwargs)

    # Testing whether errors are raised
    # Multiverse with wrong run config
    mv_kwargs['run_cfg_path'] = 'an/invalid/run_cfg_path'
    with pytest.raises(FileNotFoundError):
        Multiverse(**mv_kwargs)

def test_granular_backup(mv_kwargs):
    """Tests whether the backup of all config parts works"""
    mv = Multiverse(**mv_kwargs)
    cfg_path = mv.dirs['config']

    assert os.path.isfile(os.path.join(cfg_path, 'base_cfg.yml'))
    assert os.path.isfile(os.path.join(cfg_path, 'user_cfg.yml'))
    assert os.path.isfile(os.path.join(cfg_path, 'model_cfg.yml'))
    assert os.path.isfile(os.path.join(cfg_path, 'run_cfg.yml'))
    assert os.path.isfile(os.path.join(cfg_path, 'update_cfg.yml'))

def test_create_run_dir(default_mv):
    """Tests the folder creation in the initialsation of the Multiverse."""
    mv = default_mv

    # Reconstruct path from meta-config to have a parameter to compare to
    out_dir = str(mv.meta_config['paths']['out_dir'])  # need for python < 3.6
    path_base = os.path.join(out_dir, mv.model_name)

    # get all folders in the output dir
    folders = os.listdir(path_base)

    # select the latest one (there should be only one anyway)
    latest = folders[-1]

    # Check if the subdirectories are present
    for folder in ["config", "eval", "universes"]:
        assert os.path.isdir(os.path.join(path_base, latest, folder)) is True

def test_detect_doubled_folders(mv_kwargs):
    """Tests whether an existing folder will raise an exception."""
    # Init Multiverse
    Multiverse(**mv_kwargs)

    # create output folders again
    # expect error due to existing folders
    with pytest.raises(RuntimeError):
        # And another one, that will also create a directory
        Multiverse(**mv_kwargs)
        Multiverse(**mv_kwargs)
        # NOTE this test assumes that the two calls are so close to each other that the timestamp is the same, that's why there are two calls so that the latest the second call should raise such an error

# Simulation tests ------------------------------------------------------------

def test_run_single(default_mv):
    """Tests a run with a single simulation"""
    default_mv.run_single()
    print("Tasks: ", default_mv.wm.tasks)

    # Test that the universe directory was created
    assert os.path.isdir(os.path.join(default_mv.dirs['universes'], 'uni0'))

def test_run_sweep(mv_kwargs):
    """Tests a run with a single simulation"""
    # Adjust the defaults to use the sweep configuration for run configuration
    mv_kwargs['run_cfg_path'] = SWEEP_CFG_PATH
    mv = Multiverse(**mv_kwargs)

    # Run the sweep
    mv.run_sweep()

    # Print some info.
    print("Tasks: ", mv.wm.tasks)

# Other tests -----------------------------------------------------------------

def test_distribute_user_cfg(tmpdir, monkeypatch):
    """Tests whether user configuration distribution works as desired."""
    # Create a path for the user config test file; needs to be str-cast to
    # allow python < 3.6 implementation of os.path
    test_path = str(tmpdir.join("my_user_cfg.yml"))

    # Execute the distribute function with this path and assert it worked
    distribute_user_cfg(user_cfg_path=test_path)
    assert os.path.isfile(test_path)

    # monkeypatch the "input" function, so that it returns "y" or "no".
    # This simulates the user entering something in the terminal
    # yes-case
    monkeypatch.setattr('builtins.input', lambda x: "y")
    distribute_user_cfg(user_cfg_path=test_path)

    # no-case
    monkeypatch.setattr('builtins.input', lambda x: "n")
    distribute_user_cfg(user_cfg_path=test_path)


# Helpers ---------------------------------------------------------------------
