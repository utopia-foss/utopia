"""Tests the UtopiaDataManager and involved functions and classes."""

import pytest

from utopya import DataManager
from utopya.tools import write_yml

# Fixtures --------------------------------------------------------------------

@pytest.fixture
def data_dir(tmpdir) -> str:
    """Writes dummy data to a temporary directory and returns that directory"""
    
    # Create YAML dummy data and write it out
    foobar = dict(one=1, two=2,
                  go_deeper=dict(eleven=11),
                  a_list=list(range(10)))

    lamo = dict(nothing="to see here")

    write_yml(foobar, path=tmpdir.join("foobar.yml"))
    write_yml(lamo, path=tmpdir.join("lamo.yml"))
    write_yml(lamo, path=tmpdir.join("also_lamo.yml"))
    write_yml(lamo, path=tmpdir.join("looooooooooong_filename.yml"))

    subdir = tmpdir.mkdir("sub")
    write_yml(foobar, path=subdir.join("abc123.yml"))
    write_yml(foobar, path=subdir.join("abcdef.yml"))

    return tmpdir

# Tests -----------------------------------------------------------------------

def test_init(data_dir):
    """Tests initialisation of the Utopia data manager"""
    DataManager(str(data_dir))
