"""Tests the internal _path_setup module"""

import os
import sys
import copy

import pytest

import utopya.cfg as ucfg
from utopya.cfg import write_to_cfg_dir
from utopya._path_setup import add_modules_to_path


# Fixtures --------------------------------------------------------------------
from .test_cfg import tmp_cfg_dir

@pytest.fixture
def tmp_sys_path():
    """Work on a temporary sys.path"""
    initial_sys_path = copy.deepcopy(sys.path)
    yield
    
    sys.path = initial_sys_path

# -----------------------------------------------------------------------------

def test_add_modules_to_path(tmp_sys_path, tmp_cfg_dir):
    """Test the add_modules_to_path function"""
    # Write some paths to the configuration
    paths = dict(foo="/foo/bar/baz",
                 spam="/spam/spam")
    write_to_cfg_dir('external_module_paths', paths)

    # Should now be able to add these to sys.path via add_modules_to_path
    assert all([p not in sys.path for p in paths.values()])
    add_modules_to_path('foo', 'spam')
    assert all([p in sys.path for p in paths.values()])

    # Error on missing module
    with pytest.raises(KeyError, match="Missing configuration entry"):
        add_modules_to_path('invalid')

    # ... but can also ignore it
    add_modules_to_path('invalid', ignore_missing=True)
