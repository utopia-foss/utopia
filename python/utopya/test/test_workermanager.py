"""Tests the WorkerManager class"""

import os

import pytest

from utopya.workermanager import WorkerManager


# Fixtures --------------------------------------------------------------------
@pytest.fixture
def wm():
    """Create the simplest possible WorkerManager instance"""
    return WorkerManager(num_workers=1)

@pytest.fixture
def wm_with_tasks():
    """Create a WorkerManager instance and add some tasks"""
    env = os.environ.copy()

    # A few tasks
    tasks = []
    tasks.append((('python', '-c','print("hello oh so complex world")'),
                  dict(read_stdout=True)))
    tasks.append((('python', '-c','from time import sleep; sleep(0.5)'),
                  dict(read_stdout=False)))
    tasks.append(('python -c return',
                  dict(read_stdout=False, priority=0)))

    wm = WorkerManager(num_workers=1)

    # Pass the tasks
    for cmd, kwargs in tasks:
        wm.add_task(cmd, env=env, **kwargs)
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
    sleep_cmd = ('python', '-c','from time import sleep; sleep(0.5)')
    wm.add_task(sleep_cmd, priority=0, read_stdout=True)

    # With string argument and without reading stdout
    sleep_cmd = ('python -c return')
    wm.add_task(sleep_cmd, priority=0, read_stdout=False)

def test_start_working(wm_with_tasks):
    """Tests whether the start_working methods does what it should"""
    wm = wm_with_tasks
    wm.start_working()
    # This will be blocking

    # Check if workers have been added
    assert wm.workers

    # Get the process dicts and perform tests on it
    procs = [w['proc'] for w in wm.workers.values()]
    proc = procs[0]

    # Assert that the process has finished running
    assert proc.poll() is 0

@pytest.mark.skip("Not implemented yet.")
def test_read_stdout():
    pass