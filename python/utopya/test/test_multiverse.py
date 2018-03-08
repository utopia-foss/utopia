"""Test the Multiverse class initialization and workings."""

import os
import logging
import math

import pkg_resources

import pytest

from utopya import Multiverse

import time

log = logging.getLogger(__name__)

BASE_RUNCFG_PATH = pkg_resources.resource_filename('test', 'cfg/run_cfg.yml')
BASE_USERCFG_PATH = pkg_resources.resource_filename('test', 'cfg/user_cfg.yml')

# Fixtures ----------------------------------------------------------------
@pytest.fixture
def mv_kwargs(tmpdir) -> dict:
    """Returns a dict that can be passed to Multiverse for initialisation"""
    return dict(model_name="dummy", run_cfg_path=BASE_RUNCFG_PATH, user_cfg_path=BASE_USERCFG_PATH)

# Tests --------------------------------------------------------------------
#@pytest.mark.skip("Not far enough to initialise this.")
def test_config_handling(mv_kwargs, tmpdir):
    """Tests the initialization of the Multiverse."""
    mv_local = mv_kwargs
    local_config = dict(paths=dict(out_dir=tmpdir.dirpath(), model_note="test_user_and_run_cfg"))
    #
    # Multiverse()  with special user and run config
    Multiverse(**mv_kwargs, update_meta_cfg=local_config)
    # Multiverse() with default user config and run config
    mv_local['user_cfg_path'] = None
    local_config['model_note'] = "test_run_cfg"
    time.sleep(1)
    Multiverse(**mv_local, update_meta_cfg=local_config)
    # Testing errors
    # Multiverse with wrong run config
    with pytest.raises(FileNotFoundError):
        mv_local['run_cfg_path'] = './unvalid_run_cfg.yml'
        local_config['model_note'] = "test_not existing_run_cfg"
        Multiverse(**mv_local, update_meta_cfg=local_config)

    with pytest.raises(FileNotFoundError):
        mv_local['run_cfg_path'] = './unvalid_user_cfg.yml'
        local_config['model_note'] = "test_not existing_user_cfg"
        Multiverse(**mv_local, update_meta_cfg=local_config)


#@pytest.mark.skip("To be re-implemented when the Multiverse is further developed.")
def test_create_run_dir(mv_kwargs, tmpdir):
    """Tests the folder creation in the initialsation of the Multiverse."""
    local_config = dict(paths=dict(out_dir=tmpdir.dirpath(), model_note="test_outer_directories"))
    # Init Multiverse
    instance = Multiverse(**mv_kwargs, update_meta_cfg=local_config)
    # Reconstruct path from settings for testing
    path_base = os.path.expanduser(instance.meta_config['paths']['out_dir'])
    path_base = os.path.join(path_base, mv_kwargs['model_name'])
    # get all folders in the output dir
    all_stuff = os.listdir(path_base)
    # take the latest one
    latest = all_stuff[-1]
    # Check if the folders are present
    assert os.path.isdir(os.path.join(path_base, latest)) is True
    for folder in ["config", "eval", "universes"]:  # may need to adapt
        assert os.path.isdir(os.path.join(path_base, latest, folder)) is True

@pytest.mark.skip("To be re-implemented when the Multiverse is further developed.")
def test_detect_doubled_folders(mv_kwargs, tmpdir):
    local_config = dict(paths=dict(out_dir=tmpdir.dirpath(), model_note="test_universes_doubling"))
    # Init Multiverse
    instance = Multiverse(model_name=model_name, run_cfg_path="./test/run_config.yml")
    # set out dir to temp
    instance._meta_config['paths']['out_dir'] = tmpdir.dirpath()
    # make output folder
    instance.create_run_dir()
    # create output folders again
    # expect error due to existing folders
    with pytest.raises(RuntimeError):
        # make output folder
        instance.create_run_dir()

@pytest.mark.skip("To be re-implemented when the Multiverse is further developed.")
def test_create_uni_dir(tmpdir):
    for i, max_uni in enumerate([1, 9, 10, 11, 99, 100, 101]):
        single_create_uni_dir(tmpdir, "test_universes_folder_structure_{}".format(i), max_uni)
    # test for possible wrong inputs
    # Init Multiverse
    instance = Multiverse(model_name="test_universes_folder_structure_for_errors", run_cfg_path="./test/run_config.yml")
    # negative numbers:
    with pytest.raises(RuntimeError):
            instance._create_uni_dir(uni_id=-1, max_uni_id=-1)
    # maximum below uni_id:
    with pytest.raises(RuntimeError):
            instance._create_uni_dir(uni_id=5, max_uni_id=4)    

@pytest.mark.skip("To be re-implemented when the Multiverse is further developed.")
def test_single_sim(mv_kwargs):
    """Tests a run with a single simulation"""
    mv = Multiverse(**mv_kwargs)

    mv.run_single()

# Helpers ---------------------------------------------------------------------

def single_create_uni_dir(tmpdir, model_name="test_universe_folder_structure_single", maximum=10):
    # Init Multiverse
    instance = Multiverse(model_name=model_name, run_cfg_path="./test/run_config.yml")
    # set out dir to temp
    instance._meta_config['paths']['out_dir'] = tmpdir.dirpath()
    instance.create_run_dir()
    # Create the universe directories
    for i in range(maximum + 1):
        instance._create_uni_dir(uni_id=i, max_uni_id=maximum)
    # get the path of the universes folder
    path = instance._dirs['universes']
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
