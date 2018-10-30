"""Test the stopcond module"""

import io
import time
import subprocess

import pytest
import ruamel.yaml

from utopya.tools import yaml
import utopya.stopcond as sc
import utopya.stopcond_funcs as sc_funcs
from utopya.task import WorkerTask

# Fixtures --------------------------------------------------------------------
@pytest.fixture
def basic_sc():
    """Returns a basic StopCondition object that checks the mock timeout_wall method."""
    return sc.StopCondition(to_check=[dict(func=sc_funcs.timeout_wall,
                                           seconds=123)],
                            name="wall timeout")

@pytest.fixture
def task() -> WorkerTask:
    """Generates a WorkerTask object, manipulating it somewhat to act as a
    mock object.
    """
    task = WorkerTask(name="foo", worker_kwargs=dict(args=('echo','$?')))

    task.profiling['create_time'] = time.time() - 124.
    task.streams['out'] = dict(log_parsed=[])

    return task

# Tests of the class ----------------------------------------------------------

def test_init():
    """Test StopCondition initialization"""
    sc.StopCondition(to_check=[dict(func=sc_funcs.timeout_wall, seconds=123)])

    # Empty should also work
    sc.StopCondition(to_check=[])
    
    # These should fail
    # no to_check argument
    with pytest.raises(TypeError, match="Need at least one of the required"):
        sc.StopCondition()
    
    # invalid function name
    with pytest.raises(ImportError, match="Could not find a callable named"):
        sc.StopCondition(to_check=[dict(func="I am not a function.")])
    
    # non-callable
    with pytest.raises(TypeError, match="Given `func` needs to be a callable"):
        sc.StopCondition(to_check=[dict(func=123.456)])

    # too many arguments
    with pytest.raises(ValueError, match="Please pass either"):
        sc.StopCondition(to_check=[dict(func=sc_funcs.timeout_wall,
                                        seconds=123)],
                         func=sc_funcs.timeout_wall)

def test_constructor():
    """Tests the YAML constructor"""
    ymlstr1 = "sc: !stop-condition {to_check: [], name: foo, description: bar}"
    assert isinstance(yaml.load(ymlstr1)['sc'], sc.StopCondition)

    ymlstr2 = "sc: !stop-condition [1, 2, 3]"
    with pytest.raises(ruamel.yaml.constructor.ConstructorError):
        yaml.load(ymlstr2)

def test_representer():
    """Tests the YAML constructor"""
    sc1 = sc.StopCondition(func="timeout_wall", seconds=123)

    with io.StringIO() as f:
        yaml.dump(sc1, stream=f)
        print(f.getvalue())
        assert f.getvalue() == ("!stop-condition {func: timeout_wall, "
                                "seconds: 123}\n")

def test_magic_methods(basic_sc):
    """Tests magic methods of the StopCond class."""
    str(basic_sc)

def test_fulfilled(basic_sc, task):
    """Tests magic methods of the StopCond class."""
    # Test if it is fulfilled (should always be the case when using the fixtures)
    assert basic_sc.fulfilled(task) is True

    # Disable it: should now be false
    basic_sc.enabled = False
    assert basic_sc.fulfilled(task) is False


# Tests of the stop condition methods -----------------------------------------

def test_check_monitor_entry(task):
    """Test the check_monitor_entry stop condition function"""
    check_monitor_entry = sc_funcs.check_monitor_entry

    # Without a parsed object, this is always false, and no other checks are
    # actually performed
    assert not check_monitor_entry(task, entry_name="foo",
                                   operator="bar", value="baz")

    # Add a mock object
    task.outstream_objs.append(dict(foo="bar"))
    assert check_monitor_entry(task, entry_name="foo",
                               operator="==", value="bar")
