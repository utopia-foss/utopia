"""The utopya package implements the frontend of Utopia."""

import logging
import coloredlogs

# Configure the logging module
logging.basicConfig(level=logging.INFO)

# Add colour logging to the root logger
log = logging.getLogger(__name__)
coloredlogs.install(logger=log,
                    fmt="%(levelname)-8s %(module)-14s %(message)s")

# Specify the version
__version__ = '0.2.0'
# NOTE This needs to correspond to the one in setup.py

# Import those objects that should be easily accessible when importing utopya
from .multiverse import Multiverse, FrozenMultiverse
from .datamanager import DataManager
from .datagroup import UniverseGroup, MultiverseGroup
from . import info
