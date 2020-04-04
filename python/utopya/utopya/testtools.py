"""Tools that help testing models.

This mainly supplies the ModelTest class, which is a specialization of the
~:py:class:`utopya.model.Model` for usage in tests.
"""

import os
import logging

from .model import Model

# Get a logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

class ModelTest(Model):
    """A class to use for testing Utopia models.

    It attaches to a certain model and makes it easy to load config files with
    which test should be carried out.
    """

    def __init__(self, model_name: str, *, test_file: str=None,
                 use_tmpdir: bool=True, **kwargs):
        """Initialize the ModelTest class for the given model name.

        This is basically like the base class __init__ just that it sets the
        default value of ``use_tmpdir`` to True and renames TODO

        Args:
            model_name (str): Name of the model to test
            test_file (str, optional): The file this ModelTest is used in. If
                given, will look for config files relative to the folder this
                file is located in.
            use_tmpdir (bool, optional): Whether to use a temporary directory
                to write data to. The default value can be set here; but the
                flag can be overwritten in the create_mv and create_run_load
                methods. For false, the regular model output directory is used.

        Raises:
            ValueError: If the directory extracted from test_file is invalid
        """
        super().__init__(name=model_name,
                         base_dir=(None if test_file is None
                                   else os.path.dirname(test_file)),
                         use_tmpdir=use_tmpdir, **kwargs)
