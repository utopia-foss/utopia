"""Test the Multiverse class initialization and workings."""

import os
import logging
import math

import pytest

from utopya import Multiverse

log = logging.getLogger(__name__)

# Fixtures ----------------------------------------------------------------
#@pytest.fixture

# Tests --------------------------------------------------------------------
#@pytest.mark.skip("Not far enough to initialise this.")
def test_init():
    """Tests the initialization of the Multiverse."""
    #Multiverse()  # fails, if neither default nor metaconfig are present
    instance1 = Multiverse(run_cfg_path="./test/run_config.yml")
    print(instance1.meta_config)
    instance2 = Multiverse(run_cfg_path="./test/run_config.yml", user_cfg_path="./test/user_config.yml")
    print(instance2.meta_config)

    # Testing errors
    with pytest.raises(FileNotFoundError):
        Multiverse(run_cfg_path="not_existing_run_config.yml")

    with pytest.raises(FileNotFoundError):
        Multiverse(user_cfg_path="not_existing_user_config.yml")

#@pytest.mark.skip("To be re-implemented when the Multiverse is further developed.")
def test_create_run_dir(tmpdir,model_name="test_outer_folder_structure"):
    """Tests the folder creation in the initialsation of the Multiverse."""
    # Init Multiverse
    instance = Multiverse(model_name=model_name, run_cfg_path="./test/run_config.yml")
    # set out dir to temp
    instance._meta_config['paths']['out_dir'] = tmpdir.dirpath()
    # make output folder
    instance.create_run_dir()
    # Reconstruct path from settings for testing
    path_base = os.path.expanduser(instance.meta_config['paths']['out_dir'])
    path_base = os.path.join(path_base, model_name)
    # get all folders in the output dir
    all_stuff = os.listdir(path_base)
    # take the latest one
    latest = all_stuff[-1]
    # Check if the folders are present
    assert os.path.isdir(os.path.join(path_base, latest)) is True
    for folder in ["config", "eval", "universes"]:  # may need to adapt
        assert os.path.isdir(os.path.join(path_base, latest, folder)) is True

#@pytest.mark.skip("To be re-implemented when the Multiverse is further developed.")
def test_detect_doubled_folders(tmpdir, model_name="test_universes_doubling"):
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

#@pytest.mark.skip("To be re-implemented when the Multiverse is further developed.")
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
