"""This module sets up matplotlib to use in other plotting modules"""

import logging

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

# Import some names from utopya
from .. import DataManager
from ..datagroup import UniverseGroup, MultiverseGroup

# Local constants
log = logging.getLogger(__name__)


# -----------------------------------------------------------------------------
# Can define helper functions here

