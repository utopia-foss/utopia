"""Tests of the output of the SimpleFlocking model"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class
mtc = ModelTest("SimpleFlocking", test_file=__file__)


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

    # Test that some data was written
    for uni in mv.dm["multiverse"].values():
        data = uni["data/SimpleFlocking"]

        assert "agent/orientation" in data
        assert "agent/x" in data
        assert "agent/y" in data
        assert "norm_group_velocity" in data
        assert "orientation_circmean" in data
        assert "orientation_circstd" in data

        # Output orientation is in radians and expected range
        assert (data["agent/orientation"] > -np.pi).all()
        assert (data["agent/orientation"] < +np.pi).all()
        assert (data["orientation_circmean"] > -np.pi).all()
        assert (data["orientation_circmean"] < +np.pi).all()

        # Agent position is non-negative
        assert (data["agent/x"] >= 0).all()
        assert (data["agent/y"] >= 0).all()

        # Circular standard deviation and group velocity is positive
        assert (data["orientation_circstd"] >= 0).all()
        assert (data["norm_group_velocity"] >= 0).all()


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
        print(f"\nRunning '{cfg_name}' example ...")

        mv, _ = mtc.create_run_load(
            from_cfg=cfg_paths.get("run"), parameter_space=dict(num_steps=3)
        )
        mv.pm.plot_from_cfg(plots_cfg=cfg_paths.get("eval"))

        print(f"Succeeded running and evaluating '{cfg_name}'.\n")
