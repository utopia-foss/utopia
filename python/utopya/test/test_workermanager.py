"""Tests the WorkerManager class"""

import os

import pytest

from utopya.workermanager import WorkerManager
from utopya.task import enqueue_lines, parse_json


# Fixtures --------------------------------------------------------------------
@pytest.fixture
def wm():
    """Create the simplest possible WorkerManager instance"""
    return WorkerManager(num_workers=1)

@pytest.fixture
def wm_with_tasks():
    """Create a WorkerManager instance and add some tasks"""
    # Copy over the environment
    env = os.environ.copy()

    # Set a json-reading function
    line_read_json = lambda queue, stream: enqueue_lines(queue=queue,
                                                         stream=stream,
                                                         parse_func=parse_json)

    # A few tasks
    tasks = []
    tasks.append(dict(worker_kwargs=dict(args=('python3', '-c',
                                               'print("hello\\noh so\\n'
                                               'complex world")'),
                                         read_stdout=True, env=env),
                      priority=0))
    
    tasks.append(dict(worker_kwargs=dict(args=('python3', '-c',
                                               'from time import sleep; '
                                               'sleep(0.5)'),
                                         read_stdout=False, env=env)))
    tasks.append(dict(worker_kwargs=dict(args=('python3', '-c', 'pass'),
                                         read_stdout=False, env=env)))
    tasks.append(dict(worker_kwargs=dict(args=('python3', '-c',
                                               'print("{\'key\': \'1.23\'}")'),
                                         read_stdout=True, env=env,
                                         line_read_func=line_read_json)))

    # Now initialise the worker manager
    wm = WorkerManager(num_workers=3)

    # And pass the tasks
    for task_dict in tasks:
        wm.add_task(**task_dict)

    return wm


# Tests -----------------------------------------------------------------------

def test_init(wm):
    """Tests whether initialisation succeeds"""
    # Test different `num_workers` arguments
    WorkerManager(num_workers='auto')
    WorkerManager(num_workers=-1)
    WorkerManager(num_workers=1)

    with pytest.warns(UserWarning):
        # too many workers
        WorkerManager(num_workers=1000)

    with pytest.raises(ValueError):
        # Negative
        WorkerManager(num_workers=-1000)
    
    with pytest.raises(ValueError):
        # not int
        WorkerManager(num_workers=1.23)

    # Test different `poll_delay` arguments
    with pytest.raises(ValueError):
        # negative
        WorkerManager(num_workers=1, poll_delay=-1000)
    
    with pytest.warns(UserWarning):
        # small value
        WorkerManager(num_workers=1, poll_delay=0.001)

def test_add_tasks(wm):
    """Tests adding of tasks"""
    # With tuple argument
    sleep_cmd = ('python3', '-c', '"from time import sleep; sleep(0.5)"')
    wm.add_task(priority=0, worker_kwargs=dict(args=sleep_cmd,
                                               read_stdout=True))

    # Test that errors propagate through
    with pytest.warns(UserWarning):
        wm.add_task(setup_func=print, worker_kwargs=dict(foo="bar"))
    
    with pytest.warns(UserWarning):
        wm.add_task(setup_kwargs=dict(foo="bar"),
                    worker_kwargs=dict(foo="bar"))

    with pytest.raises(ValueError):
        wm.add_task()

def test_start_working(wm_with_tasks):
    """Tests whether the start_working methods does what it should"""
    wm = wm_with_tasks
    wm.start_working(forward_streams=True)
    # This will be blocking

    # Check that all tasks finished with exit status 0
    for task in wm.tasks:
        assert task.worker_status == 0
        assert task.profiling['end_time'] > task.profiling['create_time']

        # Trying to spawn or assigning another worker should fail
        with pytest.raises(RuntimeError):
            task.spawn_worker()

        with pytest.raises(RuntimeError):
            task.worker = "something"


def test_read_stdout(wm):
    """Checks if the stdout was read"""

    wm.start_working()

    # TODO read the stream output here
