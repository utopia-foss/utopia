"""Tests the utopya.batch module"""

from pkg_resources import resource_filename
import pytest

from utopya.tools import load_yml
from utopya.batch import BatchTaskManager

BATCH_FILE_PATH = resource_filename("test", "cfg/batch_file.yml")
BATCH_CFG = load_yml(resource_filename("test", "cfg/batch.yml"))

# -----------------------------------------------------------------------------

