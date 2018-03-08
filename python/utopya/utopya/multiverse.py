"""Implementation of the Multiverse class.

The Multiverse supplies the main user interface of the frontend.
"""
import os
import time
import copy
import logging
import pkg_resources

from utopya.workermanager import WorkerManager, enqueue_json
from utopya.tools import recursive_update, read_yml, write_yml
from utopya.info import MODELS

log = logging.getLogger(__name__)


class Multiverse:
    """The Multiverse is where everything is orchestrated.
    
    It aims to make all the functionality of the Utopia frontend accessible in
    one place.
    
    Attributes:
        dirs (dict): The absolute paths to the managed directories
        model_name (str): The model name associated with this Multiverse
        UTOPIA_EXEC (str): The name of the utopia executable, found in 
    """

    UTOPIA_EXEC = "utopia"
    BASE_CFG_PATH = pkg_resources.resource_filename('utopya',
                                                    'cfg/base_cfg.yml')
    USER_CFG_SEARCH_PATH = "~/.config/utopia/user_cfg.yml"

    def __init__(self, *, model_name: str, run_cfg_path: str, user_cfg_path: str=None, update_meta_cfg: dict=None):
        """Initialize the Multiverse.
        
        Args:
            model_name (str): A valid name of Utopia model
            run_cfg_path (str): The path to the run configuration.
            user_cfg_path (str, optional): If given, this is used to update the
                base configuration. If None, will look for it in the default
                path, see Multiverse.USER_CFG_SEARCH_PATH.
            update_meta_cfg (dict, optional): Can be used to update the meta
                config that is generated from the run_cfg and user_cfg.
        """
        # Initialize empty attributes (partly property-managed)
        self._model_name = None
        self._dirs = {}

        # Set the model name
        self.model_name = model_name

        # Initialise meta config with None
        self._meta_config = None

        # Create Meta Config
        self.meta_config = self._create_meta_config(run_cfg_path=run_cfg_path, user_cfg_path=user_cfg_path, update_meta_cfg=update_meta_cfg)

        # Create the run directory and write the meta configuration into it
        self._create_run_dir(model_name=self.model_name,
                             **self.meta_config['paths'])

        # create a WorkerManager instance
        self._wm = WorkerManager(**self.meta_config['worker_manager'])

        log.info("Initialized Multiverse for model: '%s'", self.model_name)

    # Properties ..............................................................

    @property
    def model_name(self) -> str:
        """The model name associated with this Multiverse"""
        return self._model_name

    @model_name.setter
    def model_name(self, model_name: str):
        """Checks if the model name is valid, then sets it and makes it read-only."""
        if model_name not in MODELS:
            raise ValueError("No such model '{}' available.\n"
                             "Available models: {}"
                             "".format(model_name, ", ".join(MODELS.keys())))
        
        elif self.model_name:
            raise RuntimeError("A Multiverse's associated model cannot be "
                               "changed!")

        else:
            self._model_name = model_name
            log.debug("Set model_name:  %s", model_name)

    @property
    def meta_config(self) -> dict:
        """The meta configuration."""
        return self._meta_config

    @meta_config.setter
    def meta_config(self, d: dict) -> None:
        """Set the meta configuration dict."""
        if self._meta_config:
            raise RuntimeError("Metaconfig can only be set once.")

        elif not isinstance(d, dict):
            raise TypeError("Can only interpret dictionary input for"
                            " Metaconfig but {} was given".format(type(d)))
        else:
            self._meta_config = d

    @property
    def dirs(self) -> dict:
        """Information on managed directories."""
        return self._dirs

    @property
    def wm(self) -> WorkerManager:
        """The Multiverse's WorkerManager."""
        return self._wm

    # Public methods ..........................................................

    def run(self):
        """Starts a Utopia run. Whether this will be a single simulation or
        a Parameter sweep is decided by the contents of the meta_cfg."""
        log.info("Preparing to run Utopia ...")

        # Depending on the configuration, the corresponding methods can already be called.
        if self.meta_config['run_kwargs']['perform_sweep']:
            self.run_sweep()
        else:
            self.run_single()
    
    def run_single(self):
        """Runs a single simulation."""

        # Get the parameter space from the config
        pspace = self.meta_config['parameter_space']
        # NOTE Once the ParamSpace class is used, the defaults need to be retrieved here.
        
        # Add the task to the worker manager.
        log.info("Adding task for the single simulation ...")
        self._add_sim_task(uni_id=0, max_uni_id=0, cfg_dict=pspace)

        # Tell the WorkerManager to start working, which will be the blocking call
        log.info("Starting to work now ...")
        self.wm.start_working()

        log.info("Finished single simulation run.")

    def run_sweep(self):
        """Runs a parameter sweep."""
        raise NotImplementedError

    # "Private" methods .......................................................

    def _create_meta_config(self, *, run_cfg_path: str, user_cfg_path: str=None, update_meta_cfg: dict=None) -> dict:
        """Read base configuration file and adjust parameters.
        
        The base_config file, the user_config file (if existing) and the run_config file are read in.
        The base_config is adjusted accordingly to create the meta_config.
        
        The final configuration dict is built from three components:
            1. The base is the default configuration, which is always present
            2. If a userconfig is present, this recursively updates the defaults
            3. Then, the given metaconfig recursively updates the created dict
            4. As a last step, the update_meta_cfg dict will be applied
        
        Args:
            run_cfg_path (str): path to run_config
            user_cfg_path (str, optional): path to the user_config file
            update_meta_cfg (dict, optional): will be
        
        Returns:
            dict: returns the created metaconfig that will be saved as attr
        """
        # Read in all the yaml files from their paths
        log.debug("Reading in configuration files ...")

        base_cfg = read_yml(self.BASE_CFG_PATH,
                            error_msg="Base configuration not found!")

        if user_cfg_path is not None:
            user_cfg = read_yml(user_cfg_path,
                                error_msg="{0} was given but user_config.yaml "
                                          "could not be found."
                                          "".format(user_cfg_path))

        run_cfg = read_yml(run_cfg_path,
                           error_msg="{0} was given but run_config could "
                                     "not be found.".format(run_cfg_path))

        # After this point it is assumed that all values are valid
        # Those keys or values will throw errors once they are needed ...

        # Now perform the recursive update steps
        meta_tmp = base_cfg

        if user_cfg_path is not None:  # update default with user spec
            log.debug("Updating configuration with user configuration ...")
            meta_tmp = recursive_update(meta_tmp, user_cfg)

        # And now recursively update with the run config
        log.debug("Updating configuration with run configuration ...")
        meta_tmp = recursive_update(meta_tmp, run_cfg)

        # ... and the update_meta_cfg dictionary
        if update_meta_cfg:
            log.debug("Updating configuration with given `update_meta_cfg`")
            meta_tmp = recursive_update(meta_tmp,
                                        copy.deepcopy(update_meta_cfg))
            # NOTE using copy to make sure that usage of the dict will not interfere with the Multiverse's meta config

        log.info("Loaded meta configuration.")
        return meta_tmp

    def _create_run_dir(self, *, model_name: str, out_dir: str, model_note: str=None) -> None:
        """Create the folder structure for the run output.

        This will also write the meta config to the corresponding config
        directory.

        The following folder tree will be created
        utopia_output/   # all utopia output should go here
            model_a/
                180301-125410_my_model_note/
                    config/
                    eval/
                    universes/
                        uni000/
                        uni001/
                        ...
            model_b/
                180301-125412_my_first_sim/
                180301-125413_my_second_sim/


        Args:
            model_name (str): Description
            out_dir (str): Description
            model_note (str, optional): Description

        Raises:
            RuntimeError: If the simulation directory already existed. This
                should not occur, as the timestamp is unique. If it occurs,
                something is seriously wrong. Or you are in a strange time
                zone.
        """ 
        # Create the folder path to the simulation directory
        log.debug("Creating path for run directory inside %s ...", out_dir)
        out_dir = os.path.expanduser(out_dir)
        run_dir = os.path.join(out_dir,
                               model_name,
                               time.strftime("%Y%m%d-%H%M%S"))

        # Append a model note, if needed
        if model_note:
            run_dir += "_" + model_note

        # Inform and store to directory dict
        log.debug("Expanded user and time stamp to %s", run_dir)
        self._dirs['run_dir'] = run_dir

        # Recursively create the whole path to the simulation directory
        try:
            os.makedirs(run_dir)
        except OSError as err:
            raise RuntimeError("Simulation directory already exists. This "
                               "should not have happened. Try to start the "
                               "simulation again.") from err

        # Make subfolders
        for subdir in ('config', 'eval', 'universes'):
            subdir_path = os.path.join(run_dir, subdir)
            os.mkdir(subdir_path)
            self._dirs[subdir] = subdir_path

        log.debug("Finished creating simulation directory. Now registered: %s",
                  self._dirs)

        # Write the meta config to the config directory.
        write_yml(d=self.meta_config,
                  path=os.path.join(self.dirs['config'], "meta_cfg.yml"))
        log.debug("Wrote meta configuration to config directory.")

    def _create_uni_dir(self, *, uni_id: int, max_uni_id: int) -> str:
        """The _create_uni_dir generates the folder for a single universe.

        Within the universes directory, create a subdirectory uni### for the
        given universe number, zero-padded such that they are sortable.

        Args:
            uni_id (int): ID of the universe whose folder should be created.
                Needs to be positive or zero.
            max_uni_id (int): highest ID, needed for correct zero-padding.
                Needs to be larger or equal to uni_id.
        """
        # Check if uni_id and max_uni_id are positive
        if uni_id < 0 or uni_id > max_uni_id:
            raise RuntimeError("Input variables don't match prerequisites: "
                               "uni_id >= 0, max_uni_id >= uni_id. Given arguments: "
                               "uni_id: {}, max_uni_id: {}".format(uni_id, max_uni_id))

        # Use a format string for creating the uni_path
        fstr = "uni{id:>0{digits:}d}"
        uni_path = os.path.join(self._dirs['universes'],
                                fstr.format(id=uni_id, digits=len(str(max_uni_id))))

        # Now create the folder
        os.mkdir(uni_path)
        log.debug("Created universe path: %s", uni_path)
        return uni_path

    def _add_sim_task(self, *, uni_id: int, max_uni_id: int, cfg_dict: dict) -> None:
        """Helper function that handles task assignment to the WorkerManager.

        This function performs the following steps:
            - Creating a universe (folder) for the task (simulation).
            - Writing the passed-over configuration to a yaml file
            - Passing a functional setup_func and its arguments to WorkerManager.add_task

        Note that the task will not be executed right away but is only
        registered with the WorkerManager. The task will be worker on once the
        WorkerManager has free workers available. That is the reason why the
        setup function is only passed as a functional, not called here.

        Args:
            uni_id (int): ID of the universe whose folder should be created
            max_uni_id (int): highest ID, needed for correct zero-padding
            cfg_dict (dict): given by ParamSpace. Defines how many simulations
                should be started
        """
        # Define the function that will setup everything needed for a universe
        def setup_universe(*, worker_kwargs: dict, utopia_exec: str, model_name: str, uni_id: int, max_uni_id: int, cfg_dict: dict) -> dict:
            """Sub-helper function to be returned as functional.

            Creates universe for the task, writes configuration, calls
            WorkerManager.add_task.

            Args:
                utopia_exec (str): class constant for utopia executable
                model_name (str): name of the model *derpface*
                uni_id (int): ID of the universe whose folder should be created
                max_uni_id (int): highest ID, needed for correct zero-padding
                cfg_dict (dict): given by ParamSpace. Defines how many simulations
                    should be started

            Returns:
                dict: kwargs for the process to be run when task is grabbed by
                    Worker.
            """
            # create universe directory
            uni_dir = self._create_uni_dir(uni_id=uni_id,
                                           max_uni_id=max_uni_id)

            # write essential part of config to file:
            uni_cfg_path = os.path.join(uni_dir, "config.yml")
            write_yml(d=cfg_dict, path=uni_cfg_path)

            # building args tuple for task assignment
            # assuming there exists an attribute for the executable and for the
            # model
            args = (utopia_exec, model_name, uni_cfg_path)

            # Overwrite the worker kwargs argument with totally new ones
            worker_kwargs = dict(args=args,  # passing the arguments
                                 read_stdout=True,
                                 line_read_func=enqueue_json)  # Callable
            return worker_kwargs

        # Create the dict that will be passed as arguments to setup_universe
        setup_kwargs = dict(utopia_exec=self.UTOPIA_EXEC,
                            model_name=self.model_name,
                            uni_id=uni_id, max_uni_id=max_uni_id,
                            cfg_dict=cfg_dict)

        # Add a task to the worker manager
        self.wm.add_task(priority=None,
                         setup_func=setup_universe,
                         setup_kwargs=setup_kwargs)

        log.debug("Added simulation task for universe %d.", uni_id)