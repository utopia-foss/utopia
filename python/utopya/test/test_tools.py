"""I'm a docstring."""
import os
import copy
from pkg_resources import resource_filename

import pytest

import utopya.tools as t

# Local constants
READ_TEST_PATH = resource_filename('test', 'cfg/read_test.yml')
CONSTR_TEST_PATH = resource_filename('test', 'cfg/constr_test.yml')

# Fixtures --------------------------------------------------------------------
@pytest.fixture
def testdict():
    """Create a dummy dictionary."""
    return dict(foo="bar", bar=123.456,
                baz=dict(foo="more_bar", bar=456.123, baz=[1,2,dict(three=3)]),
                a_tuple=(1,2,3),
                nothing=None, more_nothing=None)


# Tests -----------------------------------------------------------------------

def test_read_yml(tmpdir):
    """Testing if previously written file can be read."""
    # Read the test file
    d = t.read_yml(READ_TEST_PATH)

    # An Exception should be raised when no file is found
    bad_path = tmpdir.join("not_existent_config.yml")
    with pytest.raises(FileNotFoundError):
        t.read_yml(bad_path)

    with pytest.raises(FileNotFoundError, match="My Custom error message."):
        t.read_yml(bad_path, error_msg="My Custom error message.")

    # Assert that the read content is correct. Content of READ_TEST_PATH:
    # foo: bar
    # baz: 123.45
    # a_list: [1,2,3]
    # a_map:
    #   another_map:
    #     a: 1
    #     b: 2
    #     c: 3
    #   yet_another_map:
    #     foo: bar
    #     baz: 123
    #     a_list: [1,2,3]

    assert d['foo'] == 'bar'
    assert d['baz'] == 123.45
    assert d['a_list'] == [1,2,3]
    assert d['a_map']['another_map'] == dict(a=1, b=2, c=3)
    assert d['a_map']['yet_another_map'] == dict(foo="bar", baz=123,
                                                 a_list=[1,2,3])

    # Also read files that include custom constructors
    t.read_yml(CONSTR_TEST_PATH)


def test_write_yml(testdict, tmpdir):
    """Testing if writing dicts to yaml is succesful."""
    # Write the test dictionary
    path = tmpdir.join("test.yml")
    t.write_yml(testdict, path=path)

    # Writing again should fail
    with pytest.raises(FileExistsError):
        t.write_yml(testdict, path=tmpdir.join("test.yml"))

    # Read in the file and assert equality between written and read file
    assert testdict == t.read_yml(path)


def test_recursive_update(testdict):
    """Testing if recursive update works as desired."""
    d = testdict
    u = copy.deepcopy(d)

    # Make some changes
    u['more_entries'] = dict(a=1, b=2)
    u['foo'] = "changed_bar"
    u['bar'] = 654.321
    del u['a_tuple']
    u['baz'] = dict(another_entry="hello", foo="more_changed_bars",
                    nothing=dict(some="thing"))
    u['nothing'] = dict(some="thing")
    u['more_nothing'] = "something"

    assert d != u

    # Perform the update
    d = t.recursive_update(d, u)
    assert d['more_entries'] == dict(a=1, b=2)
    assert d['foo'] == "changed_bar"
    assert d['bar'] == 654.321
    assert d['a_tuple'] == (1,2,3)
    assert d['baz'] == dict(another_entry="hello", foo="more_changed_bars",
                            bar=456.123, baz=[1,2,dict(three=3)],
                            nothing=dict(some="thing"))
    assert d['nothing'] == dict(some="thing")
    assert d['more_nothing'] == "something"
