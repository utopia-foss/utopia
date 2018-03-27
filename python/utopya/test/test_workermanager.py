"""Tests the WorkerManager class"""

import os
import pkg_resources

import pytest
import random as rd
import numpy as np

from utopya.workermanager import WorkerManager, WorkerManagerTotalTimeout
from utopya.task import enqueue_lines, parse_json
from utopya.tools import read_yml

# Some constants
STOP_CONDS_PATH = pkg_resources.resource_filename('test', 'cfg/stop_conds.yml')

# Fixtures --------------------------------------------------------------------
@pytest.fixture
def wm():
    """Create the simplest possible WorkerManager instance"""
    return WorkerManager(num_workers=2, poll_delay=0.042)

@pytest.fixture
def sleep_task() -> dict:
    """Returns a dict that can be used to call add_task and just sleeps 0.5 seconds"""
    return dict(worker_kwargs=dict(args=('python3', '-c',
                                         'from time import sleep; sleep(0.5)'),
                                   read_stdout=True))

@pytest.fixture
def longer_sleep_task() -> dict:
    """Returns a dict that can be used to call add_task and just sleeps 0.5 seconds"""
    return dict(worker_kwargs=dict(args=('python3', '-c',
                                         'from time import sleep; sleep(1.0)'),
                                   read_stdout=True))

@pytest.fixture
def wm_with_tasks(sleep_task):
    """Create a WorkerManager instance and add some tasks"""
    # Set a json-reading function
    line_read_json = lambda queue, stream: enqueue_lines(queue=queue,
                                                         stream=stream,
                                                         parse_func=parse_json)

    # A few tasks
    tasks = []
    tasks.append(dict(worker_kwargs=dict(args=('python3', '-c',
                                               'print("hello\\noh so\\n'
                                               'complex world")'),
                                         read_stdout=True),
                      priority=0))
    
    tasks.append(sleep_task)
    tasks.append(dict(worker_kwargs=dict(args=('python3', '-c', 'pass'),
                                         read_stdout=False)))
    tasks.append(dict(worker_kwargs=dict(args=('python3', '-c',
                                               'print("{\'key\': \'1.23\'}")'),
                                         read_stdout=True,
                                         line_read_func=line_read_json)))
    tasks.append(sleep_task)

    # Now initialise the worker manager
    wm = WorkerManager(num_workers=2)

    # And pass the tasks
    for task_dict in tasks:
        wm.add_task(**task_dict)

    return wm

@pytest.fixture
def sc_run_kws():
    return read_yml(STOP_CONDS_PATH)['run_kwargs']

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

    # Test different `poll_delay` arguments
    with pytest.raises(ValueError):
        # negative
        WorkerManager(num_workers=1, poll_delay=-1000)
    
    with pytest.warns(UserWarning):
        # small value
        WorkerManager(num_workers=1, poll_delay=0.001)

def test_add_tasks(wm, sleep_task):
    """Tests adding of tasks"""
    # This one should work
    wm.add_task(**sleep_task)

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

def test_signal_workers(wm, sleep_task):
    """Tests the signalling of workers"""
    # Start running with a post-poll function that directly kills off the workers
    for _ in range(3):
        wm.add_task(**sleep_task)
    ppf = lambda: wm._signal_workers('active', signal='SIGKILL')
    wm.start_working(post_poll_func=ppf)

    # Same thing with an integer signal
    for _ in range(3):
        wm.add_task(**sleep_task)
    ppf = lambda: wm._signal_workers('active', signal=9)
    wm.start_working(post_poll_func=ppf)

    # Check if they were all killed
    for task in wm.tasks:
        assert task.worker_status == -9

    # And invalid signalling value
    wm.add_task(**sleep_task)
    ppf = lambda: wm._signal_workers('active', signal=3.14)
    with pytest.raises(ValueError):
        wm.start_working(post_poll_func=ppf)

    # Invalid list to signal
    with pytest.raises(ValueError):
        wm._signal_workers('foo', signal=9)

    # Signal all tasks (they all ended anyway)
    wm._signal_workers('all', signal=15)

