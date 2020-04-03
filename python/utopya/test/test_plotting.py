"""Test the plotting module"""

from pkg_resources import resource_filename

import pytest

import paramspace as psp
from dantro.plot_mngr import PlotCreatorError

from utopya import Multiverse
from utopya.testtools import ModelTest


# Get the test resources ......................................................
# Basic universe plots
BASIC_UNI_PLOTS = resource_filename('test', 'cfg/plots/basic_uni.yml')

# Bifurcation diagram, 1D and 2D
BIFURCATION_DIAGRAM_RUN = resource_filename('test',
                                'cfg/plots/bifurcation_diagram/run.yml')
BIFURCATION_DIAGRAM_PLOTS = resource_filename('test',
                                'cfg/plots/bifurcation_diagram/plots.yml')
BIFURCATION_DIAGRAM_2D_RUN = resource_filename('test',
                                'cfg/plots/bifurcation_diagram_2d/run.yml')
BIFURCATION_DIAGRAM_2D_PLOTS = resource_filename('test',
                                'cfg/plots/bifurcation_diagram_2d/plots.yml')

# Fixtures --------------------------------------------------------------------


# Tests -----------------------------------------------------------------------

def test_pcr_ext_extensions():
    """Test the changes and extensions to the ExternalPlotCreator

    ... indirectly, using some other plot creator.
    """
    model = ModelTest('dummy')
    mv, dm = model.create_run_load()
    mv.pm.raise_exc = True
    print(dm.tree)

    # Can do the default plots
    mv.pm.plot_from_cfg()

    # Can specify custom plots within utopya ... but will receive an error
    # because the data dimensionality does not match
    with pytest.raises(PlotCreatorError, match="Apply dimensionality red"):
        mv.pm.plot("test", out_dir=mv.dirs['eval'],
                   creator='universe', universes='all',
                   module='.basic_uni', plot_func='lineplot',
                   model_name='dummy', path_to_data='state')

    # Cannot do a custom plot with a bad relative module import
    with pytest.raises(PlotCreatorError, match="ModuleNotFoundError"):
        mv.pm.plot("test2", out_dir=mv.dirs['eval'],
                   creator='universe', universes='all',
                   module='.some_invalid_module', plot_func='lineplot')

    # How about absolute imports? Nope.
    with pytest.raises(PlotCreatorError, match="No module named 'some'"):
        mv.pm.plot("test3", out_dir=mv.dirs['eval'],
                   creator='universe', universes='all',
                   module='some.abs.invalid.module', plot_func='lineplot')

    # ... and when an import fails for a custom model plot module?
    with pytest.raises(PlotCreatorError,
                       match="_unrelated_ ModuleNotFoundError occurred somew"):
        mv.pm.plot("test4", out_dir=mv.dirs['eval'],
                   creator='universe', universes='all',
                   module='model_plots.some_invalid_model_name',
                   plot_func='lineplot')

def test_dummy_plotting():
    """Test plotting of the dummy model works"""
    mv, _ = ModelTest('CopyMeGrid').create_run_load()

    # Plot the default configuration
    mv.pm.plot_from_cfg()


def test_CopyMeGrid_plotting():
    """Test plotting of the CopyMeGrid model works"""
    mv, _ = ModelTest('CopyMeGrid').create_run_load()

    # Plot the default configuration
    mv.pm.plot_from_cfg()


def test_basic_plotting(tmpdir):
    """Tests the plot_funcs.basic_* modules"""
    mv, _ = ModelTest('CopyMeGrid').create_run_load()

    mv.pm.plot_from_cfg(plots_cfg=BASIC_UNI_PLOTS,
                        plot_only=["distance_map"])


def test_ca_plotting():
    """Tests the plot_funcs.ca module"""
    mv, _ = ModelTest('CopyMeGrid').create_run_load()

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
    mv.pm.plot_from_cfg(plot_only=['species_densities', 'phase_space'])


def test_distribution_plots():
    """Tests the plot_funcs.distribution module"""
    mv, _ = ModelTest('SandPile').create_run_load()

    # Plot specific plots from the default plot configuration, which are using
    # the distribution plots
    mv.pm.plot_from_cfg(plot_only=['compl_cum_prob_dist',
                                   'cluster_size_distribution'])

@pytest.mark.skip(reason="Need alternative way of testing this")
def test_bifurcation_diagram(tmpdir):
    """Test plotting of the bifurcation diagram"""
    # Create and run simulation
    raise_exc = {'plot_manager': {'raise_exc': True}}
    mv = Multiverse(model_name='SavannaHomogeneous',
                    run_cfg_path=BIFURCATION_DIAGRAM_RUN,
                    paths=dict(out_dir=str(tmpdir)),
                    **raise_exc)
    mv.run_sweep()

    # Load
    mv.dm.load_from_cfg(print_tree=False)

    # Plot the bifurcation using the last datapoint
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_PLOTS,
                        plot_only=["bifurcation_endpoint"])
    # Plot the bifurcation using the fixpoint
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_PLOTS,
                        plot_only=["bifurcation_fixpoint"])
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_PLOTS,
                        plot_only=["bifurcation_fixpoint_to_plot"])
    # Plot the bifurcation using scatter
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_PLOTS,
                        plot_only=["bifurcation_scatter"])
    # Plot the bifurcation using oscillation
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_PLOTS,
                        plot_only=["bifurcation_oscillation"])


    # Redo simulation, but using several initial conditions
    mv = Multiverse(model_name='SavannaHomogeneous',
                    run_cfg_path=BIFURCATION_DIAGRAM_RUN,
                    paths=dict(out_dir=str(tmpdir)),
                    **raise_exc,
                    parameter_space=dict(
                        seed=psp.ParamDim(default=0, range=[4])))
    mv.run_sweep()
    mv.dm.load_from_cfg(print_tree=False)

    # Plot the bifurcation using multistability
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_PLOTS,
                        plot_only=["bifurcation_fixpoint"])

@pytest.mark.skip(reason="Need alternative way of testing this")
def test_bifurcation_diagram_2d(tmpdir):
    """Test plotting of the bifurcation diagram"""
    # Create and run simulation
    raise_exc = {'plot_manager': {'raise_exc': True}}
    mv = Multiverse(model_name='SavannaHomogeneous',
                    run_cfg_path=BIFURCATION_DIAGRAM_2D_RUN,
                    paths=dict(out_dir=str(tmpdir)),
                    **raise_exc)
    mv.run_sweep()

    # Load
    mv.dm.load_from_cfg(print_tree=False)

    # Plot the bifurcation using the last datapoint
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_2D_PLOTS,
                        plot_only=["bifurcation_diagram_2d"])
    # Plot the bifurcation using the fixpoint
    mv.pm.plot_from_cfg(plots_cfg=BIFURCATION_DIAGRAM_2D_PLOTS,
                        plot_only=["bifurcation_diagram_2d_fixpoint_to_plot"])
