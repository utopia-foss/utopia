"""Test the plotting module"""

import pytest

from utopya import Multiverse

# Local constants


# Fixtures --------------------------------------------------------------------


# Tests -----------------------------------------------------------------------

def test_dummy_plotting(tmpdir):
    """Test plotting of the dummy model"""
    # Create and run simulation
    mv = Multiverse(model_name='dummy',
                    paths=dict(out_dir=str(tmpdir)))
    mv.run_single()

    # Load
    mv.dm.load_from_cfg(print_tree=True)

    # Plot the default configuration
    mv.pm.plot_from_cfg()

    # Perform a custom plot that tests the utopya plotting functions
    mv.pm.plot("all_states",
               creator='universe',
               universes='all',
               out_dir=str(tmpdir),
               module=".basic",
               plot_func="lineplot",
               y="dummy/state"
               )

def test_ca_plotting(tmpdir):
    """Tests the plot_funcs submodule using the SimpleEG model"""
    mv = Multiverse(model_name='SimpleEG',
                    paths=dict(out_dir=str(tmpdir)))
    mv.run_single()

    # Load
    mv.dm.load_from_cfg(print_tree=True)

    # Plot the default configuration
    mv.pm.plot_from_cfg()

def test_bifurcation_codim_one_plotting(tmpdir):
    """Tests the plot_funcs submodule using the SavannaHomogeneous model"""
    mv = Multiverse(model_name='SavannaHomogeneous',
                    run_cfg_path="test/cfg/test_plotting_bifurcation_"
                                 "codim_one_savanna_cfg.yml",
                    paths=dict(out_dir=str(tmpdir)))
    mv.run_sweep()

    # Load
    mv.dm.load_from_cfg(print_tree=False)

    # Plot the default configuration
    mv.pm.plot_from_cfg(plots_cfg="test/cfg/test_plotting_bifurcation_"
                                  "codim_one_savanna_plots.yml")

def test_bifurcation_codim_one_sweep_init_plotting(tmpdir):
    """Tests the plot_funcs submodule using the SavannaHomogeneous model"""
    mv = Multiverse(model_name='SavannaHomogeneous',
                    run_cfg_path="test/cfg/test_plotting_bifurcation_"
                                 "codim_one_sweep_init_savanna_cfg.yml")#,
                    # paths=dict(out_dir=str(tmpdir)))
    mv.run_sweep()

    # Load
    mv.dm.load_from_cfg(print_tree=False)

    # Plot the default configuration
    mv.pm.plot_from_cfg(plots_cfg="test/cfg/test_plotting_bifurcation_"
                                  "codim_one_sweep_init_savanna_plots.yml")