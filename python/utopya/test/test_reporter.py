"""Tests the Reporter class and derived classes."""

import pytest

from utopya.workermanager import WorkerManager
from utopya.task import enqueue_lines, parse_json
from utopya.reporter import Reporter, WorkerManagerReporter

# Fixtures --------------------------------------------------------------------

@pytest.fixture
def wm():
    """Create a WorkerManager instance with multiple sleep tasks"""
    # Initialise a worker manager
    wm = WorkerManager(num_workers=2)

    # And pass the tasks
    sleep_args = ('python3', '-c', 'from time import sleep; sleep(0.5)')
    for _ in range(9):
        wm.add_task(worker_kwargs=dict(args=sleep_args, read_stdout=True))

    return wm

# Tests -----------------------------------------------------------------------

def test_init(wm):
    """Test initialisation of the WorkerManagerReporter"""
    rep = WorkerManagerReporter(wm)

    rep.report()
