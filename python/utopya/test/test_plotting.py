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
                    update_meta_cfg=dict(paths=dict(out_dir=tmpdir)))
    mv.run_single()

    # Load
    mv.dm.load_from_cfg(print_tree=True)

    # Plot the default configuration
    mv.pm.plot_from_cfg()

    # Perform a custom plot that tests the utopya plotting functions
    mv.pm.plot("all_states",
               creator="external",
               out_dir=str(tmpdir),
               module=".basic",
               plot_func="lineplot",
               uni=0,
               y="dummy/state"
               )
