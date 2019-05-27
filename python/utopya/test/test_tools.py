"""I'm a docstring."""
import os
import copy
from pkg_resources import resource_filename

import numpy as np
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
                nothing=None, more_nothing=None)


# Test YAML constructors ------------------------------------------------------

def test_expr_constr():
    """Tests the expression constructor"""
    tstr = """
        one:   !expr 1*2*3
        two:   !expr 9 / 2
        three: !expr 2**4
        four:  !expr 1e-10
        five:  !expr 1E10
        six:   !expr inf
        seven: !expr NaN
    """

    # Load the string using the tools module, where the constructor was added
    d = t.yaml.load(tstr)

    # Assert correctness
    assert d['one'] == 1 * 2 * 3
    assert d['two'] == 9 / 2
    assert d['three'] == 2**4
    assert d['four'] == eval('1e-10') == 10.0**(-10)
    assert d['five'] == eval('1E10') == 10.0**10
    assert d['six'] == np.inf
    assert np.isnan(d['seven'])


def test_model_cfg_constructor():
    """Tests the expression constructor"""
    tstr = """
        model: !model
          model_name: dummy
          foo: baz
          lvl: 0

        sub:
          model1: !model
            model_name: dummy
            spam: 2.34
            lvl: 1
            num: 1

          model2: !model
            model_name: dummy
            lvl: 1
            num: 2
    """
    # TODO once there are more models, add nesting here

    # Load the string using the tools module, where the constructor was added
    d = t.yaml.load(tstr)

    # Assert correctness
    assert d['model'] == dict(foo="baz", spam=1.23, lvl=0)
    assert d['sub']['model1'] == dict(foo="bar", spam=2.34, lvl=1, num=1)
    assert d['sub']['model2'] == dict(foo="bar", spam=1.23, lvl=1, num=2)

    # It should fail without a model name
    with pytest.raises(KeyError, match="model_name"):
        t.yaml.load("model: !model {}")
    
    # ... or with an invalid model name
    with pytest.raises(ValueError, match="No 'invalid' model"):
        t.yaml.load("model: !model {model_name: invalid}")


# Function tests --------------------------------------------------------------

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


def test_load_model_cfg():
    """Tests the loading of model configurations by name"""
    # Load the dummy configuration, for testing
    mcfg, path = t.load_model_cfg('dummy')

    # Assert correct types
    assert isinstance(mcfg, dict)
    assert isinstance(path, str)

    # Assert expected content
    assert mcfg['foo'] == 'bar'
    assert mcfg['spam'] == 1.23
    assert os.path.isfile(path)

    # Test with invalid name
    with pytest.raises(ValueError, match="No 'invalid' model available!"):
        t.load_model_cfg('invalid')


def test_recursive_update(testdict):
    """Testing if recursive update works as desired."""
    d = testdict
    u = copy.deepcopy(d)

    # Make some changes
    u['more_entries'] = dict(a=1, b=2)
    u['foo'] = "changed_bar"
    u['bar'] = 654.321
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
    assert d['baz'] == dict(another_entry="hello", foo="more_changed_bars",
                            bar=456.123, baz=[1,2,dict(three=3)],
                            nothing=dict(some="thing"))
    assert d['nothing'] == dict(some="thing")
    assert d['more_nothing'] == "something"

def test_format_time():
    """Test the time formatting method"""
    assert t.format_time(10) == "10s"
    assert t.format_time(10.1) == "10s"
    assert t.format_time(10.1, ms_precision=2) == "10.10s"
    assert t.format_time(0.1) == "< 1s"
    assert t.format_time(0.1, ms_precision=2) == "0.10s"
    assert t.format_time(123) == "2m 3s"
    assert t.format_time(123, ms_precision=2) == "2m 3s"
    assert t.format_time(60*60*24 + 60*60*2 + 60*3 + 4) == "1d 2h 3m 4s"
    assert t.format_time(60*60*24 + 60*60*2 + 60*3 + 4,
                         max_num_parts=3) == "1d 2h 3m"
    assert t.format_time(60*60*24 + 60*60*2 + 60*3 + 4,
                         max_num_parts=2) == "1d 2h"

def test_fill_line():
    """Tests the fill_line and center_in_line methods"""
    # Shortcut for setting number of columns
    fill = lambda *args, **kwargs: t.fill_line(*args, num_cols=10, **kwargs)

    # Check that the expected number of characters are filled at the right spot
    assert fill("foo") == "foo" + 7*" "
    assert fill("foo", fill_char="-") == "foo" + 7*"-"
    assert fill("foo", align='r') == 7*" " + "foo"
    assert fill("foo", align='c') == "   foo    "
    assert fill("foob", align='c') == "   foob   "

    with pytest.raises(ValueError, match="length 1"):
        fill("foo", fill_char="---")

    with pytest.raises(ValueError, match="align argument 'bar' not supported"):
        fill("foo", align='bar')

    # The center_in_line method has a fill_char given and adds a spacing
    assert t.center_in_line("foob", num_cols=10) == "·· foob ··"  # cdot!
    assert t.center_in_line("foob", num_cols=10, spacing=2) == "·  foob  ·"
