"""Tests for the HDF5 C++ Backend, as it is integrated into Utopia"""

from pkg_resources import resource_filename

import psutil
import pytest
import numpy as np

from utopya.testtools import ModelTest
from utopya.yaml import load_yml
from utopya.tools import recursive_update

# Some constants
TEST_CFG_PATH = resource_filename('test', 'cfg/hdf5backend.yml')

# Fixtures --------------------------------------------------------------------

@pytest.fixture
def cfg() -> dict:
    return load_yml(TEST_CFG_PATH)

# -----------------------------------------------------------------------------

def test_memory_usage_write(cfg):
    """Tests the memory usage of individual Utopia processes to assert that it
    does not grow beyond a certain limit.

    Here, the HDFDataset.write method's memory usage is tested.
    """
    # Set up the model instance
    model = ModelTest("HdfBench", use_tmpdir=False)

    # For each test case...
    for i, case_cfg in enumerate(cfg['test_cases']):
        print("\n{}#\nTest case {} commencing ...\nConfig: {}\n"
              "".format("#-"*39, i, case_cfg))

        # Create a fresh multiverse, using some default values
        mv = model.create_mv(**cfg['default_mv_init_kwargs'])

        # Get the maximum expected memory values
        max_rss = case_cfg['max_rss']              # resident set size
        max_vms = case_cfg.get('max_vms', np.inf)  # virtual memory size
        # NOTE The resident set size is probably the most representative here.
        #      For that reason, the vms is optional

        def check_memory():
            """A callable to check memory.

            NOTE This needs to be implemented here because it uses variables
                 from the outer scope
            """
            # Go over all active tasks
            for task in mv.wm.active_tasks:
                # Using psutil, get some information on the memory
                proc = psutil.Process(pid=task.worker_pid)
                mem_info = proc.memory_info()

                # Assert the memory used by the task is below some value
                assert(mem_info.rss <= max_rss)  # Resident set size
                assert(mem_info.vms <= max_vms)  # Virtual memory size


        # Get the uni config, extracting it from the parameter space's default
        pspace = mv.meta_cfg['parameter_space']
        uni_cfg = pspace.default

        # Update it with any additionally given config
        if case_cfg.get('uni_cfg'):
            uni_cfg = recursive_update(uni_cfg, case_cfg['uni_cfg'])

        # Add a simulation task and start working, periodically polling the
        # worker and invoking the post_poll_function
        mv._add_sim_task(uni_id_str="0",
                         uni_cfg=uni_cfg,
                         is_sweep=False)
        mv.wm.start_working(**mv.meta_cfg["run_kwargs"],
                            post_poll_func=check_memory)
