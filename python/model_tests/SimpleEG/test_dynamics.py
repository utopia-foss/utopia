"""Tests of the initialisation of the SimpleEG model"""

import numpy as np
import pytest

from utopya.testtools import ModelTest

from .test_init import model_cfg

# Configure the ModelTest class
mtc = ModelTest("SimpleEG", test_file=__file__)

# Fixtures --------------------------------------------------------------------
# Define fixtures here


# Helpers ---------------------------------------------------------------------


# Tests -----------------------------------------------------------------------

def test_nonstatic():
    """Check that a randomly initialized grid generates non-static output."""
