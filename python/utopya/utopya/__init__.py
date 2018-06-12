"""The utopya package implements the frontend of Utopia."""

# Configure the logging module for the whole package here
import logging
logging.basicConfig(format="%(levelname)-8s %(module)-14s %(message)s",
                    level=logging.INFO)
log = logging.getLogger(__name__)

# Import those objects that should be easily accessible when importing utopya
from .multiverse import Multiverse
from .datamanager import DataManager
from . import info
