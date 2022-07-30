"""Tests of the output of the SimpleFlocking model"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

# Configure the ModelTest class
mtc = ModelTest("SimpleFlocking", test_file=__file__)


# Fixtures --------------------------------------------------------------------
# Define fixtures


# Tests -----------------------------------------------------------------------


def test_flocking():
    """Tests that the flocking scenario leads to uniform directions"""
    mv, dm = mtc.create_run_load(
        from_cfg_set="flocking",
        parameter_space=dict(
            num_steps=5000,
            write_every=100,
            SimpleFlocking=dict(noise_level=0.0),
        ),
    )

    for uni in dm["multiverse"].values():
        data = uni["data/SimpleFlocking"]

        orientation_circstd = data["orientation_circstd"]
        print(orientation_circstd.data)
        assert orientation_circstd.isel(time=-1) < 0.1
