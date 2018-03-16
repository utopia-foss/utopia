"""Test the stopcond module"""

import yaml
import time
import subprocess

import pytest

import utopya.stopcond as sc
import utopya.stopcond_funcs as sc_funcs

# Fixtures --------------------------------------------------------------------
@pytest.fixture
def basic_sc():
    """Returns a basic StopCondition object that checks the mock timeout_wall method."""
    return sc.StopCondition(to_check=[dict(func=sc_funcs.timeout_wall,
                                           seconds=123)],
                            name="wall timeout")

@pytest.fixture
def proc():
    """A mock process object"""
    return subprocess.Popen(args=('echo', '$?'))

@pytest.fixture
def worker_info():
    """A mock worker_info dictionary that suggests that there was a timeout when tested against the `timeout_wall` method"""
    return dict(create_time=time.time() - 124.)

# Tests of the class ----------------------------------------------------------

def test_init():
    """Test StopCondition initialization"""
    sc.StopCondition(to_check=[])
    
    # These should fail
    # no to_check argument
    with pytest.raises(TypeError):
        sc.StopCondition()
    
    # invalid function name
    with pytest.raises(TypeError):
        sc.StopCondition(to_check=[dict(func="I am not a function.")])

def test_constructors():
    """Tests the YAML constructor"""
    yaml.add_constructor(u'!stop-condition', sc.stop_cond_constructor)
    yaml.add_constructor(u'!sc-func', sc.sc_func_constructor)

    ymlstr1 = "sc: !stop-condition {to_check: [], name: foo, description: bar}"
    assert isinstance(yaml.load(ymlstr1)['sc'], sc.StopCondition)

    ymlstr2 = "sc: !stop-condition [1, 2, 3]"
    with pytest.raises(TypeError):
        yaml.load(ymlstr2)

    ymlstr3 = "scf: !sc-func timeout_wall"
    assert yaml.load(ymlstr3)['scf'] is sc_funcs.timeout_wall

    # non-scalar node type
    ymlstr4 = "scf: !sc-func [timeout_wall, foo, bar]"
    with pytest.raises(TypeError):
        yaml.load(ymlstr4)
    
    # non-scalar node type
    ymlstr5 = "scf: !sc-func non_existant_function"
    with pytest.raises(ImportError):
        yaml.load(ymlstr5)
        

def test_magic_methods(basic_sc):
    """Tests magic methods of the StopCond class."""
    str(basic_sc)

def test_fulfilled(basic_sc, proc, worker_info):
    """Tests magic methods of the StopCond class."""
    # Test if it is fulfilled (should always be the case when using the fixtures)
    assert basic_sc.fulfilled(proc, worker_info=worker_info) is True

    # Disable it: should now be false
    basic_sc.enabled = False
    assert basic_sc.fulfilled(proc, worker_info=worker_info) is False

# Tests of the stop condition methods -----------------------------------------
