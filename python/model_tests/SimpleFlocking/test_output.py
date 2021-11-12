"""Tests of the output of the SimpleFlocking model"""

import numpy as np

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class
mtc = ModelTest("SimpleFlocking", test_file=__file__)  # TODO set the model name


# Fixtures --------------------------------------------------------------------
# Define fixtures


# Tests -----------------------------------------------------------------------

def test_that_it_runs():
    """Tests that the model runs through with the default settings"""
    # Create a Multiverse using the default model configuration
    mv = mtc.create_mv()

    # Run a single simulation
    mv.run_single()

    # Load data using the DataManager and the default load configuration
    mv.dm.load_from_cfg(print_tree=True)
    # The `print_tree` flag creates output of which data was loaded


def test_run_and_eval_cfgs():
    """Carries out all additional configurations that were specified alongside
    the default model configuration.

    This is done automatically for all run and eval configuration pairs that
    are located in subdirectories of the ``cfgs`` directory (at the same level
    as the default model configuration).
    If no run or eval configurations are given in the subdirectories, the
    respective defaults are used.

    See :py:meth:`~utopya.model.Model.run_and_eval_cfg_paths` for more info.
    """
    for cfg_name, cfg_paths in mtc.default_config_sets.items():
        print("\nRunning '{}' example ...".format(cfg_name))

        mv, _ = mtc.create_run_load(from_cfg=cfg_paths.get('run'),
                                    parameter_space=dict(num_steps=3))
        mv.pm.plot_from_cfg(plots_cfg=cfg_paths.get('eval'))

        print("Succeeded running and evaluating '{}'.\n".format(cfg_name))