def test_timeout(wm, sleep_task):
    """Tests whether the timeout succeeds"""
    # Add some sleep tasks
    for _ in range(3):
        wm.add_task(**sleep_task)
    # NOTE With two workers, this should take approx 1 second to execute

    # Check if the run does not start for an invalid timeout value
    with pytest.raises(ValueError):
        wm.start_working(timeout=-123.45)

    # Check if no WorkerManagerTotalTimeout is raised for a high timeout value
    wm.start_working(timeout=1.7)

    # Add more asks
    for _ in range(3):
        wm.add_task(**sleep_task)

    # Test if one is raised for a smaller timeout value
    with pytest.raises(WorkerManagerTotalTimeout):
        wm.start_working(timeout=0.7)

def test_stopconds(wm, wm_with_tasks, longer_sleep_task, sc_run_kws):
    """Tests the stop conditions"""
    # Populate the basic wm with two sleep tasks
    wm.add_task(**longer_sleep_task)
    wm.add_task(**longer_sleep_task)

    # Start working, with timeout_wall == 0.4 seconds
    wm.start_working(**sc_run_kws)

    # Assert that there are no workers remaining and that both have exit status -15
    assert wm.active_tasks == []
    for task in wm.tasks:
        assert task.worker_status == -15

    # Now to the more complex setting
    wm = wm_with_tasks
    wm.start_working(**sc_run_kws)

    # TODO more tests here

@pytest.mark.skip("Properly implement this!")
def test_read_stdout(wm):
    """Checks if the stdout was read"""

    wm.start_working()

    # TODO read the stream output here



def test_task_queue(wm, tmpdir):
    """Checks tasks are order properly in queue, according to priority from -inf(high priority) to inf(low)"""
    # TODO Nicen the Test
    for num_workers in [1, 2, 10]:
        os.mkdir(os.path.join(tmpdir, str(num_workers)))
        print(os.listdir(tmpdir))
        wm.num_workers = num_workers  # that tasks are not only started but also finished in order

        # Set a json-reading function
        line_read_json = lambda queue, stream: enqueue_lines(queue=queue,
                                                             stream=stream,
                                                             parse_func=parse_json)
                                            
        # A few tasks
        tasks = []
        priorities_ids = []  # [(pri,id),...]
        id_i = 0
        for i in range(10):
            if rd.random() > 0.3:
                priority = rd.random()
            else:
                priority = np.inf  # None #None does not work as priority is set manually and not via task class
            tasks.append(dict(worker_kwargs=dict(args=('python3', '-c',
                                                       'import os; from time import time;'
                                                       'print(os.listdir("{0}")); os.mkdir(os.path.join("{0}", "{1}", str(time()),"{2}","{3}"))'.format(tmpdir, num_workers, priority, id_i)
                                                       #'print("priority : "+str(priority)+"  "+"id : "+str(id_i))'
                                                       ),
                                                 read_stdout=True,
                                                 line_read_func=line_read_json),
                              priority=priority))
            if priority is None:
                priority = np.inf
            priorities_ids.append((priority, id_i))
            id_i += 1

        # And pass the tasks
        for task_dict in tasks:
            wm.add_task(**task_dict)

        wm.start_working()

        # TODO read the stream output here and chek priorities and ids are order right, may simpler than building so much folders
        print('Used task list', priorities_ids)
        # sort local list
        priorities_ids.sort()
        print('Sorted Used task list', priorities_ids)
        # check folder structure
        act_path = os.path.join(tmpdir, str(num_workers))
        folders = [name for name in os.listdir(act_path)]
        folders.sort()
        for i, folder in enumerate(folders):
            if not os.listdir(os.path.join(act_path, str(folder)))[0] == priorities_ids[i][0]:
                raise RuntimeError('WrongOrder Priority')
            if not os.listdir(os.path.join(act_path, str(folder), str(priorities_ids[i][0])))[0] == priorities_ids[i][1]:
                raise RuntimeError('WrongOrder ID')
        
