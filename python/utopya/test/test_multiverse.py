"""Test the Multiverse class initialization and workings.

As the Multiverse will always generate a folder structure, it needs to be taken care that these output folders are temporary and are deleted after the tests. This can be done with the tmpdir fixture of pytest.
"""

import os
import uuid
from pkg_resources import resource_filename

import pytest

from utopya import Multiverse, FrozenMultiverse

# Get the test resources
RUN_CFG_PATH = resource_filename('test', 'cfg/run_cfg.yml')
USER_CFG_PATH = resource_filename('test', 'cfg/user_cfg.yml')
BAD_USER_CFG_PATH = resource_filename('test', 'cfg/bad_user_cfg.yml')
SWEEP_CFG_PATH = resource_filename('test', 'cfg/sweep_cfg.yml')

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
                paths=unique_paths)

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
    # With the full set of arguments
    Multiverse(**mv_kwargs)

    # Without the run configuration
    mv_kwargs.pop('run_cfg_path')
    mv_kwargs['paths']['model_note'] += "_wo_run_cfg"
    Multiverse(**mv_kwargs)

    # Suppressing the user config
    mv_kwargs['user_cfg_path'] = False
    mv_kwargs['paths']['model_note'] += "_wo_user_cfg"
    Multiverse(**mv_kwargs)
    # NOTE Without specifying a path, the search path will be used, which makes
    # the results untestable and creates spurious folders for the user.
    # Therefore, we cannot test for the case where no user config is given ...

    # Test with a bad user config
    mv_kwargs['user_cfg_path'] = BAD_USER_CFG_PATH
    mv_kwargs['paths']['model_note'] = "bad_user_cfg"
    with pytest.raises(ValueError, match="There was a 'parameter_space' key"):
        Multiverse(**mv_kwargs)

def test_invalid_model_name_and_operation(default_mv, mv_kwargs):
    """Tests for correct behaviour upon invalid model names"""
    # Try to change the model name although it was already set
    with pytest.raises(RuntimeError, match="A Multiverse's associated"):
        default_mv.model_name = "dummy"

    # Try to instantiate with invalid model name
    mv_kwargs['model_name'] = "invalid_model_RandomShit_bgsbjkbkfvwuRfopiwehGP"
    with pytest.raises(ValueError, match="No such model 'invalid_model_Ran"):
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
    out_dir = str(mv.meta_cfg['paths']['out_dir'])  # need str for python < 3.6
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
    with pytest.raises(RuntimeError, match="Simulation directory already"):
        # And another one, that will also create a directory
        Multiverse(**mv_kwargs)
        Multiverse(**mv_kwargs)
        # NOTE this test assumes that the two calls are so close to each other
        #      that the timestamp is the same, that's why there are two calls
        #      so that the latest the second call should raise such an error

# Simulation tests ------------------------------------------------------------

def test_run_single(default_mv):
    """Tests a run with a single simulation"""
    # Run a single simulation using the default multiverse
    default_mv.run_single()

    # Test that the universe directory was created as proxy of run finished
    assert os.path.isdir(os.path.join(default_mv.dirs['universes'], 'uni0'))

def test_run_sweep(mv_kwargs):
    """Tests a run with a single simulation"""
    # Adjust the defaults to use the sweep configuration for run configuration
    mv_kwargs['run_cfg_path'] = SWEEP_CFG_PATH
    mv = Multiverse(**mv_kwargs)

    # Run the sweep
    mv.run_sweep()

    # With a parameter space without volume, i.e. without any sweeps added,
    # the sweep should not be possible
    mv_kwargs['run_cfg_path'] = RUN_CFG_PATH
    mv_kwargs['paths']['model_note'] = "_invalid_cfg"
    mv = Multiverse(**mv_kwargs)

    with pytest.raises(ValueError, match="The parameter space has no sweeps"):
        mv.run_sweep()

def test_multiple_runs_not_allowed(mv_kwargs):
    """Assert that multiple runs are prohibited"""
    # Create Multiverse and run
    mv = Multiverse(**mv_kwargs)
    mv.run_single()

    # Another run should not be possible
    with pytest.raises(RuntimeError, match="Could not add simulation task"):
        mv.run_single()


# FrozenMultiverse tests ------------------------------------------------------

def test_FrozenMultiverse(mv_kwargs):
    """Test the FrozenMultiverse class"""
    # Need a regular Multiverse and corresponding output for that
    mv = Multiverse(**mv_kwargs)
    mv.run_single()

    # Now create a frozen Multiverse from that one
    # Without run directory, the latest one should be loaded
    print("\nInitializing FrozenMultiverse without further kwargs")
    FrozenMultiverse(**mv_kwargs)

    # NOTE For the other tests, need to adjust the data directory in order to
    # not create collisions in the eval directory due to same timestamps

    # With a relative path, the corresponding directory should be found
    print("\nInitializing FrozenMultiverse with timestamp as run_dir")
    FrozenMultiverse(**mv_kwargs, run_dir=os.path.basename(mv.dirs['run']),
                     data_manager=dict(out_dir="eval/{date:}_2"))

    # With an absolute path, that path should be used directly
    print("\nInitializing FrozenMultiverse with absolute path to run_dir")
    FrozenMultiverse(**mv_kwargs, run_dir=mv.dirs['run'],
                     data_manager=dict(out_dir="eval/{date:}_3"))

    # With a relative path, the path relative to the CWD should be used
    print("\nInitializing FrozenMultiverse with relative path to run_dir")
    FrozenMultiverse(**mv_kwargs, run_dir=os.path.relpath(mv.dirs['run'],
                                                          start=os.getcwd()),
                     data_manager=dict(out_dir="eval/{date:}_4"))

    # Bad type of run directory should fail
    with pytest.raises(TypeError, match="Argument run_dir needs"):
        FrozenMultiverse(**mv_kwargs, run_dir=123,
                         data_manager=dict(out_dir="eval/{date:}_5"))

    # Non-existing directory should fail
    with pytest.raises(IOError, match="No directory found at"):
        FrozenMultiverse(**mv_kwargs, run_dir="my_non-existing_directory",
                         data_manager=dict(out_dir="eval/{date:}_6"))

