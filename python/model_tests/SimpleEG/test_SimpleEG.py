"""Tests of the output of the SimpleEG model"""

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class
mtc = ModelTest("SimpleEG", test_file=__file__)

# Fixtures --------------------------------------------------------------------
# Define fixtures here


# Helpers ---------------------------------------------------------------------

def set_initial_state(state: str, *, s1_fraction: float=None) -> dict:
    """Creates a dict that can be used to set the initial state"""
    d = dict(parameter_space=dict(SimpleEG=dict(initial_state=state)))

    if s1_fraction is not None:
        d['parameter_space']['SimpleEG']['s1_fraction'] = s1_fraction

    return d


# Tests -----------------------------------------------------------------------

def test_basics():
    """Test the most basic features of the model, e.g. that it runs"""
    # Create a Multiverse using the default model configuration
    mv = mtc.create_mv()

    # Run a single simulation
    mv.run_single()

    # Load data using the DataManager
    mv.dm.load_from_cfg(print_tree=True)
    # The `print_tree` flag creates output of which data was loaded

    # Assert that data was loaded, i.e. that data was written
    assert len(mv.dm)


def test_initial_states(): 
    """Test that the initial states are"""
    # Use the config file for common settings, change it via additional kwargs
    mv1 = mtc.create_mv_from_cfg("initial_state.yml",
                                 **set_initial_state('random'))

    # Run the simulation (initial step only)
    mv.run_sweep()

    # Load data
    mv.dm.load_from_cfg(print_tree=True)

    # TODO add assertions below ...
    assert False
