"""Tests the WorkerManager class"""

import pytest
from utopya.workermanager import WorkerManager


@pytest.fixture
def wm():
    """Create the simplest possible WorkerManager class"""
    return WorkerManager(num_workers=1)

def test_init(wm):
    """Tests whether initialisation succeeds"""
    return