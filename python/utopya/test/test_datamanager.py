"""Tests the UtopiaDataManager and involved functions and classes."""

import os
import uuid
from pkg_resources import resource_filename

import numpy as np

import pytest

from utopya import Multiverse, DataManager
import utopya.datacontainer as udc

# Local constants
RUN_CFG_PATH = resource_filename('test', 'cfg/run_cfg.yml')
SWEEP_CFG_PATH = resource_filename('test', 'cfg/sweep_cfg.yml')

# Fixtures --------------------------------------------------------------------

@pytest.fixture
def mv_kwargs(tmpdir) -> dict:
    """Returns a dict that can be passed to Multiverse for initialisation.

    This uses the `tmpdir` fixture provided by pytest, which creates a unique
    temporary directory that is removed after the tests ran through.
    """
    # Create a dict that specifies a unique testing path.
    # The str cast is needed for python version < 3.6
    rand_str = "test_" + uuid.uuid4().hex[:8]
    unique_paths = dict(out_dir=tmpdir, model_note=rand_str)

    return dict(model_name='dummy',
                run_cfg_path=RUN_CFG_PATH,
                user_cfg_path=False,  # to omit the user config
                update_meta_cfg=dict(paths=unique_paths)
                )

@pytest.fixture
def dm_after_single(mv_kwargs) -> DataManager:
    """Initialises a Multiverse with a DataManager, runs a simulation with
    output going into a temporary directory, then returns the DataManager."""
    # Initialise the Multiverse
    mv_kwargs['run_cfg_path'] = RUN_CFG_PATH
    mv = Multiverse(**mv_kwargs)

    # Run a sweep
    mv.run_single()
    
    # Return the data manager
    return mv.dm

@pytest.fixture
def dm_after_sweep(mv_kwargs) -> DataManager:
    """Initialises a Multiverse with a DataManager, runs a simulation with
    output going into a temporary directory, then returns the DataManager."""
    # Initialise the Multiverse
    mv_kwargs['run_cfg_path'] = SWEEP_CFG_PATH
    mv = Multiverse(**mv_kwargs)

    # Run a sweep
    mv.run_sweep()
    
    # Return the data manager
    return mv.dm

# Tests -----------------------------------------------------------------------

def test_init(tmpdir):
    """Tests initialisation of the Utopia data manager"""
    DataManager(str(tmpdir))

def test_load_single(dm_after_single):
    """Tests the loading of simulation data for a single simulation"""
    dm = dm_after_single

    # Load and print a tree of the loaded data
    dm.load_from_cfg(print_tree=True)

    # Check that the config is loaded as expected
    assert 'cfg' in dm
    assert 'cfg/base' in dm
    assert 'cfg/meta' in dm
    assert 'cfg/model' in dm
    assert 'cfg/run' in dm

    assert len(dm['uni']) == 1
    assert 'uni' in dm
    assert 0 in dm['uni']
    uni = dm['uni'][0]
    
    # Check that the uni config is loaded
    assert 'cfg' in uni

    # Check that the binary data is loaded as expected
    assert 'data' in uni
    assert 'data/dummy' in uni

    # Get the state dataset and check its content
    dset = uni['data/dummy/state']

    assert isinstance(dset, udc.NumpyDC)
    assert dset.shape == (uni['cfg']['num_steps'] + 1, 1000)
    assert np.issubdtype(dset.dtype, float)

def test_load_sweep(dm_after_sweep):
    """Tests the loading of simulation data for a sweep"""
    dm = dm_after_sweep

    # Load and print a tree of the loaded data
    dm.load_from_cfg(print_tree=True)

    # Check that the config is loaded as expected
    assert 'cfg' in dm
    assert 'cfg/base' in dm
    assert 'cfg/meta' in dm
    assert 'cfg/model' in dm
    assert 'cfg/run' in dm

    for uni_no, uni in dm['uni'].items():
        # Check that the uni config is loaded
        assert 'cfg' in uni

        # Check that the binary data is loaded as expected
        assert 'data' in uni
        assert 'data/dummy' in uni
        assert 'data/dummy/state' in uni
    
        # Get the state dataset and check its content
        dset = uni['data/dummy/state']

        assert isinstance(dset, udc.NumpyDC)
        assert dset.shape == (uni['cfg']['num_steps'] + 1, 1000)
        assert np.issubdtype(dset.dtype, float)

def test_MultiverseGroup(dm_after_single):
    """Tests the MultiverseGroup"""
    dm = dm_after_single

    # Load and print a tree of the loaded data
    dm.load_from_cfg(print_tree=True)

    # Check integer acccess
    assert 0 in dm['uni']

    # Bad integer keys should not work
    with pytest.raises(KeyError, match="No universe with ID 42 available"):
        dm['uni'][42]
    
    with pytest.raises(KeyError, match="Universe IDs cannot be negative!"):
        dm['uni'][-1]
