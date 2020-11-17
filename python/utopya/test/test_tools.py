"""I'm a docstring."""
import os
import copy
from pkg_resources import resource_filename

import numpy as np
import pytest

import utopya.tools as t

# Local constants


# Fixtures --------------------------------------------------------------------
@pytest.fixture
def testdict():
    """Create a dummy dictionary."""
    return dict(foo="bar", bar=123.456,
                baz=dict(foo="more_bar", bar=456.123, baz=[1,2,dict(three=3)]),
                nothing=None, more_nothing=None)


# Test YAML constructors ------------------------------------------------------
# NOTE These are actually defined in the yaml module ... but available in tools

def test_expr_constructor():
    """Tests the expression constructor"""
    tstr = """
        one:   !expr 1*2*3
        two:   !expr 9 / 2
        three: !expr 2**4
        four:  !expr 1e-10
        five:  !expr 1E10
        six:   !expr inf
        seven: !expr NaN
        eight: !expr (2 + 3) * 4
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
    assert d['eight'] == (2 + 3) * 4


def test_any_and_all_constructor():
    """Tests the !any and !all yaml tags"""
    tstr = """
        any0: !any [false, false, 0]
        any1: !any [true, false, false]
        any2: !any [foo, bar, baz]
        all0: !all [false, 0, true]
        all1: !all [true, true, 1]
        all2: !all [foo, bar, false]
    """
    d = t.yaml.load(tstr)

    assert not d['any0']
    assert d['any1']
    assert d['any2']

    assert not d['all0']
    assert d['all1']
    assert not d['all2']


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
    with pytest.raises(KeyError, match="No model with name 'invalid' found"):
        t.yaml.load("model: !model {model_name: invalid}")


# Function tests --------------------------------------------------------------

def test_add_item():
    """Tests the add_item method"""
    # Basic case
    d = dict()
    t.add_item(123, add_to=d, key_path=["foo", "bar"])
    assert d['foo']['bar'] == 123

    # With function call
    d = dict()
    t.add_item(123, add_to=d, key_path=["foo", "bar"],
               value_func=lambda v: v**2)
    assert d['foo']['bar'] == 123**2

    # Invalid value
    with pytest.raises(ValueError, match="My custom error message with -123"):
        t.add_item(-123, add_to=d, key_path=["foo", "bar"],
                   is_valid=lambda v: v>0,
                   ErrorMsg=lambda v: ValueError("My custom error message "
                                                 "with {}".format(v)))

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
    assert t.format_time(59.127, ms_precision=2) == "59.13s"
    assert t.format_time(60.127, ms_precision=2) == "1m"
    assert t.format_time(61.127, ms_precision=2) == "1m 1s"
    assert t.format_time(123) == "2m 3s"
    assert t.format_time(123, ms_precision=2) == "2m 3s"
    assert t.format_time(60*60*24 + 60*60*2 + 60*3 + 4 + .5) == "1d 2h 3m 4s"
    assert t.format_time(60*60*24 + 60*60*2 + 60*3 + 4) == "1d 2h 3m 4s"
    assert t.format_time(60*60*24 + 60*60*2 + 60*3 + 0) == "1d 2h 3m"
    assert t.format_time(60*60*24 + 60*60*2 + 60*0 + 4) == "1d 2h 4s"
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

def test_parse_si_multiplier():
    """Tests the parse_si_multiplier function"""
    parse_si = t.parse_si_multiplier

    assert parse_si("0") == 0
    assert parse_si("1") == 1
    assert parse_si("123") == 123
    assert parse_si("123k") == 123 * 1000
    assert parse_si("123 k") == 123 * 1000
    assert parse_si("    123 k  ") == 123 * 1000

    assert parse_si("1.23M") == 1230000
    assert parse_si("1.23G") == 1230000000
    assert parse_si("1.23T") == 1230000000000

    # negative values
    assert parse_si("-0") == 0
    assert parse_si("-1") == -1
    assert parse_si("-1k") == -1000
    assert parse_si("- 1k") == -1000
    assert parse_si("- 1 k") == -1000
    assert parse_si("-1.23k") == -1230
    assert parse_si("-1.23456k") == int(-1234.56) == -1234

    # integer rounding
    assert parse_si("0.000001k") == 0
    assert parse_si("1.234567k") == int(1234.567)

    # errors
    for s in ("1a", "1.23b", "--123k", "-1.23kk", "3M5"):
        with pytest.raises(ValueError, match="Cannot parse"):
            parse_si(s)

def test_parse_num_steps():
    """Tests the parse_num_steps function"""
    parse_N = t.parse_num_steps

    assert parse_N(1) == 1
    assert parse_N(100) == 100
    assert parse_N("100") == 100
    assert parse_N("100k") == 100000
    assert parse_N("1.23k") == 1230

    # can _additionally_ be in scientific notation
    assert parse_N("1.23e3") == 1230
    assert parse_N("1.23e+5") == 123000

    # errors
    for arg in (-1, "-1", "-123k", "-1.23e+5"):
        with pytest.raises(ValueError, match="needs to be non-negative"):
            parse_N(arg)

        # ... unless the error is suppressed
        parse_N(arg, raise_if_negative=False)
