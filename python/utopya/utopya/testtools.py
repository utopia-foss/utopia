"""Tools that help testing models"""

import os
import logging
from tempfile import TemporaryDirectory

import py
from pkg_resources import resource_filename

from utopya import Multiverse
from utopya.info import MODELS

# Get a logger
log = logging.getLogger(__name__)

# The model test class --------------------------------------------------------

class ModelTest:
    """A class to use for testing Utopia models.

    It attaches to a certain model and makes it easy to load config files with
    which test should be carried out.
    """

    def __init__(self, model_name: str, sim_errors: str='raise'):
        """Initialize the ModelTest for the given model name
        
        Args:
            model_name (str): Name of the model to test
            sim_errors (str, optional): Whether to raise errors from Multiverse
        
        Raises:
            ValueError: If no test directory exists corresponding to model_name
        """
        # Store model name
        self._model_name = None
        self.model_name = model_name

        # Find the test directory corresponding to this model
        # TODO find a better way than to hard-code `model_tests` here
        test_dir = py.path.local(resource_filename("model_tests", model_name))
        if not test_dir.exists() or not test_dir.isdir():
            raise ValueError("No test directory for model '{}' found in "
                             "model_tests! Expected a directory at {}!"
                             "".format(self.model_name, test_dir))

        self._test_dir = test_dir

        # Need a container of Multiverse's and temporary output directories
        # such that they do not go out of scope
        self._mvs = []

        # Store exit handling value
        self._sim_errors = sim_errors


    # Properties ..............................................................

    @property
    def model_name(self) -> str:
        """The name of the model to test"""
        return self._model_name

    @model_name.setter
    def model_name(self, model_name: str):
        """Checks if the model name is valid, then sets it and makes it read-only."""
        if model_name not in MODELS:
            raise ValueError("No such model '{}' available.\n"
                             "Available models: {}"
                             "".format(model_name,
                                       ", ".join(MODELS.keys())))
        
        elif self.model_name:
            raise RuntimeError("A ModelTest's associated model cannot be "
                               "changed!")

        self._model_name = model_name
        log.debug("Set model_name:  %s", model_name)

    @property
    def test_dir(self) -> py.path.local:
        """Returns the path to the test directory (as py.path.local object)"""
        return self._test_dir


    # Public methods ..........................................................

    def get_cfg_by_name(self, cfg_name: str) -> str:
        """Returns the path of the *.yml config file with the given name from
        the test directory
        
        Args:
            cfg_name (str): The name of the config file to look for in the
                test directory. Required extension is *.yml!
        
        Returns:
            str: The absolute path to the config file
        
        Raises:
            FileNotFoundError: If the desired file could 
        """
        cfg_path = self.test_dir.join(cfg_name+".yml")

        if not cfg_path.exists() and cfg_path.isfile():
            raise FileNotFoundError("No config file found by name '{}' at {}!"
                                    "".format(cfg_name, cfg_path))

        return str(cfg_path)
    
    def create_mv(self, run_cfg_path=None, **update_meta_cfg) -> Multiverse:
        """Creates a Multiverse for this model using the default model config
        
        Args:
            **update_meta_cfg: Can be used to update the meta configuration
        
        Returns:
            Multiverse: The created Multiverse object
        """

        # Use update_meta_cfg to pass a unique temporary directory as out_dir
        tmpdir = TemporaryDirectory(prefix=self.model_name,
                                    suffix="mv_no{}".format(len(self._mvs)))
        
        if 'paths' not in update_meta_cfg:
            update_meta_cfg['paths'] = dict(out_dir=tmpdir.name)
        else:
            update_meta_cfg['paths']['out_dir'] = tmpdir.name

        # Also set the exit handling value
        _se = self._sim_errors
        if 'worker_manager' not in update_meta_cfg:
            update_meta_cfg['worker_manager'] = dict(nonzero_exit_handling=_se)
        else:
            update_meta_cfg['worker_manager']['nonzero_exit_handling'] = _se

        # Create the Multiverse
        mv = Multiverse(model_name=self.model_name,
                        run_cfg_path=None, update_meta_cfg=update_meta_cfg)

        # Store it, then return
        self._store_mv(mv, out_dir=tmpdir)
        return mv
    
    def create_mv_from_cfg(self, cfg_name: str, **update_meta_cfg) -> Multiverse:
        """Creates a Multiverse for this model using a test config file as run
        configuration.
        
        Args:
            cfg_name (str): Name of the *.yml config file to use as the run
                configuration of this Multiverse.
            **update_meta_cfg: Can be used to update the meta configuration
        
        Returns:
            Multiverse: The created Multiverse object
        """
        return self.create_mv(run_cfg_path=self.get_cfg_by_name(cfg_name),
                              update_meta_cfg=update_meta_cfg)

    # Private methods .........................................................

    def _store_mv(self, mv: Multiverse, **kwargs) -> None:
        """Stores a created Multiverse object and all the kwargs in a dict"""
        self._mvs.append(dict(mv=mv, **kwargs)) 
