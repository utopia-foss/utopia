"""The utopya package implements the frontend of Utopia."""

# Configure the logging module for the whole package here
import logging
logging.basicConfig(format="%(levelname)-8s %(module)-14s %(message)s",
                    level=logging.INFO)
log = logging.getLogger(__name__)

# Specify the version
__version__ = '0.1.0-pre.0'
# NOTE This needs to correspond to the one in setup.py

# Import those objects that should be easily accessible when importing utopya
from .multiverse import Multiverse
from .datamanager import DataManager
from . import info
