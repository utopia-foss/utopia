"""Tests the WorkerManager class"""

import os
import copy

import pytest

from utopya.workermanager import WorkerManager, enqueue_lines, parse_json, WorkerManagerTotalTimeout


# Fixtures --------------------------------------------------------------------
@pytest.fixture
def wm():
    """Create the simplest possible WorkerManager instance"""
    return WorkerManager(num_workers=2)

@pytest.fixture
def sleep_task() -> dict:
    """Returns a dict that can be used to call add_task and just sleeps 0.5 seconds"""
    env = os.environ.copy()
    return dict(worker_kwargs=dict(args=('python3', '-c',
                                         'from time import sleep; sleep(0.5)'),
                                   read_stdout=False, env=env))

@pytest.fixture
def wm_with_tasks(sleep_task):
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
    
    tasks.append(sleep_task)
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

def test_init():
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

def test_add_tasks(wm, sleep_task):
    """Tests adding of tasks"""
    # This one should work
    wm.add_task(**sleep_task)

    # Check the warnings
    with pytest.warns(UserWarning):
        wm.add_task(setup_func=lambda: "foo", **sleep_task)
    
    with pytest.warns(UserWarning):
        wm.add_task(setup_kwargs=dict(foo="bar"), **sleep_task)

    # And the errors
    with pytest.raises(ValueError):
        wm.add_task()

def test_spawn_worker(wm):
    """Tests the spawning of a worker"""
    # Try to spawn from non-tuple `args` argument
    with pytest.raises(TypeError):
        wm._spawn_worker(args="echo $?", read_stdout=False)

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

def test_signal_workers(wm, sleep_task):
    """Tests the signalling of workers"""
    for _ in range(3):
        wm.add_task(**sleep_task)

    # Start running with a post-poll function that directly kills off the workers
    ppf = lambda: wm._signal_workers(workers=wm.working, signal='SIGKILL')
    wm.start_working(post_poll_func=ppf)

    # Same thing with an integer signal
    for _ in range(3):
        wm.add_task(**sleep_task)
    ppf = lambda: wm._signal_workers(workers=wm.working, signal=9)
    wm.start_working(post_poll_func=ppf)

    # And invalid signalling value
    with pytest.raises(ValueError):
        wm._signal_workers(workers=None, signal=3.14)

def test_timeout(wm, sleep_task):
    """Tests whether the timeout succeeds"""
    # Add some sleep tasks
    for _ in range(3):
        wm.add_task(**sleep_task)
    # NOTE This should take 1 second to execute

    # Check if the run does not start for an invalid timeout value
    with pytest.raises(ValueError):
        wm.start_working(timeout=-123.45)

    # Check if no WorkerManagerTotalTimeout is raised for a high timeout value
    wm.start_working(timeout=2.)

    # Add more asks
    for _ in range(3):
        wm.add_task(**sleep_task)

    # Test if one is raised for a smaller timeout value
    with pytest.raises(WorkerManagerTotalTimeout):
        wm.start_working(timeout=0.7)


@pytest.mark.skip("Properly implement this!")
def test_read_stdout(wm):
    """Checks if the stdout was read"""

    wm.start_working()

    # TODO read the stream output here
