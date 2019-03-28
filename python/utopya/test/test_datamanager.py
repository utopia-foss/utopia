"""Tests the UtopiaDataManager and involved functions and classes."""

import os
import uuid
from pkg_resources import resource_filename

import numpy as np

import pytest

from paramspace import ParamSpace

from utopya import Multiverse, DataManager
import utopya.datagroup as udg
import utopya.datacontainer as udc

# Local constants
RUN_CFG_PATH = resource_filename('test', 'cfg/run_cfg.yml')
SWEEP_CFG_PATH = resource_filename('test', 'cfg/sweep_cfg.yml')
LARGE_SWEEP_CFG_PATH = resource_filename('test', 'cfg/large_sweep_cfg.yml')

# Fixtures --------------------------------------------------------------------

@pytest.fixture
def mv_kwargs(tmpdir) -> dict:
    """Returns a dict that can be passed to Multiverse for initialisation.

    This uses the `tmpdir` fixture provided by pytest, which creates a unique
    temporary directory that is removed after the tests ran through.
    """
    # Create a random string to use as model note
    rand_str = "test_" + uuid.uuid4().hex

    # Create a dict that specifies a unique testing path.
    return dict(model_name='dummy',
                run_cfg_path=RUN_CFG_PATH,
                user_cfg_path=False,  # to omit the user config
                paths=dict(out_dir=str(tmpdir), model_note=rand_str)
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

@pytest.fixture
def dm_after_large_sweep(mv_kwargs) -> DataManager:
    """Initialises a Multiverse with a DataManager, runs a simulation with
    output going into a temporary directory, then returns the DataManager."""
    # Initialise the Multiverse
    mv_kwargs['run_cfg_path'] = LARGE_SWEEP_CFG_PATH
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

    # Check that 'multiverse' is a MultiverseGroup
    assert 'multiverse' in dm
    assert isinstance(dm['multiverse'], udg.MultiverseGroup)
    assert isinstance(dm['multiverse'].pspace, ParamSpace)

    assert len(dm['multiverse']) == 1
    assert 0 in dm['multiverse']
    uni = dm['multiverse'][0]
    
    # Check that the uni config is loaded
    assert 'cfg' in uni

    # Check that the binary data is loaded as expected
    assert 'data' in uni
    assert 'data/dummy' in uni

    # Get the state dataset and check its content
    dset = uni['data/dummy/state']
    print(dset.data)

    assert isinstance(dset, (udc.NumpyDC, udc.XarrayDC))
    assert dset.shape[1] == 1000
    assert np.issubdtype(dset.dtype, float)

    # Test other configured capabilities
    # write_every -> only every write_every step should have been written
    write_every = int(uni['data/dummy'].attrs['write_every'])
    assert write_every == uni['cfg']['write_every']
    assert dset.shape[0] == (uni['cfg']['num_steps'] // write_every) + 1


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

    # Check that 'multiverse' is a MultiverseGroup of right length
    assert 'multiverse' in dm
    assert isinstance(dm['multiverse'], udg.MultiverseGroup)
    assert isinstance(dm['multiverse'].pspace, ParamSpace)
    assert 0 not in dm['multiverse']
    assert len(dm['multiverse']) == dm['multiverse'].pspace.volume

    # Now go over all available universes
    for uni_no, uni in dm['multiverse'].items():
        # Check that the uni config is loaded
        assert 'cfg' in uni

        # Check that the binary data is loaded as expected
        assert 'data' in uni
        assert 'data/dummy' in uni
        assert 'data/dummy/state' in uni
    
        # Get the state dataset and check its content
        dset = uni['data/dummy/state']

        assert isinstance(dset, (udc.NumpyDC, udc.XarrayDC))
        assert dset.shape == (uni['cfg']['num_steps'] + 1, 1000)
        assert np.issubdtype(dset.dtype, float)
