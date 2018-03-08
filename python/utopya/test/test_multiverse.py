"""Test the Multiverse class initialization and workings."""

import os
import logging
import math
import pkg_resources

import pytest

from utopya import Multiverse
from utopya.multiverse import distribute_user_cfg


log = logging.getLogger(__name__)

BASE_RUNCFG_PATH = pkg_resources.resource_filename('test', 'cfg/run_cfg.yml')
BASE_USERCFG_PATH = pkg_resources.resource_filename('test', 'cfg/user_cfg.yml')

# Fixtures ----------------------------------------------------------------
@pytest.fixture
def mv_kwargs(tmpdir) -> dict:
    """Returns a dict that can be passed to Multiverse for initialisation"""
    return dict(model_name="dummy", run_cfg_path=BASE_RUNCFG_PATH, user_cfg_path=BASE_USERCFG_PATH)

# Initialisation tests --------------------------------------------------------

@pytest.mark.skip("TODO")
def test_init():
    """Tests whether initialisation works for all cases."""
    raise NotImplementedError

def test_invalid_model_name_and_operation(mv_kwargs, tmpdir):
    """Tests for correct behaviour upon invalid model names"""
    mv_local = mv_kwargs

    # Try to change the model name
    local_config = dict(paths=dict(out_dir=tmpdir.dirpath(), model_note="test_try_change_model_name"))
    instance = Multiverse(**mv_local, update_meta_cfg=local_config)
    with pytest.raises(RuntimeError):
        instance.model_name = "dummy"

    # Try invalid model name  
    local_config = dict(paths=dict(out_dir=tmpdir.dirpath(),
                                   model_note="test_invalid_model_name"))
    mv_local['model_name'] = "invalid_model_RandomShit_bgsbjkbkfvwuRfopiwehGEP"

    with pytest.raises(ValueError):
        Multiverse(**mv_local, update_meta_cfg=local_config)

def test_config_handling(mv_kwargs, tmpdir):
    """Tests the config handling of the Multiverse"""
    mv_local = mv_kwargs
    local_config = dict(paths=dict(out_dir=tmpdir.dirpath(),
                                   model_note="test_user_and_run_cfg"))
    
    # Multiverse  with special user and run config
    Multiverse(**mv_kwargs, update_meta_cfg=local_config)

    # Multiverse with default user config and run config
    mv_local['user_cfg_path'] = None
    local_config['paths']['model_note'] = "test_run_cfg"
    Multiverse(**mv_local, update_meta_cfg=local_config)

    # Testing whether errors are raises
    # Multiverse with wrong run config
    mv_local['run_cfg_path'] = './invalid_run_cfg.yml'
    local_config['paths']['model_note'] = "test_not existing_run_cfg"
    with pytest.raises(FileNotFoundError):
        Multiverse(**mv_local, update_meta_cfg=local_config)

def test_create_run_dir(mv_kwargs, tmpdir):
    """Tests the folder creation in the initialsation of the Multiverse."""
    local_config = dict(paths=dict(out_dir=tmpdir.dirpath(),
                                   model_note="test_outer_directories"))

    # Init Multiverse
    instance = Multiverse(**mv_kwargs, update_meta_cfg=local_config)

    # Reconstruct path from settings for testing
    path_base = os.path.expanduser(instance.meta_config['paths']['out_dir'])
    path_base = os.path.join(path_base, mv_kwargs['model_name'])

    # get all folders in the output dir
    folders = os.listdir(path_base)

    # take the latest one
    latest = folders[-1]

    # Check if the folders are present
    assert os.path.isdir(os.path.join(path_base, latest)) is True
    for folder in ["config", "eval", "universes"]:  # may need to adapt
        assert os.path.isdir(os.path.join(path_base, latest, folder)) is True

def test_detect_doubled_folders(mv_kwargs, tmpdir):
    """Tests whether an existing folder will raise an exception."""
    local_config = dict(paths=dict(out_dir=tmpdir.dirpath(),
                                   model_note="test_universes_doubling"))

    # Init Multiverse
    Multiverse(**mv_kwargs, update_meta_cfg=local_config)

    # create output folders again
    # expect error due to existing folders
    with pytest.raises(RuntimeError):
        # make output folder
        Multiverse(**mv_kwargs, update_meta_cfg=local_config)

def test_create_uni_dir(mv_kwargs, tmpdir):
    """Test creation of the uni directory"""
    local_config = dict(paths=dict(out_dir=tmpdir.dirpath(),
                                   model_note="test_uni_directories"))

    for i, max_uni in enumerate([1, 9, 10, 11, 99, 100, 101]):
        local_config['paths']['model_note'] = "test_universes_folder_structure_{}".format(i)
        single_create_uni_dir(mv_kwargs, local_config, max_uni)

    # test for possible wrong inputs
    # Init Multiverse
    local_config['paths']['model_note'] = "test_uni_id_consistency"
    mv = Multiverse(**mv_kwargs, update_meta_cfg=local_config)

    # negative numbers:
    with pytest.raises(RuntimeError):
            mv._create_uni_dir(uni_id=-1, max_uni_id=-1)

    # maximum below uni_id:
    with pytest.raises(RuntimeError):
            mv._create_uni_dir(uni_id=5, max_uni_id=4)    


# Simulation tests ------------------------------------------------------------

@pytest.mark.skip("To be re-implemented when the Multiverse is further developed.")
def test_single_sim(mv_kwargs):
    """Tests a run with a single simulation"""
    mv = Multiverse(**mv_kwargs)

    mv.run_single()

# Other tests -----------------------------------------------------------------

def test_distribute_user_cfg(tmpdir, monkeypatch):
    """Tests whether user configuration distribution works as desired."""
    test_path = os.path.join(tmpdir.dirpath(), "my_user_cfg.yml")
    distribute_user_cfg(user_cfg_path=test_path)

    assert os.path.isfile(test_path)

    # monkeypatch the "input" function, so that it returns "y" or "no".
    # This simulates the user entering something in the terminal
    # no-case
    monkeypatch.setattr('builtins.input', lambda x: "y")
    distribute_user_cfg(user_cfg_path=test_path)

    # yes-case
    monkeypatch.setattr('builtins.input', lambda x: "n")
    distribute_user_cfg(user_cfg_path=test_path)


# Helpers ---------------------------------------------------------------------

def single_create_uni_dir(mv_kwargs, local_config, maximum=10):
    """A helper function to create a single uni dir"""
    # Init Multiverse
    mv = Multiverse(**mv_kwargs, update_meta_cfg=local_config)

    # Create the universe directories
    for i in range(maximum + 1):
        mv._create_uni_dir(uni_id=i, max_uni_id=maximum)

    # get the path of the universes folder
    path = mv.dirs['universes']

    # calculate the number of needed filling zeros dependend on the maximum number of different calulations
    number_filling_zeros = math.ceil(math.log(maximum + 1, 10))
    
    # check that minimal number of filling zeros (at maximum no zero in front)
    if maximum > 0:
        uni = "uni" + str(maximum).zfill(number_filling_zeros)
        assert uni[3] != '0'

    # check if all universe directories are created
    for i in range(maximum + 1):
        path_uni = os.path.join(path, "uni" + str(i).zfill(number_filling_zeros))
        assert os.path.isdir(path_uni) is True
