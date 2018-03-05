
import pytest
import os
from utopya import Multiverse


def test_init():
    """Tests the initialsation of the Multiverse."""
    Multiverse()


def test_create_sim_dir():
    # has to be adapted if user specific path
    # read path names from meta_cfg.yaml
    path_dict = dict(base=None, out_dir='utopia_output', model_name='', model_note='test')
    if path_dict['base'] is None:
            path_base = os.path.expanduser("~")
    else:
        path_base = path_dict['base']
    path_base += "/"+path_dict['out_dir']+"/"+path_dict['model_name']
    all_stuff = os.listdir(path_base)
    print(all_stuff)
    latest = all_stuff[-1]
    print(path_base+latest)
    if not os.path.isdir(path_base+latest):
        print('base directory not created')
    folder_list = ["config", "eval", "universes"]
    for folder in folder_list:
        print(path_base+latest+"/"+folder)
        if not os.path.isdir(path_base+latest+"/"+folder):
            print(folder, 'not created')
            raise RuntimeError
