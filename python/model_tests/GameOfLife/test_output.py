"""Tests of the output of the GameOfLife model"""

import numpy as np

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class for GameOfLife
mtc = ModelTest("GameOfLife", test_file=__file__)


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
        print("\nRunning '{}' configuration ...".format(cfg_name))

        mv, _ = mtc.create_run_load(from_cfg=cfg_paths.get('run'),
                                    parameter_space=dict(num_steps=3))
        mv.pm.plot_from_cfg(plots_cfg=cfg_paths.get('eval'))

        print("Succeeded running and evaluating '{}'.\n".format(cfg_name))


# NOTE The below test is an example of how to access data and write a test.
#      You can adjust this to your needs and then remove the decorator to
#      enable the test.
def test_output():
    """Test that the output structure is correct"""
    # Create a Multiverse and let it run
    mv, dm = mtc.create_run_load(from_cfg="output.yml", perform_sweep=True)
    # NOTE this is a shortcut. It creates the mv, lets it run, then loads data

    # Get the meta-config from the DataManager
    mcfg = dm['cfg']['meta']
    print("meta config: ", mcfg)

    # For each universe, iterate over the output data and assert the shape
    # and the content of the output data
    for uni_no, uni in dm['multiverse'].items():
        # Get the data
        data = uni['data']['GameOfLife']

        # Get the config of this universe
        uni_cfg = uni['cfg']

        # Check that all datasets are available
        assert 'living' in data

        # It is very helpful to make use of the xarray functionality!
        # See:  http://xarray.pydata.org/en/stable/why-xarray.html
        living = data['living']
        assert living.dims == ('time', 'x', 'y')

        # Check that in each time-step there are living
        # as well as dead cells
        for t, d in living.groupby('time'):
            assert 0 in d
            assert 1 in d
