"""Tests initialization of the SandPile model"""

import numpy as np

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class for SandPile
mtc = ModelTest("SandPile", test_file=__file__)

# Fixtures --------------------------------------------------------------------
# Define fixtures

def model_cfg(**params) -> dict:
    """Helper method to create a model configuration"""
    return dict(parameter_space=dict(SandPile=dict(**params)))

def cm_cfg(**params) -> dict:
    """Helper method to create a cell_manager configuration"""
    return model_cfg(cell_manager=dict(**params))

# Tests -----------------------------------------------------------------------

def test_initialization(): 
    """Test that initialization is correct, i.e. that all expected datasets
    are created and all cells topple in the initial time step
    """
    # Gather a list of data managers to check
    dms = []

    # Basic configuration
    _, dm = mtc.create_run_load(from_cfg="init.yml", perform_sweep=True)
    dms.append(dm)

    # Should also work with other critical slope values
    mv, dm = mtc.create_run_load(from_cfg="init.yml", perform_sweep=False,
                                 **model_cfg(critical_slope=10))
    
    # Or a different neighborhood ...
    # FIXME Does not currently work
    # mv, dm = mtc.create_run_load(from_cfg="init.yml", perform_sweep=False,
    #                              **cm_cfg(neighborhood=dict(mode="Moore")))

    # For each data manager and universe, iterate over the output data and test
    for i, dm in enumerate(dms):
        for uni_no, uni in dm['multiverse'].items():
            # Get the data
            data = uni['data/SandPile']

            # Get the config of this universe
            uni_cfg = uni['cfg']
            print("\nUniverse configration:", uni_cfg)

            # Get the critical slope value
            critical_slope = uni_cfg['SandPile']['critical_slope']

            # Check that all expected datasets are available
            assert 'slope' in data
            assert 'avalanche' in data
            assert 'avalanche_size' in data

            # Check that only a single time step is written
            assert data['slope'].time.size == 1
            assert data['avalanche'].time.size == 1
            assert data['avalanche_size'].shape == (1,)

            # Check that all cells topple in the initial step
            num_cells = data['avalanche'].x.size * data['avalanche'].y.size
            assert np.all(data['slope'] <= critical_slope)
            assert np.all(data['avalanche'])
            assert data['avalanche_size'][0] == num_cells

    # For sub-critical initialization, there is no toppling
    _, dm = mtc.create_run_load(from_cfg="init.yml", perform_sweep=True,
                                **cm_cfg(
                                    cell_params=dict(
                                        initial_slope_lower_limit=0,
                                        initial_slope_upper_limit=2)
                                    )
                                )
    
    for uni_no, uni in dm['multiverse'].items():
        # Get the data
        data = uni['data/SandPile']

        # Get the config of this universe
        uni_cfg = uni['cfg']
        print("\nUniverse configration:", uni_cfg)

        # Get the critical slope value
        critical_slope = uni_cfg['SandPile']['critical_slope']

        # Check that only one cell is in avalanche
        num_cells = data['avalanche'].x.size * data['avalanche'].y.size
        assert np.all(data['slope'] < critical_slope)
        assert data['avalanche'].sum() == 1
        assert data['avalanche_size'][0] == 1


def test_invalid_parameters():
    """Make sure invalid arguments lead to an error"""

    # Bad initial slope values limits
    with pytest.raises(SystemExit):
        mtc.create_run_load(from_cfg="init.yml", perform_sweep=False,
                            **cm_cfg(
                                cell_params=dict(
                                    initial_slope_lower_limit=6,
                                    initial_slope_upper_limit=5)
                                )
                            )
    
    with pytest.raises(SystemExit):
        mtc.create_run_load(from_cfg="init.yml", perform_sweep=False,
                            **cm_cfg(
                                cell_params=dict(
                                    initial_slope_lower_limit=5,
                                    initial_slope_upper_limit=5)
                                )
                            )

    # Bad neighborhood mode
    with pytest.raises(SystemExit):
        mtc.create_run_load(from_cfg="init.yml", perform_sweep=False,
                            **cm_cfg(neighborhood=dict(mode="Moore")))
