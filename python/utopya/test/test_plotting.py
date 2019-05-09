"""Test the plotting module"""

import os
import uuid
from pkg_resources import resource_filename

import pytest

import paramspace as psp

from utopya import Multiverse
from utopya.testtools import ModelTest

# Get the test resources
BIFURCATION_DIAGRAM_RUN_CFG_PATH = resource_filename('test', 
                            'cfg/test_plotting__bifurcation_diagram__'
                            'run_cfg.yml')
BIFURCATION_DIAGRAM_PLOTS_CFG_PATH = resource_filename('test', 
                            'cfg/test_plotting__bifurcation_diagram__'
                            'plots_cfg.yml')
BIFURCATION_DIAGRAM_2D_RUN_CFG_PATH = resource_filename('test', 
                            'cfg/test_plotting__bifurcation_diagram_2d__'
                            'run_cfg.yml')
BIFURCATION_DIAGRAM_2D_PLOTS_CFG_PATH = resource_filename('test', 
                            'cfg/test_plotting__bifurcation_diagram_2d__'
                            'plots_cfg.yml')


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


def test_basic_plotting(tmpdir):
    """Tests the plot_funcs.basic_* modules"""
    mv, _ = ModelTest('CopyMe').create_run_load()
    
    # test lineplots
    mv, _ = ModelTest('SavannaHeterogeneous').create_run_load()

    mv.pm.plot_from_cfg(plot_only=["state_densities"])
    mv.pm.plot_from_cfg(plot_only=["phase_diagram"])


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
    mv.pm.plot_from_cfg(plot_only=['area_fraction'])
    

    # Again, with PredatorPrey
    mv, _ = ModelTest('PredatorPrey').create_run_load()

    # Plot specific plots from the default plot configuration, which are using
    # the time_series plots
    mv.pm.plot_from_cfg(plot_only=['densities'])


def test_distribution_plots():
    """Tests the plot_funcs.distribution module"""
    mv, _ = ModelTest('SandPile').create_run_load()

    # Plot specific plots from the default plot configuration, which are using
    # the distribution plots
    mv.pm.plot_from_cfg(plot_only=['compl_cum_prob_dist',
                                   'cluster_size_distribution'])


def test_bifurcation_diagram(tmpdir):
    """Test plotting of the bifurcation diagram"""
    # Create and run simulation
    raise_exc = {'plot_manager': {'raise_exc': True}}
    mv = Multiverse(model_name='SavannaHomogeneous',
                    run_cfg_path=BIFURCATION_DIAGRAM_RUN_CFG_PATH,
                    paths=dict(out_dir=str(tmpdir)),
                    **raise_exc)
    mv.run_sweep()

    # Load
    mv.dm.load_from_cfg(print_tree=False)

    # Plot the bifurcation using the last datapoint
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_PLOTS_CFG_PATH,
                        plot_only=["bifurcation_endpoint"])
    # Plot the bifurcation using the fixpoint
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_PLOTS_CFG_PATH,
                        plot_only=["bifurcation_fixpoint"])
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_PLOTS_CFG_PATH,
                        plot_only=["bifurcation_fixpoint_to_plot"])
    # Plot the bifurcation using scatter
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_PLOTS_CFG_PATH,
                        plot_only=["bifurcation_scatter"])
    # Plot the bifurcation using oscillation
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_PLOTS_CFG_PATH,
                        plot_only=["bifurcation_oscillation"])


    # Redo simulation, but using several initial conditions
    mv = Multiverse(model_name='SavannaHomogeneous',
                    run_cfg_path=BIFURCATION_DIAGRAM_RUN_CFG_PATH,
                    paths=dict(out_dir=str(tmpdir)),
                    **raise_exc,
                    parameter_space=dict(
                        seed=psp.ParamDim(default=0, range=[4])))
    mv.run_sweep()
    mv.dm.load_from_cfg(print_tree=False)

    # Plot the bifurcation using multistability
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_PLOTS_CFG_PATH,
                        plot_only=["bifurcation_fixpoint"])

def test_bifurcation_diagram_2d(tmpdir):
    """Test plotting of the bifurcation diagram"""
    # Create and run simulation
    raise_exc = {'plot_manager': {'raise_exc': True}}
    mv = Multiverse(model_name='SavannaHomogeneous',
                    run_cfg_path=BIFURCATION_DIAGRAM_2D_RUN_CFG_PATH,
                    paths=dict(out_dir=str(tmpdir)),
                    **raise_exc)
    mv.run_sweep()

    # Load
    mv.dm.load_from_cfg(print_tree=False)

    # Plot the bifurcation using the last datapoint
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_2D_PLOTS_CFG_PATH,
                        plot_only=["bifurcation_diagram_2d"])
    # Plot the bifurcation using the fixpoint
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_2D_PLOTS_CFG_PATH,
                        plot_only=["bifurcation_diagram_2d_fixpoint_to_plot"])
