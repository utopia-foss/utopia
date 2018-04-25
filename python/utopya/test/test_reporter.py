"""Tests the Reporter class and derived classes."""

import time

import pytest

from utopya.workermanager import WorkerManager
from utopya.task import enqueue_lines, parse_json
from utopya.reporter import Reporter, WorkerManagerReporter

# Local constants
MIN_REP_INTV = 0.1

# Fixtures --------------------------------------------------------------------

@pytest.fixture
def sleep_task() -> dict:
    """The kwargs for a sleep task"""
    sleep_args = ('python3', '-c', 'from time import sleep; sleep(0.1)')
    return dict(worker_kwargs=dict(args=sleep_args, read_stdout=True))

@pytest.fixture
def wm(sleep_task) -> WorkerManager:
    """Create a WorkerManager instance with multiple sleep tasks"""
    # Initialise a worker manager
    wm = WorkerManager(num_workers=4)

    # And pass some tasks
    for _ in range(11):
        wm.add_task(**sleep_task)

    return wm

@pytest.fixture
def rf_list() -> list:
    """Returns a list of report formats; this will invoke them with their
    default settings."""
    return ['runtime', 'task_counters', 'progress', 'progress_bar']

@pytest.fixture
def rf_dict() -> dict:
    """Returns a report format dict."""
    return dict(runtime=dict(min_report_intv=MIN_REP_INTV),
                tasks=dict(parser='task_counters'),
                progress=dict(),
                short_progress_bar=dict(parser='progress_bar', num_cols=19))

@pytest.fixture
def rep(wm, rf_list) -> WorkerManagerReporter:
    """Returns a WorkerManagerReporter"""
    return WorkerManagerReporter(wm, report_formats=rf_list,
                                 default_format='runtime')

# Tests -----------------------------------------------------------------------

def test_init(wm):
    """Test simplest initialisation of the WorkerManagerReporter"""
    rep = WorkerManagerReporter(wm)

    # Associating another one with the same WorkerManager should fail
    with pytest.raises(RuntimeError, match="Already set the reporter"):
        WorkerManagerReporter(wm)

    # Ensure correct association
    assert rep.wm is wm
    assert wm._reporter is rep

def test_init_with_rf_list(wm, rf_list):
    """Test initialisation of the WorkerManagerReporter with report formats"""
    rep = WorkerManagerReporter(wm, report_formats=rf_list)

def test_init_with_rf_dict(wm, rf_dict):
    """Test initialisation of the WorkerManagerReporter"""
    rep = WorkerManagerReporter(wm, report_formats=rf_dict,
                                default_format='runtime')

def test_add_report_format(rep):
    """Test the add_report_format method"""

    # Adding formats with the same name should not work
    with pytest.raises(ValueError, match="A report format with the name"):
        rep.add_report_format('runtime')

    # Invalid write_to argument
    with pytest.raises(TypeError,
                       match="Invalid type for argument `write_to`"):
        rep.add_report_format('foo', parser='runtime', write_to=(1, 2, 3))

    # Invalid parser
    with pytest.raises(ValueError, match="No parser named"):
        rep.add_report_format('invalid')
    
    # Invalid writer
    with pytest.raises(ValueError, match="No writer named"):
        rep.add_report_format('foo', parser='runtime', write_to='invalid')

def test_task_counter(rep):
    """Tests the task_counters property"""

    # Assert initial worker manager state is as desired
    tc = rep.task_counters
    assert tc['total'] == 11
    assert tc['queued'] == 11
    assert tc['active'] == 0
    assert tc['finished'] == 0

    # Let the WorkerManager work and then test again
    rep.wm.start_working()

    tc = rep.task_counters
    assert tc['total'] == 11
    assert tc['queued'] == 0
    assert tc['active'] == 0
    assert tc['finished'] == 11

def test_parsers(rep, sleep_task):
    """Tests the custom parser methods of the WorkerManagerReporter that is
    written for simple terminal reports
    """

    # Disable minimum report interval and create shortcuts to the parse methods
    rep.min_report_intv = None
    ptc = rep._parse_task_counters
    pp = rep._parse_progress
    ppb = lambda *a, **kws: rep._parse_progress_bar(*a, num_cols=19, **kws)

    # Test the initial return strings
    assert ptc() == "total: 11,  queued: 11,  active: 0,  finished: 0"
    assert pp() == "Finished   0 / 11  (0.0%)"
    assert ppb() == "╠          ╣   0.0%"
    
    # Start working and check again afterwards
    rep.wm.start_working()
    assert ptc() == "total: 11,  queued: 0,  active: 0,  finished: 11"
    assert pp() == "Finished  11 / 11  (100.0%)"
    assert ppb() == "╠▓▓▓▓▓▓▓▓▓▓╣ 100.0%"

    # Add another task to the WorkerManager, which should change the counts
    rep.wm.add_task(**sleep_task)
    assert ptc() == "total: 12,  queued: 1,  active: 0,  finished: 11"
    assert pp() == "Finished  11 / 12  (91.7%)"
    assert ppb() == "╠▓▓▓▓▓▓▓▓▓ ╣  91.7%"

def test_report(rep):
    """Tests the report method."""
    # Test the default report format
    assert rep.report()

    # Other report formats
    assert rep.report('task_counters')
    assert rep.report('progress')
    assert rep.report('progress_bar')

    # Invalid report formats
    with pytest.raises(KeyError):
        rep.report('foo')

def test_min_report_intv(wm, rf_dict):
    """Test correct behaviour of the minimum report interval"""
    rep = WorkerManagerReporter(wm, report_formats=rf_dict,
                                default_format='runtime')

    # This should work
    assert rep.report()

    # Now this should hit the minimum report interval
    assert not rep.report()
    assert not rep.report()

    # After sleeping, this should work again
    time.sleep(MIN_REP_INTV)
    assert rep.report()

    # And blocked again ...
    assert not rep.report()
