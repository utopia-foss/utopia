"""Tests of the state functions of the Environement model"""

import numpy as np

import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class for Environment
mtc = ModelTest("Environment", test_file=__file__)

# Fixtures --------------------------------------------------------------------
# Define fixtures


# Tests -----------------------------------------------------------------------

def test_uniform():
    """Tests that the output from 'uniform' is correct"""
    _, dm = mtc.create_run_load(from_cfg="state_uniform.yml")

    for uni_no, uni in dm['multiverse'].items():
        data = uni['data']['Environment']['some_heterogeneous_parameter']
        cfg = uni['cfg']['Environment']

        true_data = np.empty(data.grid_shape)
        true_data.fill(0.1)
        assert np.isclose(data.isel({'time': 0}), true_data).all()
        true_data.fill(1.0)
        assert np.isclose(data.isel({'time': 1}), true_data).all()
        true_data.fill(1.1)
        assert np.isclose(data.isel({'time': 2}), true_data).all()
        true_data.fill(1.2)
        assert np.isclose(data.isel({'time': 3}), true_data).all()

def test_slope():
    """Tests that the output from 'slope' is correct"""
    _, dm = mtc.create_run_load(from_cfg="state_steps.yml")

    for uni_no, uni in dm['multiverse'].items():
        data = uni['data']['Environment']['some_heterogeneous_parameter']
        cfg = uni['cfg']['Environment']
        
        # value changes for every y value
        true_data = np.empty(data.grid_shape)
        true_data[:,0].fill(0.)
        true_data[:,1].fill(0.)
        true_data[:,2].fill(0.5)
        true_data[:,3].fill(0.5)
        true_data[:,4].fill(1.)
        true_data[:,5].fill(1.)
        assert np.isclose(data.sel({'time': 0}), true_data).all()
        assert np.isclose(data.sel({'time': 1}), true_data*1).all()
        assert np.isclose(data.sel({'time': 2}), true_data*2).all()
        assert np.isclose(data.sel({'time': 3}), true_data*3).all()


def test_noise():
    """Tests that the output from 'noise' is correct"""
    _, dm = mtc.create_run_load(from_cfg="state_noise.yml")

    for uni_no, uni in dm['multiverse'].items():
        data = uni['data']['Environment']['some_heterogeneous_parameter']
        cfg = uni['cfg']['Environment']

        assert np.isclose(data.sel({'time': 0}).mean(dim=['x', 'y']), [1.],
                          atol=1.e-2)
        assert np.isclose(data.sel({'time': 1}).mean(dim=['x', 'y']), [0.5],
                          atol=5.e-2)
        
        assert np.isclose(data.sel({'time': 2}).mean(dim=['x', 'y']), [0.],
                          atol=5.e-2)
        assert np.isclose(data.sel({'time': 2}).std(dim=['x', 'y']), [1.],
                          atol=1.e-2)

        assert np.isclose(data.sel({'time': 3}).mean(dim=['x', 'y']), [10.],
                          atol=5.e-2)
        assert np.isclose(data.sel({'time': 4}).mean(dim=['x', 'y']), [20.],
                          atol=5.e-2)
