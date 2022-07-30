"""Tests of the parameter functions of the Environement model"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class for Environment
mtc = ModelTest("Environment", test_file=__file__)

# Fixtures --------------------------------------------------------------------
# Define fixtures


# Tests -----------------------------------------------------------------------


def test_increment():
    """Tests that the output from 'epf_increment' is correct"""
    _, dm = mtc.create_run_load(from_cfg="param_increment.yml")

    for uni_no, uni in dm["multiverse"].items():
        data = uni["data"]["Environment"]["some_global_parameter"]
        cfg = uni["cfg"]["Environment"]

        assert np.isclose(data.sel({"time": 0}), 1)
        assert np.isclose(data.sel({"time": 1}), 3)
        assert np.isclose(data.sel({"time": 2}), 5)
        assert np.isclose(data.sel({"time": 3}), 7)


def test_random():
    """Tests that the output from 'epf_random' is correct"""
    _, dm = mtc.create_run_load(from_cfg="param_random.yml")

    for uni_no, uni in dm["multiverse"].items():
        data = uni["data"]["Environment"]["some_global_parameter"]
        cfg = uni["cfg"]["Environment"]

        # normal: mean 0, stddev 1
        assert np.isclose(
            data.sel(time=slice(0, 999)).mean(dim=["time"]), [0.0], atol=5.0e-2
        ).all()
        assert np.isclose(
            data.sel(time=slice(0, 999)).std(dim=["time"]), [1.0], atol=5.0e-2
        ).all()

        # poisson mean 10
        assert np.isclose(
            data.sel(time=slice(1000, 1999)).mean(dim=["time"]),
            [10.0],
            rtol=0.1,
        ).all()

        # exponential lambda 10
        assert np.isclose(
            data.sel(time=slice(2000, 2999)).mean(dim=["time"]),
            [0.1],
            atol=5.0e-2,
        ).all()
        assert np.isclose(
            data.sel(time=slice(2000, 2999)).std(dim=["time"]),
            [0.1],
            atol=5.0e-2,
        ).all()

        # uniform_int interval [0,10]
        assert np.isclose(
            data.sel(time=slice(3000, 3999)).mean(dim=["time"]),
            [5],
            atol=5.0e-2,
        ).all()
        assert np.isclose(
            data.sel(time=slice(3000, 3999)).min(dim=["time"]), [0], atol=0
        ).all()
        assert np.isclose(
            data.sel(time=slice(3000, 3999)).max(dim=["time"]), [10], atol=0
        ).all()

        # uniform_real interval [0, 1.]
        assert np.isclose(
            data.sel(time=slice(4000, 4999)).mean(dim=["time"]),
            [0.5],
            atol=5.0e-2,
        ).all()
        assert np.isclose(
            data.sel(time=slice(4000, 4999)).min(dim=["time"]),
            [0.0],
            atol=5.0e-2,
        ).all()
        assert np.isclose(
            data.sel(time=slice(4000, 4999)).max(dim=["time"]),
            [1.0],
            atol=5.0e-2,
        ).all()


def test_rectangular():
    """Tests that the output from 'epf_rectangular' is correct"""
    _, dm = mtc.create_run_load(from_cfg="param_rectangular.yml")

    for uni_no, uni in dm["multiverse"].items():
        data = uni["data"]["Environment"]["some_global_parameter"]
        cfg = uni["cfg"]["Environment"]

        assert np.isclose(data.sel(time=slice(0, 4)), 2).all()
        assert np.isclose(data.sel(time=slice(5, 9)), 1).all()
        assert np.isclose(data.sel(time=slice(10, 14)), 2).all()
        assert np.isclose(data.sel(time=slice(15, 19)), 1).all()

        assert np.isclose(data.sel(time=slice(20, 24)), 1).all()
        assert np.isclose(data.sel(time=slice(25, 29)), 0).all()
        assert np.isclose(data.sel(time=slice(30, 34)), 1).all()
        assert np.isclose(data.sel(time=slice(35, 39)), 0).all()

        assert np.isclose(data.sel(time=slice(45, 49)), 0).all()
        assert np.isclose(data.sel(time=slice(50, 54)), 1).all()
        assert np.isclose(data.sel(time=slice(55, 59)), 0).all()
        assert np.isclose(data.sel(time=slice(60, 64)), 1).all()

        assert np.isclose(data.sel(time=slice(65, 66)), 1).all()
        assert np.isclose(data.sel(time=slice(67, 69)), 0).all()
        assert np.isclose(data.sel(time=slice(75, 76)), 1).all()
        assert np.isclose(data.sel(time=slice(77, 79)), 0).all()


def test_set():
    """Tests that the output from 'epf_set' is correct"""
    _, dm = mtc.create_run_load(from_cfg="param_set.yml")

    for uni_no, uni in dm["multiverse"].items():
        data = uni["data"]["Environment"]["some_global_parameter"]
        cfg = uni["cfg"]["Environment"]

        assert np.isclose(data.sel(time=slice(0, 2)), 1).all()
        assert np.isclose(data.sel(time=slice(3, 5)), 2).all()


def test_sinusoidal():
    """Tests that the output from 'epf_sinusoidal' is correct"""
    _, dm = mtc.create_run_load(from_cfg="param_sinusoidal.yml")

    for uni_no, uni in dm["multiverse"].items():
        data = uni["data"]["Environment"]["some_global_parameter"]
        cfg = uni["cfg"]["Environment"]

        assert np.isclose(data.sel(time=[0, 50, 100, 150]), 0).all()
        assert (data.sel(time=[25, 125]) > 9.9).all()
        assert (data.sel(time=[75, 175]) < -9.9).all()

        assert np.isclose(data.sel(time=[225, 275, 325, 375]), 10).all()
        assert (data.sel(time=[200, 300, 400]) > 19.9).all()
        assert (data.sel(time=[250, 350]) < 0.1).all()
