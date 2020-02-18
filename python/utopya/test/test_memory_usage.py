"""Tests memory usage of parts of the Utopia C++ backend"""

from collections import defaultdict
from pkg_resources import resource_filename
from typing import Dict

import psutil
import pytest
import numpy as np

from utopya.testtools import Model, ModelTest
from utopya.yaml import load_yml
from utopya.tools import recursive_update

# Some constants
TEST_CFG_PATH = resource_filename('test', 'cfg/memory_usage.yml')

# Fixtures --------------------------------------------------------------------

@pytest.fixture
def cfg() -> dict:
    return load_yml(TEST_CFG_PATH)

def gather_memory_info(model: Model, *,
                       mv_kwargs: dict=None,
                       update_uni_cfg: dict=None
                       ) -> Dict[str, np.ndarray]:
    """Performs a single simulation task for the given model and returns a
    dict with information about the acquired memory usage.
    
    Args:
        model (Model): The model to run
        mv_kwargs (dict, optional): The keyword arguments used to update the
            meta configuration of the created Multiverse object
        update_uni_cfg (dict, optional): Arguments used to (recursively)
            update the configuration for the single universe that is being run
    
    Returns:
        Dict[str, np.ndarray]: A dict of memory usage sequences. Available keys
            are ``rss`` (for resident set size) and ``vms`` (for virtual
            memory size).
    """
    # A dict to hold the memory information
    mem_data = defaultdict(lambda: defaultdict(list))

    # Create a fresh multiverse, using some default values
    mv = model.create_mv(**(mv_kwargs if mv_kwargs else {}))

    def check_memory():
        """A callable to store memory usage of active tasks in ``mem_info``.

        NOTE This needs to be implemented here because it uses variables
             from the outer scope, including the mutable ``mem_info``.
        """
        # Go over all active tasks
        for task in mv.wm.active_tasks:
            # Using psutil, get some information on the memory and store it in
            # the (mutable) mem_info dict
            pid = task.worker_pid
            proc = psutil.Process(pid=pid)
            mem_info = proc.memory_info()

            mem_data[pid]['rss'].append(mem_info.rss)  # Resident Set Size
            mem_data[pid]['vms'].append(mem_info.vms)  # Virtual Memory Size

    # Get the uni config, extracting it from the parameter space's default;
    # then (if given) update it recursively
    pspace = mv.meta_cfg['parameter_space']
    uni_cfg = pspace.default
    if update_uni_cfg:
        uni_cfg = recursive_update(uni_cfg, update_uni_cfg)

    # Add a simulation task and start working, periodically polling the workers
    # and invoking the post_poll_function, which stores the memory in
    mv._add_sim_task(uni_id_str="0", uni_cfg=uni_cfg, is_sweep=False)
    mv.wm.start_working(**mv.meta_cfg["run_kwargs"],
                        post_poll_func=check_memory)

    # Convert sequences to numpy arrays
    for task_pid, task_mem_info in mem_data.items():
        mem_data[task_pid]['rss'] = np.array(task_mem_info['rss'])
        mem_data[task_pid]['vms'] = np.array(task_mem_info['vms'])

    return mem_data

# -----------------------------------------------------------------------------

def test_hdf5_write(cfg):
    """Tests the memory usage of the HDFDataset::write method using the
    HdfBench model.
    """
    # Set up the model instance
    model = ModelTest("HdfBench")

    # Default Multiverse initialization parameters
    mv_kwargs = cfg['default_mv_kwargs']

    # For each test case...
    for i, case_cfg in enumerate(cfg['test_cases']):
        print("\n{}#\nTest case {} commencing ...\nConfig: {}\n"
              "".format("#-"*39, i, case_cfg))

        # Gather memory
        mem_data = gather_memory_info(model, mv_kwargs=mv_kwargs,
                                      update_uni_cfg=case_cfg.get('uni_cfg'))

        # Get the maximum expected memory values
        max_rss = case_cfg['max_rss']              # resident set size
        max_vms = case_cfg.get('max_vms', np.inf)  # virtual memory size
        # NOTE The resident set size is probably the most representative here.
        #      For that reason, the vms is optional

        # Assert the memory used by the task is below some value (for all PIDs)
        for task_pid, task_mem_data in mem_data.items():
            assert(max(task_mem_data['rss']) <= max_rss)  # Resident set size
            assert(max(task_mem_data['vms']) <= max_vms)  # Virtual memory size
