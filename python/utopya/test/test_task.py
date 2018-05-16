"""Test the Task class implementation"""

import queue
import io
from time import sleep

import numpy as np
import pytest

from utopya.task import Task, WorkerTask, TaskList, enqueue_lines, enqueue_json


# Fixtures ----------------------------------------------------------------

@pytest.fixture
def tasks() -> TaskList:
    """Returns a TaskList filled with tasks"""
    tasks = TaskList()
    for name, priority in enumerate(np.random.random(size=50)):
        tasks.append(Task(name=name, priority=priority))

    return tasks

@pytest.fixture
def workertasks() -> TaskList:
    """Returns a TaskList filled with WorkerTasks"""
    tasks = TaskList()
    for name, priority in enumerate(np.random.random(size=50)):
        tasks.append(WorkerTask(name=name, priority=priority,
                                worker_kwargs=dict(args=('echo', '$?'))))

    return tasks

# Task tests ------------------------------------------------------------------

def test_task_init():
    """Test task initialization"""
    Task() # will get a UID as name
    Task(name=0)
    Task(name=1, priority=1000)
    Task(name=2, priority=1000.01)

    # Test that the task name cannot be changed
    with pytest.raises(AttributeError):
        t = Task(name="foo")
        t.name = "bar"

def test_task_sorting(tasks):
    """Tests whether different task objects are sortable"""
    # Put into a normal list, as the TaskList does not support sorting
    tasks = list(tasks)

    # Sort it, then do some checks
    tasks.sort()

    t1 = tasks[0]
    t2 = tasks[1]

    assert (t1 <= t2) is True
    assert (t1 == t2) is False
    assert (t1 == t1) is True

def test_task_properties(tasks):
    """Test properties and magic methods"""
    _ = [str(t) for t in tasks]

    # Check if the name is given
    task = Task(name="foo")
    assert task.name == task._name == "foo"

    # Check if the UID is returned if no name was given
    task = Task()
    assert task.name == str(task.uid)

    # Check if the default priority is given
    assert task.priority == np.inf

# WorkerTask tests ------------------------------------------------------------

def test_workertask_init():
    """Tests the WorkerTask class"""
    WorkerTask(name=0, worker_kwargs=dict(foo="bar"))
    
    with pytest.warns(UserWarning, match="`worker_kwargs` given but also"):
        WorkerTask(name=0, setup_kwargs=dict(foo="bar"),
                   worker_kwargs=dict(foo="bar"))

    with pytest.raises(ValueError, match="Need either argument `setup_func`"):
        WorkerTask(name=0)

def test_workertask_magic_methods(workertasks):
    """Test magic methods"""
    _ = [str(t) for t in workertasks]

def test_workertask_invalid_args():
    """Test for invalid argument to the WorkerTask"""
    # It should not be possible to spawn a worker with non-tuple arguments
    t = WorkerTask(name=0, worker_kwargs=dict(args="python -c 'hello hello'"))
    
    with pytest.raises(TypeError):
        t.spawn_worker()

    # Wanting to save streams but not giving a path should raise an error
    t = WorkerTask(name=1, worker_kwargs=dict(args=("python", "--version"),
                                              save_streams=True))

    with pytest.raises(ValueError, match="Was told to `save_streams` but"):
        t.spawn_worker()

def test_workertask_streams(tmpdir):
    """Tests the read_ and save_streams methods of the WorkerTask"""
    save_path = tmpdir.join("out.log")

    t = WorkerTask(name="stream_test", 
                   worker_kwargs=dict(args=("echo", "foo\nbar\nbaz"),
                                            read_stdout=True,
                                            line_read_func=enqueue_json,
                                            save_streams=True,
                                            save_streams_to=str(save_path)))

    # There are no streams yet, so trying to save streams now should not
    # generate a file at save_path
    t.save_streams()
    assert not save_path.isfile()

    # Now spawn the worker
    t.spawn_worker()
    
    # Wait until done
    while t.worker.poll() is None:
        # Delay the while loop
        sleep(0.05)

    # Read a single line
    t.read_streams()
    assert len(t.streams['out']['log']) == 1

    # Read all the remaining stream content
    t.read_streams(max_num_reads=-1)
    assert len(t.streams['out']['log']) == 3

    # Save it
    t.save_streams()
    assert save_path.isfile()

    # Check the content
    with open(save_path) as f:
        lines = [line.strip() for line in f]

    assert len(lines) == 5
    assert lines[0].startswith("Log of 'out' stream of WorkerTask")
    assert lines[2:] == ["foo", "bar", "baz"]

    # Trying to save the streams again, there should be no more lines available
    t.save_streams()

# TaskList tests --------------------------------------------------------------

def test_tasklist(tasks):
    """Tests the TaskList features"""
    # Add some tasks
    for _ in range(3):
        tasks.append(Task(name=len(tasks)))

    # This should not work: the tasks already exist
    with pytest.raises(ValueError):
        tasks.append(tasks[0])

    # Or it was not even a task
    with pytest.raises(TypeError):
        tasks.append(("foo", "bar"))

    # Check the __contains__ method
    assert ("foo",) not in tasks
    assert all([task in tasks for task in tasks])

# Additional functions Test from task.py tests --------------------------------

def test_enque_lines_unicode():
    # only testing the inner block of the function, the error is raised later from put_noawait()
    # valid unicode utf8
    with pytest.raises(TypeError):
        enqueue_lines(queue=queue.Queue, stream=io.BytesIO(bytes('hello', 'utf-8')))
    # non valid unicode utf8
    with pytest.raises(TypeError):
        enqueue_lines(queue=queue.Queue, stream=io.BytesIO(bytes('hello', 'utf-32')))

def test_enque_json():
    # only testing the wrapper, the error is raised later from put_noawait() in enqueue_lines
    with pytest.raises(TypeError):
        enqueue_json(queue=queue.Queue, stream=io.BytesIO(bytes('hello', 'utf-8')))
