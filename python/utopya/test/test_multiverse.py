"""Test the Multiverse class initialization and workings."""

import os
import logging
import math

import pytest

from utopya import Multiverse

log = logging.getLogger(__name__)

# Fixtures ----------------------------------------------------------------
@pytest.fixture
def mv_config():
    return dict(paths=dict(out_dir='~/utopia_output', model_name='', model_note=''))

# Tests --------------------------------------------------------------------
@pytest.mark.skip("Not far enough to initialise this.")
def test_init():
    """Tests the initialization of the Multiverse."""
    Multiverse()  # fails, if neither default nor metaconfig are present
    Multiverse(metaconfig="metaconfig.yml")
    Multiverse(metaconfig="metaconfig.yml", userconfig="userconfig.yml")

    # Testing errors
    with pytest.raises(FileNotFoundError):
        Multiverse(metaconfig="not_existing_metaconfig.yml")

    with pytest.raises(FileNotFoundError):
        Multiverse(userconfig="not_existing_userconfig.yml")

@pytest.mark.skip("To be re-implemented when the Multiverse is further developed.")
def test_create_sim_dir(tmpdir, mv_config):
    """Tests the folder creation in the initialsation of the Multiverse."""
    # has to be adapted if user specific path
    # read path names from meta_cfg.yaml

    # adapt cfg to special needs
    mv_config['paths']['out_dir'] = tmpdir.dirpath()
    mv_config['paths']['model_name'] = "test_outer_folder_structure"

    # Init Multiverse
    Multiverse(mv_config)
    # Reconstruct path from settings for testing
    path_base = os.path.expanduser(mv_config['paths']['out_dir'])
    path_base = os.path.join(path_base, mv_config['paths']['model_name'])
    # get all folders in the output dir
    all_stuff = os.listdir(path_base)
    # take the latest one
    latest = all_stuff[-1]
    # Check if the folders are present
    assert os.path.isdir(os.path.join(path_base, latest)) is True
    for folder in ["config", "eval", "universes"]:  # may need to adapt
        assert os.path.isdir(os.path.join(path_base, latest, folder)) is True

@pytest.mark.skip("To be re-implemented when the Multiverse is further developed.")
def test_detect_doubled_folders(tmpdir, mv_config):
    # adapt cfg to special needs
    mv_config['paths']['out_dir'] = tmpdir.dirpath()
    mv_config['paths']['model_name'] = "test_universes_doubling"
    # create two Multiverses after another (within one second) 
    # expect error due to existing folders
    Multiverse(mv_config)
    with pytest.raises(RuntimeError):
        Multiverse(mv_config)

@pytest.mark.skip("To be re-implemented when the Multiverse is further developed.")
def test_create_uni_dir(tmpdir, mv_config):
    # adapt cfg to special needs
    mv_config['paths']['out_dir'] = tmpdir.dirpath()
    for i, max_uni in enumerate([1, 9, 10, 11, 99, 100, 101]):
        # adapt cfg to special needs
        mv_config['paths']['model_name'] = "test_universes_folder_structure_{}".format(i)
        single_create_uni_dir(tmpdir, mv_config, max_uni)
    # test for possible wrong inputs
    mv_config['paths']['model_name'] = "test_universes_folder_structure_for_errors"
    # Init Multiverse
    instance = Multiverse(mv_config)
    # negative numbers:
    with pytest.raises(RuntimeError):
            instance._create_uni_dir(uni_id=-1, max_uni_id=-1)
    # maximum below uni_id:
    with pytest.raises(RuntimeError):
            instance._create_uni_dir(uni_id=5, max_uni_id=4)    


def single_create_uni_dir(tmpdir, mv_config, maximum=10):
    # Init Multiverse
    instance = Multiverse(mv_config)
    # Create the universe directories
    for i in range(maximum + 1):
        instance._create_uni_dir(uni_id=i, max_uni_id=maximum)
    # get the path of the universes folder
    path = instance.dirs['universes']
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
