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
def wm() -> WorkerManager:
    """Create a WorkerManager instance with multiple sleep tasks"""
    # Initialise a worker manager
    wm = WorkerManager(num_workers=2)

    # And pass the tasks
    sleep_args = ('python3', '-c', 'from time import sleep; sleep(0.5)')
    for _ in range(9):
        wm.add_task(worker_kwargs=dict(args=sleep_args, read_stdout=True))

    return wm

@pytest.fixture
def rep(wm) -> WorkerManagerReporter:
    """Returns a WorkerManagerReporter"""
    return WorkerManagerReporter(wm, min_report_intv=MIN_REP_INTV)

# Tests -----------------------------------------------------------------------

def test_init(wm):
    """Test initialisation of the WorkerManagerReporter"""
    rep = WorkerManagerReporter(wm, min_report_intv=MIN_REP_INTV)

def test_report(rep):
    """Tests the report method."""
    # Ensure that reporting is not carried out if inside the min report intv.
    assert rep.report()
    assert not rep.report()
    time.sleep(MIN_REP_INTV)
    assert rep.report()
    assert not rep.report()
    
