"""Tests the WorkerManager class"""

import os

import pytest

from utopya.workermanager import WorkerManager, enqueue_lines, parse_json


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
                                               'print("hello oh so complex '
                                               'world")'),
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

    # Test different `poll_frequencies` arguments
    with pytest.raises(ValueError):
        # negative
        WorkerManager(num_workers=1, poll_freq=-1)

def test_add_tasks(wm):
    """Tests adding of tasks"""
    # With tuple argument
    sleep_cmd = ('python3', '-c', '"from time import sleep; sleep(0.5)"')
    wm.add_task(priority=0, worker_kwargs=dict(args=sleep_cmd,
                                               read_stdout=True))

    # TODO Check the warnings

def test_start_working(wm_with_tasks):
    """Tests whether the start_working methods does what it should"""
    wm = wm_with_tasks
    wm.start_working()
    # This will be blocking

    # Check if workers have been added
    assert wm.workers

    # Get the process dicts and check that they all finished with exit code 0
    procs = [w['proc'] for w in wm.workers.values()]
    assert all([p.poll() is 0 for p in procs])
    assert not any([w['status'] for w in wm.workers.values()])

    # Check their run times
    create_times = [w['create_time'] for w in wm.workers.values()]
    end_times = [w['end_time'] for w in wm.workers.values()]
    assert all([c < e for c, e in zip(create_times, end_times)])
    assert (end_times[1] - create_times[1]) > 0.5 # for the sleep task

def test_read_stdout(wm):
    """Checks if the stdout was read"""

    wm.start_working()

    # TODO read the stream output here
