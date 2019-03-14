"""Test the plotting module"""
import os
import uuid

import pytest

from utopya import Multiverse
from pkg_resources import resource_filename

# Get the test resources
BIFURCATION_CFG_PATH = resource_filename('test', 
                            'cfg/test_plotting_bifurcation_'
                            'codim_one_savanna_cfg.yml')
BIFURCATION_PLOTS_PATH = resource_filename('test', 
                            'cfg/test_plotting_bifurcation_'
                            'codim_one_savanna_plots.yml')
BIFURCATION_SWEEP_CFG_PATH = resource_filename('test', 
                            'cfg/test_plotting_bifurcation_'
                            'codim_one_sweep_init_savanna_cfg.yml')
BIFURCATION_SWEEP_PLOTS_PATH = resource_filename('test', 
                            'cfg/test_plotting_bifurcation_'
                            'codim_one_sweep_init_savanna_plots.yml')


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

    # Plot the default configuration, which already includes some CA plotting
    mv.pm.plot_from_cfg()

    # To explicitly plot with the frames writer, select the disabled config
    mv.pm.plot_from_cfg(plot_only=["strategy_and_payoff_frames"])

def test_bifurcation_codim_one_plotting(tmpdir):
    """Tests the plot_funcs submodule using the SavannaHomogeneous model
    
    Sweep the growth parameter alpha and use it as a bifurcation axis
    """
    mv = Multiverse(model_name='SavannaHomogeneous',
                    run_cfg_path=BIFURCATION_CFG_PATH,
                    paths=dict(out_dir=str(tmpdir)))
    mv.run_sweep()

    # Load
    mv.dm.load_from_cfg(print_tree=False)

    # Plot the default configuration
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_PLOTS_PATH,
                        plot_only=["bifurcation_alpha"])
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_PLOTS_PATH,
                        plot_only=["bifurcation_alpha_scatter"])
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_PLOTS_PATH,
                        plot_only=["bifurcation_alpha_find_peaks"])

    # with pytest.raises(ValueError):
    #     mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_PLOTS_PATH,
    #                     plot_only=["bifurcation_invalid_dim"])
    # with pytest.raises(ValueError):
    #     mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_PLOTS_PATH,
    #                     plot_only=["bifurcation_invalid_codim"])
    # with pytest.raises(ValueError):
    #     mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_PLOTS_PATH,
    #                     plot_only=["bifurcation_invalid_prop_name"])


def test_bifurcation_codim_one_sweep_init_plotting(tmpdir):
    """Tests the plot_funcs submodule using the SavannaHomogeneous model
    
    Sweep the growth parameter alpha and use it as a bifurcation axis.
    Sweep the initial_state for a codimension.
    """
    mv = Multiverse(model_name='SavannaHomogeneous',
                    run_cfg_path=BIFURCATION_SWEEP_CFG_PATH,
                    paths=dict(out_dir=str(tmpdir)))
    mv.run_sweep()

    # Load
    mv.dm.load_from_cfg(print_tree=False)

    # Plot the default configuration
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_SWEEP_PLOTS_PATH)
