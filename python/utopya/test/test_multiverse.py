
import os
import shutil
import logging
import time
import math
import random as rd

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
    """Tests the initialsation of the Multiverse."""
    # FIXME
    Multiverse()

#@pytest.mark.skip("To be re-implemented when the Multiverse is further developed.")
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
    for folder in ["config", "eval", "universes"]: # may need to adapt
        assert os.path.isdir(os.path.join(path_base, latest, folder)) is True

#@pytest.mark.skip("To be re-implemented when the Multiverse is further developed.")
def test_detect_doubled_folders(tmpdir, mv_config):
    # adapt cfg to special needs
    mv_config['paths']['out_dir'] = tmpdir.dirpath()
    mv_config['paths']['model_name'] = "test_universes_doubling"
    # create two Multiverses after another (within one second) 
    # expect error due to existing folders
    Multiverse(mv_config)
    with pytest.raises(RuntimeError):
        Multiverse(mv_config)

#@pytest.mark.skip("To be re-implemented when the Multiverse is further developed.")
def test_create_uni_dir(tmpdir,mv_config):
    # adapt cfg to special needs
    mv_config['paths']['out_dir'] = tmpdir.dirpath()
    for i, max_uni in enumerate([1, 9, 10, 11, 99, 100, 101]):
        # adapt cfg to special needs
        mv_config['paths']['model_name'] = "test_universes_folder_structure"+str(i)
        single_create_uni_dir(tmpdir, mv_config, max_uni)

def single_create_uni_dir(tmpdir, mv_config, maximum=10):
    # Init Multiverse
    instance = Multiverse(mv_config)
    # Create the universe directories
    for i in range(0, maximum+1):
        instance._create_uni_dir(i, maximum)
    # get the path of the universes folder
    path = instance.dirs['universes']
    # calculate the number of needed filling zeros dependend on the maximum number of different calulations
    number_filling_zeros = math.ceil(math.log(maximum+1, 10))

    # check if all universe directories are created
    for i in range(0, maximum+1):
        path_uni = os.path.join(path, "uni"+str(i).zfill(number_filling_zeros))
        assert os.path.isdir(path_uni) is True
        # check if the calculation was fine (not too many leading zeros), by checking if the last one has no leading zeros
        if i == maximum:
            assert path_uni[4] != '0'
