"""Tests the utopya.batch module"""

from pkg_resources import resource_filename
import pytest

from utopya.model import Model
from utopya.tools import load_yml
from utopya.batch import BatchTaskManager

BATCH_FILE_PATH = resource_filename("test", "cfg/batch_file.yml")
BATCH_CFG = load_yml(resource_filename("test", "cfg/batch.yml"))

# -----------------------------------------------------------------------------

def test_batch_file():
    """Tests the BatchTaskManager on the batch file"""
    # Make sure the required models have some output generated
    for model_name in ("dummy",):
        Model(name=model_name).create_mv().run()

    # Can now perform the batch tasks ...
    btm = BatchTaskManager(batch_cfg_path=BATCH_FILE_PATH, debug=True)
    btm.perform_all_tasks()
