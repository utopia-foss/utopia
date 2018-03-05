"""Tests the WorkerManager class"""

import pytest
from utopya.workermanager import WorkerManager


@pytest.fixture
def wm():
    """Create the simplest possible WorkerManager class"""
    return WorkerManager(num_workers=1)

def test_init(wm):
    """Tests whether initialisation succeeds"""
    # Test different `num_workers` arguments
    WorkerManager(num_workers='auto')
    WorkerManager(num_workers=-1)
    WorkerManager(num_workers=1)

    # Should warn (too many workers)
    with pytest.warns(UserWarning):
        WorkerManager(num_workers=1000)

    # Should fail (not int or negative)
    with pytest.raises(ValueError):
        WorkerManager(num_workers=-1000)
    
    with pytest.raises(ValueError):
        WorkerManager(num_workers=1.23)