"""Implementation of the Multiverse class.

The Multiverse supplies the main user interface of the frontend.
"""
import os
import time
import copy
import logging
from shutil import copyfile
import pkg_resources

import paramspace as psp

from utopya.workermanager import WorkerManager
from utopya.task import enqueue_json
from utopya.tools import recursive_update, read_yml, write_yml
from utopya.info import MODELS

# Configure and get logger
log = logging.getLogger(__name__)


class Multiverse:
    """The Multiverse is where everything is orchestrated.
    
    It aims to make all the functionality of the Utopia frontend accessible in
    one place.
    
    Attributes:
        BASE_CFG_PATH (str): The path to the base configuration, supplied with
            the utopya package
        meta_config (dict): The parsed Multiverse meta-configuration. All
            further arguments are extracted from this dict.
        model_name (str): The model name associated with this Multiverse
        USER_CFG_SEARCH_PATH (str): The path at which a user config is expected
    """

    BASE_CFG_PATH = pkg_resources.resource_filename('utopya',
                                                    'cfg/base_cfg.yml')
    USER_CFG_SEARCH_PATH = os.path.expanduser("~/.config/utopia/user_cfg.yml")

    def __init__(self, *, model_name: str, run_cfg_path: str=None, update_meta_cfg: dict=None, user_cfg_path: str=None):
        """Initialize the Multiverse.
        
        Args:
            model_name (str): A valid name of Utopia model
            run_cfg_path (str, optional): The path to the run configuration.
            update_meta_cfg (dict, optional): Can be used to update the meta
                config that is generated from the run_cfg and user_cfg.
            user_cfg_path (str, optional): If given, this is used to update the
                base configuration. If None, will look for it in the default
                path, see Multiverse.USER_CFG_SEARCH_PATH.
        """
        # Initialize empty attributes (partly property-managed)
        self._model_name = None
        self._dirs = {}

        # Set the model name
        self.model_name = model_name

        # Save the model binary path
        self._model_binpath = MODELS[self.model_name]['binpath']
        log.debug("Associated executable of model '%s':\n  %s",
                  self.model_name, self.model_binpath)

        # Create Meta Config
        self._meta_config = None
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
    def model_binpath(self) -> str:
        """The path to this model's binary"""
        return self._model_binpath

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
    # TODO the run methods should only be allowed to run once!

    def run(self):
        """Starts a Utopia run. Whether this will be a single simulation or
        a Parameter sweep is decided by the contents of the meta_cfg."""
        log.info("Preparing to run Multiverse ...")

        # Depending on the configuration, the corresponding methods can already be called.
        if self.meta_config['run_kwargs']['perform_sweep']:
            self.run_sweep()
        else:
            self.run_single()
    
    def run_single(self):
        """Runs a single simulation."""

        # Get the parameter space from the config
        pspace = self.meta_config['parameter_space']
        
        if isinstance(pspace, psp.ParamSpace):
            # Get the default
            log.info("Got a ParamSpace object. Retrieving default point ...")
            uni_cfg = pspace.default
        else:
            uni_cfg = copy.deepcopy(pspace)
        
        # Add the task to the worker manager.
        log.info("Adding task for simulation of a single universe ...")
        self._add_sim_task(uni_id=0, max_uni_id=0, uni_cfg=uni_cfg)

        # Tell the WorkerManager to start working, which will be the blocking call
        self.wm.start_working(forward_streams=True)

        log.info("Finished single universe run. Yay. :)")

    def run_sweep(self):
        """Runs a parameter sweep."""
        
        # Get the parameter space from the config
        pspace = self.meta_config['parameter_space']

        if not isinstance(pspace, psp.ParamSpace):
            raise TypeError("For performing parameter sweeps, the run "
                            "configuration needs to be initialized with a "
                            "ParamSpace object at key `parameter_space`. "
                            "If initializing via a YAML file, make sure to "
                            "have added the `!pspace` tag to that key.")

        log.info("Adding tasks for simulation of %d universes ...",
                 pspace.volume)

        max_uni_id = pspace.volume - 1

        for uni_cfg, uni_id in pspace.all_points(with_info=('state_no',)):
            self._add_sim_task(uni_id=uni_id, max_uni_id=max_uni_id,
                               uni_cfg=uni_cfg)

        log.info("Tasks added.")
        self.wm.start_working(forward_streams=True)

        log.info("Finished Multiverse parameter sweep. Wohoo. :)")

    # "Private" methods .......................................................

    def _create_meta_config(self, *, run_cfg_path: str, user_cfg_path: str, update_meta_cfg: dict) -> dict:
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
            user_cfg_path (str): path to the user_config file
            update_meta_cfg (dict): will be used to update the resulting dict
        
        Returns:
            dict: returns the created metaconfig that will be saved as attr
        """
        log.debug("Reading in configuration files ...")

        # Read in the base configuration (stored inside the package)
        base_cfg = read_yml(self.BASE_CFG_PATH,
                            error_msg="Base configuration not found!")

        # Decide whether to read in the user configuration from the default search location or use a user-passed one
        if user_cfg_path is None:
            log.debug("Looking for user configuration file in default "
                      "location, %s", self.USER_CFG_SEARCH_PATH)

            if os.path.isfile(self.USER_CFG_SEARCH_PATH):
                user_cfg_path = self.USER_CFG_SEARCH_PATH
            else:
                log.debug("No file found at the default search location.")

        elif user_cfg_path is False:
            log.debug("Not loading the user configuration from the default "
                      "search path: %s", self.USER_CFG_SEARCH_PATH)

        if user_cfg_path:
            user_cfg = read_yml(user_cfg_path,
                                error_msg="Did not find user "
                                "configuration at the specified "
                                "path {}!".format(user_cfg_path))
        else:
            user_cfg = None

        # Read in the run configuration
        if run_cfg_path:
            run_cfg = read_yml(run_cfg_path,
                               error_msg="{0} was given but run_config could "
                                         "not be found.".format(run_cfg_path))
        else:
            run_cfg = None

        # After this point it is assumed that all values are valid
        # Those keys or values will throw errors once they are needed ...

        # Now perform the recursive update steps
        meta_tmp = base_cfg

        if user_cfg:  # update default with user spec
            log.debug("Updating configuration with user configuration ...")
            meta_tmp = recursive_update(meta_tmp, user_cfg)

        # And now recursively update with the run config, if available
        if run_cfg:
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

    @staticmethod
    def _create_uni_basename(*, uni_id: int, max_uni_id: int) -> str:
        """Returns the formatted universe basename, zero-padded, for usage
        in WorkerTask names and universe directory creation.
        
        Args:
            uni_id (int): ID of the universe whose folder should be created.
                Needs to be positive or zero.
            max_uni_id (int): highest ID, needed for correct zero-padding.
                Needs to be larger or equal to uni_id.
        
        Returns:
            str: The zero-padded universe basename, e.g. uni0042
        
        Raises:
            ValueError: For invalid `uni_id` or `max_uni_id` arguments 
        """

        # Check if uni_id and max_uni_id are positive
        if uni_id < 0 or uni_id > max_uni_id:
            raise ValueError("Input variables don't match prerequisites: "
                             "uni_id >= 0, max_uni_id >= uni_id. Given "
                             "arguments: uni_id {}, max_uni_id {}"
                             "".format(uni_id, max_uni_id))

        # Use a format string to create the zero-padded universe basename
        return "uni{id:>0{digits:}d}".format(id=uni_id,
                                             digits=len(str(max_uni_id)))

    def _add_sim_task(self, *, uni_id: int, max_uni_id: int, uni_cfg: dict) -> None:
        """Helper function that handles task assignment to the WorkerManager.
        
        This function creates a WorkerTask that will perform the following
        actions __once it is grabbed and worked at__:
          - Create a universe (folder) for the task (simulation)
          - Write that universe's configuration to a yaml file in that folder
          - Create the correct arguments for the call to the model binary
    
        To that end, this method gathers all necessary arguments and registers
        a WorkerTask with the WorkerManager.
        
        Args:
            uni_id (int): ID of the universe whose folder should be created
            max_uni_id (int): highest ID, needed for correct zero-padding
            uni_cfg (dict): given by ParamSpace. Defines how many simulations
                should be started
        """
        def setup_universe(*, worker_kwargs: dict, model_name: str, model_binpath: str, uni_cfg: dict, uni_basename: str) -> dict:
            """The callable that will setup everything needed for a universe.
            
            This is called before the worker process starts working on the universe.
            
            Args:
                worker_kwargs (dict): the current status of the worker_kwargs
                    dictionary; is always passed to a task setup function
                model_name (str): The name of the model
                model_binpath (str): path to the binary to execute
                uni_cfg (dict): the configuration to create a yml file from
                    which is then needed by the model
                uni_basename (str): Basename of the universe to use for folder
                    creation, i.e.: zero-padded universe number, e.g. uni0042
            
            Returns:
                dict: kwargs for the process to be run when task is grabbed by
                    Worker.
            """
            # create universe directory path using the basename
            uni_dir = os.path.join(self.dirs['universes'], uni_basename)

            # Now create the folder
            os.mkdir(uni_dir)
            log.debug("Created universe directory:\n  %s", uni_dir)

            # Generate a path to the output hdf5 file and add it to the dict
            output_path = os.path.join(uni_dir, "data.h5")
            
            # FIXME this should actually not be saved under the key of the model name, but on the highest level of the generated configuration
            if model_name not in uni_cfg:
                # Need to create the high level entry
                uni_cfg[model_name] = dict(output_path=output_path)
            else:
                uni_cfg[model_name]['output_path'] = output_path

            # write essential part of config to file:
            uni_cfg_path = os.path.join(uni_dir, "config.yml")
            write_yml(d=uni_cfg, path=uni_cfg_path)

            # building args tuple for task assignment
            # assuming there exists an attribute for the executable and for the
            # model
            args = (model_binpath, uni_cfg_path)

            # Overwrite the worker kwargs argument with totally new ones
            worker_kwargs = dict(args=args,  # passing the arguments
                                 read_stdout=True,
                                 line_read_func=enqueue_json)  # Callable
            return worker_kwargs

        # Generate the universe basename, which will be used for the folder and the task name
        uni_basename = self._create_uni_basename(uni_id=uni_id,
                                                 max_uni_id=max_uni_id)

        # Create the dict that will be passed as arguments to setup_universe
        setup_kwargs = dict(model_name=self.model_name,
                            model_binpath=self.model_binpath,
                            uni_cfg=uni_cfg,
                            uni_basename=uni_basename)

        # Add a task to the worker manager
        self.wm.add_task(name=uni_basename,
                         priority=None,
                         setup_func=setup_universe,
                         setup_kwargs=setup_kwargs)

        log.debug("Added simulation task for universe %d.", uni_id)


# -----------------------------------------------------------------------------
# Others

def distribute_user_cfg(user_cfg_path: str=Multiverse.USER_CFG_SEARCH_PATH):
    """Distributes a copy of the base config to the user config search path of the Multiverse class."""
    # Get the class constants for the base config
    base_cfg_path = Multiverse.BASE_CFG_PATH

    # Check if a user config already exists
    if os.path.isfile(user_cfg_path):
        # There already is one. Ask if this should be overwritten...
        print("A user config already exists at "+str(user_cfg_path))
        if input("Replace with base config? [y, N]  ").lower() in ['yes', 'y']:
            # Delete the file
            os.remove(user_cfg_path)
            print("")

        else:
            # Abort here
            print("Not distributing user config ...")
            return
    
    # At this point, can assume that it is desired to write the file and there is no other file there
    # Make sure that the folder exists
    os.makedirs(os.path.dirname(user_cfg_path), exist_ok=True)

    # Copy the file to where it should be
    copyfile(base_cfg_path, user_cfg_path)

    print("Distributed user configuration to {}\n".format(user_cfg_path))
