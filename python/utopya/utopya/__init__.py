"""The utopya package implements the frontend of Utopia."""

import os
import logging
import coloredlogs

# Configure the logging module
logging.basicConfig(level=logging.INFO)

# Add colour logging to the root logger
log = logging.getLogger(__name__)
coloredlogs.install(logger=log,
                    fmt="%(levelname)-8s %(module)-15s %(message)s")

# Specify the version
__version__ = '0.4.0'
# NOTE This needs to correspond to the one in setup.py
# TODO single-source this!


# Define or import some global variables ......................................

# The global model registry object
from .model_registry import MODELS


# Import classes that should be easily accessible .............................
from .multiverse import Multiverse, FrozenMultiverse
from .datamanager import DataManager
from .datagroup import UniverseGroup, MultiverseGroup
from .model import Model
