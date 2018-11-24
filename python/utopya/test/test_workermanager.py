"""Tests the WorkerManager class"""

import os
import queue
import pkg_resources

import numpy as np
import pytest

from utopya.workermanager import WorkerManager, WorkerManagerTotalTimeout, WorkerTaskNonZeroExit
from utopya.tools import read_yml

# Some constants
STOP_CONDS_PATH = pkg_resources.resource_filename('test', 'cfg/stop_conds.yml')

# Fixtures --------------------------------------------------------------------
@pytest.fixture
def wm():
    """Create the simplest possible WorkerManager instance"""
    return WorkerManager(num_workers=2, poll_delay=0.042)

@pytest.fixture
def wm_priQ():
    """Create simple WorkerManager instance with a PriorityQueue for the tasks.
        Priority from -inf to +inf (high to low)."""
    return WorkerManager(num_workers=2, poll_delay=0.01,
                         QueueCls=queue.PriorityQueue)

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
                                         stdout_parser="yaml_dict")))
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
    WorkerManager()
    WorkerManager(num_workers='auto')
    WorkerManager(num_workers=-1)
    WorkerManager(num_workers=1)

    with pytest.warns(UserWarning, match="Set WorkerManager to use more"):
        # too many workers
        WorkerManager(num_workers=1000)

    with pytest.raises(ValueError, match="Need positive integer"):
        # Not positive
        WorkerManager(num_workers=0)
    
    with pytest.raises(ValueError,
                       match="Received invalid argument `num_workers`"):
        # Too negative
        WorkerManager(num_workers=-1000)
    
    with pytest.raises(ValueError, match="Need positive integer"):
        # not int
        WorkerManager(num_workers=1.23)

    # Test different `poll_delay` arguments
    with pytest.raises(ValueError):
        # negative
        WorkerManager(num_workers=1, poll_delay=-1000)
    
    with pytest.warns(UserWarning):
        # small value
        WorkerManager(num_workers=1, poll_delay=0.001)

    # Test initialisation with different nonzero_exit_handling values
    WorkerManager(nonzero_exit_handling='ignore')
    WorkerManager(nonzero_exit_handling='warn')
    WorkerManager(nonzero_exit_handling='raise')
    with pytest.raises(ValueError, match="`nonzero_exit_handling` needs to"):
        WorkerManager(nonzero_exit_handling='invalid')

    # Test initialisation with an (invalid) Reporter type
    with pytest.raises(TypeError, match="Need a WorkerManagerReporter"):
        WorkerManager(reporter='not_a_reporter')
    # NOTE the tests with the actual WorkerManagerReporter can be found in
    # test_reporter.py, as they require adequate initialisation arguments
    # for which it would make no sense to make them available here

    # Test passing report specifications
    wm = WorkerManager(rf_spec=dict(foo='bar'))
    assert wm.rf_spec['foo'] == 'bar'

def test_add_tasks(wm, sleep_task):
    """Tests adding of tasks"""
    # This one should work
    wm.add_task(**sleep_task)

    # Test that warnings and errors propagate through    
    with pytest.warns(UserWarning, match="`worker_kwargs` given but also"):
        wm.add_task(setup_kwargs=dict(foo="bar"),
                    worker_kwargs=dict(foo="bar"))

    with pytest.raises(ValueError, match="Need either argument `setup_func`"):
        wm.add_task()

def test_start_working(wm_with_tasks):
    """Tests whether the start_working methods does what it should"""
    wm = wm_with_tasks
    wm.start_working()
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

def test_nonzero_exit_handling(wm):
    """Test that the non-zero exception handling works"""

    # Work sequentially
    wm.num_workers = 1

    # Generate a failing task config
    failing_task = dict(worker_kwargs=dict(args=("false",)))

    # Test that with 'ignore', everything runs as expected
    wm.nonzero_exit_handling = 'ignore'
    wm.add_task(**failing_task)
    wm.add_task(**failing_task)
    wm.start_working()
    assert wm.num_finished_tasks == 2

    # Now run through 'warning' mode
    wm.nonzero_exit_handling = 'warn'
    wm.add_task(**failing_task)
    wm.add_task(**failing_task)
    wm.start_working()
    assert wm.num_finished_tasks == 4

    # Now run through 'raise' mode
    wm.nonzero_exit_handling = 'raise'
    wm.add_task(**failing_task)
    wm.add_task(**failing_task)

    with pytest.raises(SystemExit) as pytest_wrapped_e:
        wm.start_working()
    
    assert pytest_wrapped_e.type == SystemExit
    assert pytest_wrapped_e.value.code == 1
    assert wm.num_finished_tasks == 5

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

@pytest.mark.skip("Feature not yet implemented.")
def test_detach(wm):
    pass

def test_empty_task_queue(wm):
    with pytest.raises(queue.Empty):
        wm._grab_task()

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
    wm.start_working(timeout=23.4)

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

def test_priority_queue(wm_priQ, sleep_task):
    """Checks that tasks are dispatched from the task queue in order of their
    priority, if a priority queue is used."""
    wm = wm_priQ

    # Create a list of priorities that should be checked
    prios         = [-np.inf, 2., 1., None, 0, -np.inf, 0, +np.inf, 1.]
    correct_order = [0,       6,  4,  7,    2, 1,       3, 8,       5]
    # If priorites are equal, the task added first have higher priority
    # NOTE cannot just order the list of tasks, because then there would be
    # no ground truth (the same ordering mechanism between tasks would be used)
    
    # Add tasks to the WorkerManager, keeping track of addition order
    tasks = []
    for prio in prios:
        tasks.append(wm.add_task(priority=prio, **sleep_task))

    # Now, start working
    wm.start_working()
    # Done working now.

    # Assert that the internal task list has the same order
    assert tasks == wm.tasks

    # Extract the creation times of the tasks for manual checks
    creation_times = [t.profiling['create_time'] for t in tasks]
    print("Creation times:")
    print("\n".join([str(e) for e in creation_times]))
    print("Correct order:", correct_order)

    # Sort task list by correct order and by creation times
    tasks_by_correct_order = [t for _, t in sorted(zip(correct_order, tasks))]
    tasks_by_creation_time = [t for _, t in sorted(zip(creation_times, tasks))]
    
    # Check that the two lists compare equal
    assert tasks_by_correct_order == tasks_by_creation_time
