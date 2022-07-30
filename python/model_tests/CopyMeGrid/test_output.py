"""Tests of the output of the CopyMeGrid model"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class for CopyMeGrid
mtc = ModelTest("CopyMeGrid", test_file=__file__)  # TODO set the model name


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
    # NOTE can also use a shortcut to do all of the above, see test_output

    # Assert that data was loaded, i.e. that data was written
    assert len(mv.dm)


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
        print(f"\nRunning '{cfg_name}' configuration ...")

        mv, _ = mtc.create_run_load(
            from_cfg=cfg_paths.get("run"), parameter_space=dict(num_steps=3)
        )
        mv.pm.plot_from_cfg(plots_cfg=cfg_paths.get("eval"))

        print(f"Succeeded running and evaluating '{cfg_name}'.\n")


# NOTE The below test is an example of how to access data and write a test.
#      You can adjust this to your needs and then remove the decorator to
#      enable the test.
# TODO Adapt this to the data you are putting out
def test_output():
    """Test that the output structure is correct"""
    # Create a Multiverse and let it run
    mv, dm = mtc.create_run_load(from_cfg="output.yml", perform_sweep=True)
    # NOTE this is a shortcut. It creates the mv, lets it run, then loads data

    # Get the meta-config from the DataManager
    mcfg = dm["cfg"]["meta"]
    print("meta config: ", mcfg)

    # For each universe, iterate over the output data and assert the shape
    # and the content of the output data
    for uni_no, uni in dm["multiverse"].items():
        # Get the data
        data = uni["data"]["CopyMeGrid"]  # TODO change this to your model name

        # Get the config of this universe
        uni_cfg = uni["cfg"]

        # Check that all datasets are available
        assert "some_state" in data
        assert "some_trait" in data

        # It is very helpful to make use of the xarray functionality!
        # See:  http://xarray.pydata.org/en/stable/why-xarray.html
        some_state = data["some_state"]
        assert some_state.dims == ("time", "x", "y")

        some_trait = data["some_trait"]
        assert some_trait.dims == ("time", "x", "y")

        # Can do further tests here ...
