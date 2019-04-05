"""Test the plotting module"""

import os
import uuid
from pkg_resources import resource_filename

import pytest

from utopya import Multiverse
from utopya.testtools import ModelTest

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

def test_dummy_plotting():
    """Test plotting of the dummy model works"""
    mv, _ = ModelTest('CopyMe').create_run_load()
    
    # Plot the default configuration
    mv.pm.plot_from_cfg()


def test_CopyMe_plotting():
    """Test plotting of the CopyMe model works"""
    mv, _ = ModelTest('CopyMe').create_run_load()

    # Plot the default configuration
    mv.pm.plot_from_cfg()


def test_basic_plots(tmpdir):
    """Tests the plot_funcs.basic module"""
    mv, _ = ModelTest('CopyMe').create_run_load()
    mv.pm.plot("all_states",
               creator='universe',
               universes='all',
               out_dir=str(tmpdir),
               module=".basic",
               plot_func="lineplot",
               y="dummy/state"
               )


def test_ca_plotting():
    """Tests the plot_funcs.ca module"""
    mv, _ = ModelTest('CopyMe').create_run_load()

    # Run the CA plots (initial frame + animation)
    mv.pm.plot_from_cfg(plot_only=["initial_state_and_trait"])
    mv.pm.plot_from_cfg(plot_only=["state_and_trait_anim"])


    # Same again with SimpleEG . . . . . . . . . . . . . . . . . . . . . . . . 
    mv, _ = ModelTest('SimpleEG').create_run_load()
    
    # Plot the default configuration, which already includes some CA plotting
    mv.pm.plot_from_cfg()

    # To explicitly plot with the frames writer, select the disabled config
    mv.pm.plot_from_cfg(plot_only=["strategy_and_payoff_frames"])


def test_time_series_plots():
    """Tests the plot_funcs.time_series module"""
    mv, _ = ModelTest('SandPile').create_run_load()

    # Plot specific plots from the default plot configuration, which are using
    # the time_series plots
    mv.pm.plot_from_cfg(plot_only=['area_fraction', 'mean_slope'])


def test_distribution_plots():
    """Tests the plot_funcs.distribution module"""
    mv, _ = ModelTest('SandPile').create_run_load()

    # Plot specific plots from the default plot configuration, which are using
    # the distribution plots
    mv.pm.plot_from_cfg(plot_only=['compl_cum_prob_dist', 'cluster_size_dist'])


@pytest.mark.skipped(reason="to be implemented!")  # TODO
def test_attractor_plots():
    """Tests the plot_funcs.attractor module"""
    mv, _ = ModelTest('SELECT_MODEL').create_run_load()

    # Plot specific plots from the default plot configuration, which are using
    # the attractor plots
    mv.pm.plot_from_cfg(plot_only=['ATTRACTOR_PLOT1', 'ATTRACTOR_PLOT2'])


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
