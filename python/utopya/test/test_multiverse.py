
import pytest
import os
import logging
import time
import math
from utopya import Multiverse

log = logging.getLogger(__name__)

pytest.mark.skip("Not far enough to initialise this.")
def test_init():
    """Tests the initialsation of the Multiverse."""
    Multiverse()

pytest.mark.skip("Have to wait for user-specific config file to do this.")
def test_create_sim_dir():
    # has to be adapted if user specific path
    # read path names from meta_cfg.yaml
    path_dict = dict(out_dir='~/utopia_output', model_name='', model_note='')
    path_base = os.path.expanduser(path_dict['out_dir'])
    path_base = os.path.join(path_base, path_dict['model_name'])
    all_stuff = os.listdir(path_base)
    latest = all_stuff[-1]
    if not os.path.isdir(os.path.join(path_base, latest)):
        log.debug("Base directory not found %s", path_base)
        raise RuntimeError
    folder_list = ["config", "eval", "universes"]  # may need to adapt
    for folder in folder_list:
        if not os.path.isdir(os.path.join(path_base, latest, folder)):
            log.debug("Inner directory not found %s at %s", folder, os.path.join(path_base, latest, folder))
            raise RuntimeError

pytest.mark.skip("Have to wait for user-specific config file to do this.")
def test_create_uni_dir(maximum=10):
    time.sleep(1)
    instance = Multiverse()
    for i in range(0, maximum):
        instance._create_uni_dir(i, maximum)
    path = instance.dirs['universes']
    for i in range(0, maximum):
        if not os.path.isdir(os.path.join(path, "uni"+str(i).zfill(math.ceil(math.log(maximum+1, 10))))):
            log.debug("Uni directory not found %s at %s", i, path)
            raise RuntimeError
