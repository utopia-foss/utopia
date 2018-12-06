"""Tools that help testing models"""

import os
import logging
from tempfile import TemporaryDirectory
from typing import Union, Tuple

import py
from pkg_resources import resource_filename

from utopya import Multiverse, DataManager
from utopya.info import MODELS

# Get a logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

class ModelTest:
    """A class to use for testing Utopia models.

    It attaches to a certain model and makes it easy to load config files with
    which test should be carried out.
    """

    def __init__(self, model_name: str, *, test_file: str=None, sim_errors: str='raise'):
        """Initialize the ModelTest for the given model name
        
        Args:
            model_name (str): Name of the model to test
            test_file (str): The file this ModelTest is used in. If given,
                will look for config files relative to the folder this file is
                located in.
            sim_errors (str, optional): Whether to raise errors from Multiverse
        
        Raises:
            ValueError: If the directory extracted from test_file is invalid
        """
        # Store model name
        self._model_name = None
        self.model_name = model_name

        # Check if a test_file was given to use for determining the test_dir
        if test_file:
            test_dir = py.path.local(os.path.dirname(test_file))
            if not test_dir.exists() or not test_dir.isdir():
                raise ValueError("Could not extract a valid directory path "
                                 "from the given `test_file` '{}'. Does it "
                                 "exist?".format(test_file))
            self._test_dir = test_dir

        else:
            self._test_dir = None

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
        if self.model_name:
            raise RuntimeError("A ModelTest's associated model cannot be "
                               "changed!")
        
        elif model_name not in MODELS:
            raise ValueError("No such model '{}' available.\n"
                             "Available models: {}"
                             "".format(model_name,
                                       ", ".join(MODELS.keys())))
        
        # All checks ok, set value
        self._model_name = model_name
        log.debug("Set model_name:  %s", model_name)

    @property
    def test_dir(self) -> Union[py.path.local, None]:
        """Returns the path to the test directory (as py.path.local object)"""
        return self._test_dir


    # Public methods ..........................................................

    def get_file_path(self, rel_path: str) -> str:
        """Returns the absolute path of the file that can be found relative to
        the test directory of this ModelTest class instance.
        
        Args:
            rel_path (str): The relative path of the file
        
        Returns:
            str: The absolute path to the file
        
        Raises:
            FileNotFoundError: If the desired file could not be found
            ValueError: If no test_file was given during initialisation
        """
        if not self.test_dir:
            raise ValueError("No test directory associated with this "
                             "ModelTest; pass one during initialization to be "
                             "able to generate absolute paths.")

        if not os.path.isabs(rel_path):
            abs_path = self.test_dir.join(rel_path)
        else:
            abs_path = py.path.local(rel_path)

        if not abs_path.exists() or not abs_path.isfile():
            raise FileNotFoundError("No file '{}' found relative to test "
                                    "directory! Expected a file at: {}"
                                    "".format(rel_path, abs_path))

        return str(abs_path)

    def create_mv(self, *, from_cfg: str=None, run_cfg_path: str=None, **update_meta_cfg) -> Multiverse:
        """Creates a Multiverse for this model.
        
        Args:
            from_cfg (str, optional): The name of the config file (relative to
                the test directory) to be used.
            run_cfg_path (str, optional): The path of the run_cfg to use. Can
                not be passed if from_cfg argument evaluates to True.
            **update_meta_cfg: Can be used to update the meta configuration
        
        Returns:
            Multiverse: The created Multiverse object
        """

        # Check arguments
        if from_cfg and run_cfg_path:
            raise ValueError("Can only pass either argument `from_cfg` OR "
                             "`run_cfg_path`, but got both!")

        elif from_cfg:
            # Use the config file in the test directory as run_cfg_path
            run_cfg_path = self.get_file_path(from_cfg)


        # Use update_meta_cfg to pass a unique temporary directory as out_dir
        tmpdir = TemporaryDirectory(prefix=self.model_name,
                                    suffix="_mv{}".format(len(self._mvs)))
        
        if 'paths' not in update_meta_cfg:
            update_meta_cfg['paths'] = dict(out_dir=tmpdir.name)
        else:
            update_meta_cfg['paths']['out_dir'] = tmpdir.name

        # Also set the exit handling value, if not already set
        _se = self._sim_errors

        if 'worker_manager' not in update_meta_cfg:
            update_meta_cfg['worker_manager'] = dict(nonzero_exit_handling=_se)

        elif 'nonzero_exit_handling' not in update_meta_cfg['worker_manager']:
            update_meta_cfg['worker_manager']['nonzero_exit_handling'] = _se

        # else: entry was already set; don't set it again

        # Create the Multiverse
        mv = Multiverse(model_name=self.model_name,
                        run_cfg_path=run_cfg_path,
                        **update_meta_cfg)

        # Store it, then return
        self._store_mv(mv, out_dir=tmpdir)
        return mv

    def create_run_load(self, *, from_cfg: str=None, run_cfg_path: str=None, print_tree: bool=True, **update_meta_cfg) -> Tuple[Multiverse, DataManager]:
        """Chains the create_mv, mv.run, and mv.dm.load_from_cfg
        methods together and returns a (Multiverse, DataManager) tuple.
        
        Args:
            from_cfg (str, optional): The name of the config file (relative to
                the test directory) to be used.
            run_cfg_path (str, optional): The path of the run_cfg to use. Can
                not be passed if from_cfg argument evaluates to True.
            print_tree (bool, optional): Whether to print the loaded data tree
            **update_meta_cfg: Arguments passed to the create_mv function
        
        Raises:
            ValueError: If both from_cfg and run_cfg_path were given
        """

        # Create Multiverse
        mv = self.create_mv(from_cfg=from_cfg,
                            run_cfg_path=run_cfg_path,
                            **update_meta_cfg)

        # Run the simulation
        mv.run()

        # Now, load the data into the data manager
        mv.dm.load_from_cfg(print_tree=print_tree)

        return mv, mv.dm


    # Private methods .........................................................

    def _store_mv(self, mv: Multiverse, **kwargs) -> None:
        """Stores a created Multiverse object and all the kwargs in a dict"""
        self._mvs.append(dict(mv=mv, **kwargs)) 
