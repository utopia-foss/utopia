"""Test the Task class implementation"""

from typing import List

import numpy as np
import pytest

from utopya.task import Task


# Fixtures ----------------------------------------------------------------

@pytest.fixture
def tasks() -> List[Task]:
    """Returns a list of tasks"""
    tasks = []
    for uid, priority in enumerate(np.random.random(size=50)):
        tasks.append(Task(uid=uid, priority=priority))

    return tasks

# Initialisation tests --------------------------------------------------------

def test_init():
    """Test task initialization"""
    Task(uid=0)
    Task(uid=1, priority=1000)

    # Invalid initialization arguments
    with pytest.raises(TypeError):
        Task(uid=1.23)
    with pytest.raises(ValueError):
        Task(uid=-1)

    # Test that the task ID cannot be changed
    with pytest.raises(RuntimeError):
        t = Task(uid=2)
        t.uid = 3

def test_sorting(tasks):
    """Tests whether different task objects are sortable"""
    tasks.sort()

    t1 = tasks[0]
    t2 = tasks[1]

    assert (t1 <= t2) is True
    assert (t1 == t2) is False
    assert (t1 == t1) is True

def test_magic_methods(tasks):
    """Test magic methods"""
    task_strs = [str(t) for t in tasks]
