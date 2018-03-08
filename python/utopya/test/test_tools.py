"""I'm a docstring."""
import pytest
import os
import copy
import utopya.tools as t


# Fixtures
@pytest.fixture
def testdict():
    """Create a dummy dictionary."""
    testdict = dict()
    for i, s in enumerate(list("Test")):
        testdict[s] = i
    return testdict


@pytest.fixture
def testdict_deep():
    """Create a dummy dictionary with multiple levels."""
    testdict_deep = dict()
    for s in list("Test"):
        for i, v in enumerate(list("bar")):
            testdict_deep[s] = dict(v=i)
    return testdict_deep


# Tests -----------------------------------------------------------------------
def test_write_yml_single(testdict, tmpdir):
    """Testing if dummy write is succesful."""
    t.write_yml(testdict, os.path.join(tmpdir.dirpath(), "test_cfg_dummy.yml"))


def test_write_yml_double(testdict, tmpdir):
    """Writing two yml configs to see if the Exception is raised."""
    t.write_yml(testdict, os.path.join(tmpdir.dirpath(), "test_cfg.yml"))

    # calling the write function again should cause an error:
    with pytest.raises(FileExistsError):
        t.write_yml(testdict, tmpdir.dirpath())


def test_write_yml_deep(testdict_deep, tmpdir):
    """Writing a deep dummy dictionary with multiple levels."""
    t.write_yml(testdict_deep,
                os.path.join(tmpdir.dirpath(), "test_cfg_deep.yml"))


def test_read_yml(tmpdir):
    """Testing if previously written file can be read."""
    path = os.path.join(tmpdir.dirpath(), "test_cfg.yml")
    t.read_yml(path)


def test_read_yml_not_existing(tmpdir):
    """Testing if Exception is raised when no file is found."""
    path = os.path.join(tmpdir.dirpath(), "not_existent_config.yml")
    with pytest.raises(FileNotFoundError):
        t.read_yml(path)


def test_recursive_update(testdict):
    """Testing if recursive update actually works."""
    # creating 2 dummy dictionaries
    to_update_dict = testdict
    change_dict = copy.deepcopy(to_update_dict)

    # actually make changes to change_dict
    change_dict['T'] = 5

    with pytest.raises(RuntimeError):
        if change_dict != to_update_dict:
            raise RuntimeError("The dictionaries are obviously not equal.")

    to_update_dict = t.recursive_update(to_update_dict, change_dict)
    if to_update_dict != change_dict:
        raise RuntimeError("recursive_update doesn't work!")


def test_recursive_update_deep(testdict_deep):
    """Testing if deep recursive update actually works."""
    # creating 2 dummy dictionaries
    to_update_dict = testdict_deep
    change_dict = copy.deepcopy(to_update_dict)

    # actually make changes to change_dict
    change_dict['T']['b'] = 5

    with pytest.raises(RuntimeError):
        if change_dict != to_update_dict:
            raise RuntimeError("The dictionaries are obviously not equal.")

    to_update_dict = t.recursive_update(to_update_dict, change_dict)
    if to_update_dict != change_dict:
        raise RuntimeError("recursive_update doesn't work!")


def test_read_yml_with_error(tmpdir):
    """Testing if Exception is raised when no file is found with custom Error message."""
    path = os.path.join(tmpdir.dirpath(), "not_existent_config.yml")
    with pytest.raises(FileNotFoundError):
        t.read_yml(path, "My Custom error message.")
