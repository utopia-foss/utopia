"""Tests the utopya.parameter module and the Parameter class"""

import io
from itertools import chain
from typing import Any, Tuple
from pkg_resources import resource_filename

import numpy as np
import ruamel.yaml
import pytest

import utopya
from utopya.parameter import (Parameter,
                              extract_validation_objects as _extract)
from utopya.yaml import yaml, load_yml


# -- Test resources -----------------------------------------------------------

PM_CFG_PATH = resource_filename('test', 'cfg/parameter.yml')


# -- Fixtures -----------------------------------------------------------------

@pytest.fixture
def valid_params() -> list:
    return [
        Parameter(default='foo',
                  name='some string', description='a test string',
                  is_any_of=('foo', 'bar', 'baz'), dtype=str),
        Parameter(default=3),
        Parameter.from_shorthand(0.3, mode='is-probability'),
        Parameter.from_shorthand(3, mode='is-positive'),
        Parameter.from_shorthand(-2, mode='is-negative'),
        Parameter.from_shorthand(3, mode='is-int'),
        Parameter.from_shorthand(True, mode='is-bool'),
        Parameter.from_shorthand(False, mode='is-bool'),
        Parameter.from_shorthand('foo', mode='is-string'),
        Parameter.from_shorthand(0, mode='is-unsigned')
    ]


# -- Tests --------------------------------------------------------------------

def test_Parameter_init(valid_params):
    """Tests the initialisation of the Parameter class."""
    # Successful initialisation
    # ... is tested via `valid_params` fixture

    # Check some specific scenarios
    # .. Minimal
    p = Parameter(default=123)
    assert p.default == 123
    assert p.name is None
    assert p.description is None
    assert p.limits is None
    assert p.limits_mode == '[]'
    assert p.is_any_of is None
    assert p.dtype is None

    # .. With limits
    p = Parameter(default=42, limits=[40, 42], limits_mode='(]', dtype=int,
                  name="fourtytwo", description="always the same")
    assert p.default == 42
    assert p.name == "fourtytwo"
    assert p.description == "always the same"
    assert p.limits == (40, 42)
    assert isinstance(p.limits, tuple)
    assert p.limits_mode == '(]'
    assert p.is_any_of is None
    assert p.dtype == np.dtype(int)

    # Check error messages
    with pytest.raises(TypeError, match="missing 1 required keyword-only"):
        Parameter()

    with pytest.raises(ValueError, match='Unrecognized `limits_mode` arg'):
        Parameter(default=3, limits=[0, 1], limits_mode=')(')

    with pytest.raises(ValueError, match='cannot be specified at the same'):
        Parameter(default=3, limits=[0, 1], is_any_of=[1, 2, 3])

    with pytest.raises(ValueError, match='Unrecognized shorthand mode \'inv'):
        Parameter.from_shorthand('foo', mode='invalid-tag')

    with pytest.raises(TypeError, match='Expected a tuple-like `limits` arg'):
        Parameter(default=None, limits="foo")

    with pytest.raises(ValueError, match='should be of length 2! Got: '):
        Parameter(default=None, limits=(1,2,3))

    with pytest.raises(TypeError, match='non-numerical default value \'foo\''):
        Parameter(default="foo", limits=(1,2))

def test_Parameter_valid_default(valid_params):
    """The default should always be valid"""
    for param in valid_params:
        assert param.validate(param.default)

def test_Parameter_magic_methods(valid_params):
    """Tests the magic methods of the Parameter"""
    for param in valid_params:
        # Always equal to itself, never equal to some other type or the
        # previous parameter
        assert param != "foo"
        assert param == param
        assert param != Parameter(default="so unique")
        assert all([param != p for p in valid_params if p is not param])

        # String representation is robust
        assert str(param).startswith("<Parameter")

    # Sufficient information contained in string representation
    p = Parameter(default=42, limits=[40, 43], dtype=int,
                  name="fourtytwo", description="always the same")
    assert "42" in str(p)
    assert "fourtytwo" in str(p)
    assert "is_any_of" not in str(p)
    assert "(40, 43), limits_mode: '[]'" in str(p)
    assert "int" in str(p)

def test_parameter_extraction():
    """Tests extraction and dumping of Parameter objects from and to a
    configuration
    """
    cfg = load_yml(PM_CFG_PATH)["to_extract"]
    model_cfg, params_to_validate = _extract(cfg, model_name='model')

    assert model_cfg, {} == _extract(model_cfg, model_name='model')
    assert len(params_to_validate) == 10  # NOTE When adjusting this, add the
                                          #      explicit case below!

    # Check explicitly
    assert (   params_to_validate[('model', 'param1', 'subparam1')]
            == Parameter(default=0.3, limits=[0, 2], dtype=float))

    assert (   params_to_validate[('model', 'param1', 'subparam3')]
            == Parameter(default=-3, limits=[None, 0],
                         dtype=int, limits_mode='()'))

    assert (   params_to_validate[('model', 'param1', 'subparam4')]
            == Parameter(default=42.2, limits=[0, None], limits_mode='(]'))

    assert (   params_to_validate[('model', 'param2',)]
            == Parameter.from_shorthand(0.5, mode='is-probability'))

    assert (   params_to_validate[('model', 'param3', 'subparam2y')]
            == Parameter.from_shorthand(True, mode='is-bool'))

    assert (   params_to_validate[('model', 'param3', 'subparam2n')]
            == Parameter.from_shorthand(False, mode='is-bool'))

    assert (   params_to_validate[('model', 'param3', 'subparam3')]
            == Parameter(default="baz", dtype=str,
                         is_any_of=['foo', 'bar', 'baz', 'bam']))

    assert (params_to_validate[('model', 'param4')]
            == Parameter.from_shorthand(2, mode='is-int'))

    assert (   params_to_validate[('model', 'param5')]
            == Parameter(default=0.4))

    assert (   params_to_validate[('model', 'param6')]
            == Parameter(default=None, dtype=str))

def test_yaml_roundtrip():
    """Tests loading and writing from and to yaml files."""
    def make_roundtrip(obj: Any) -> Any:
        s = io.StringIO("")
        yaml.dump(obj, stream=s)
        s = s.getvalue()
        return yaml.load(s)

    cfg = load_yml(PM_CFG_PATH)["to_extract"]
    model_cfg, params_to_validate = _extract(cfg, model_name='model')

    for param_key, param in params_to_validate.items():
        assert make_roundtrip(param) == param

        # Check magic methods once more
        assert param == param
        assert param != 'foo'
        assert str(param)

def test_Parameter_validation():
    """Tests validation of the Parameter class."""
    def get_match(element) -> Tuple[Any, str]:
        """If the given element is a sequence, assume the second value is
        an error match string
        """
        if isinstance(element, (tuple, list)):
            return element[0], element[1]
        return element, None

    cfg = load_yml(PM_CFG_PATH)

    for case_name, case_cfg in chain(cfg["to_assert"].items(),
                                     cfg["shorthands"].items()):
        print(f"Testing case '{case_name}' ...")
        param = case_cfg["construct_param"]

        if "expected_default" in case_cfg:
            assert param.default == case_cfg["expected_default"]

        for val in case_cfg["validate_true"]:
            assert param.validate(val)

        for val in case_cfg["validate_false"]:
            val, match = get_match(val)

            assert not param.validate(val, raise_exc=False)
            with pytest.raises(utopya.parameter.ValidationError, match=match):
                assert param.validate(val)
