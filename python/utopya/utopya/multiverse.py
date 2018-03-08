"""Implementation of the Multiverse class.

The Multiverse supplies the main user interface of the frontend.
"""
import os
import time
import logging

from utopya.workermanager import WorkerManager, enqueue_json
from utopya.tools import recursive_update, read_yml, write_yml

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

    def __init__(self, *, model_name: str, metaconfig: str="metaconfig.yml", userconfig: str=None):
        """Initialize the Multiverse.

        Load default configuration file and adjust parameters given
        by metaconfig and userconfig.
        """
        self._config = self._configure(metaconfig=metaconfig,
                                       userconfig=userconfig)

        # set the model name for folder setup
        self._model_name = model_name

        # Initialise empty dict for keeping track of directory paths
        self.dirs = dict()

        # Now create the simulation directory and its internal subdirectories
        self._create_sim_dir(**self._config['multiverse']['output_path'])

        # create a WorkerManager instance
        self._wm = WorkerManager(**self._config['multiverse']['worker_manager'])

    # Properties ..............................................................

    @property
    def model_name(self) -> str:
        """The model name associated with this Multiverse"""
        return self._model_name

    @property
    def wm(self) -> WorkerManager:
        """The Multiverse's WorkerManager."""
        return self._wm

    # Public methods ..........................................................

    def run(self):
        """Starts a Utopia run. Whether this will be a single simulation or
        a Parameter sweep is decided by the contents of the meta_cfg."""
        log.info("Preparing to run Utopia ...")

        # FIXME The config path needs to be changed to the correct one when putting this together. This is only a dummy for now, should be the call to the property
        run_single = self._config['perform_sweep']

        # Depending on the configuration, the corresponding methods can already be called.
        if run_single:
            self.run_single()
        else:
            self.run_sweep()
    
    def run_single(self):
        """Runs a single simulation."""

        # Get the parameter space from the config
        # FIXME use property here
        pspace = self._config['parameter_space']
        
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

    def _configure(self, *, metaconfig: str, userconfig: str=None) -> dict:
        """Read default configuration file and adjust parameters.

        The default metaconfig file, the user/machine-specific file (if
        existing) and the regular metaconfig file are read in and the default
        metaconfig is adjusted accordingly to create a single output file.

        Args:
            metaconfig: path to metaconfig. An empty or invalid path raises
                FileNotFoundError.
            userconfig: optional user/machine-specific configuration file

        Returns:
            dict: returns the updated default metaconfig to be processed
                further or to be written out.
        """
        # In the following, the final configuration dict is built from three components:
        # The base is the default configuration, which is always present
        # If a userconfig is present, this recursively updates the defaults
        # Then, the given metaconfig recursively updates the created dict
        log.debug("Reading default metaconfig.")
        defaults = read_yml("default_metaconfig.yml",
                            error_msg="default_metaconfig.yml is not present.")

        if userconfig is not None:
            log.debug("Reading userconfig from: %s.", userconfig)
            userconfig = read_yml(userconfig,
                                  error_msg="{0} was given but userconfig"
                                            " could not be found."
                                            "".format(userconfig))

        log.debug("Reading metaconfig from: %s", metaconfig)
        metaconfig = read_yml(metaconfig,
                              error_msg="{0} was given but metaconfig"
                                        " could not be found."
                                        "".format(metaconfig))

        # TODO: typechecks of values should be completed below here.
        # after this point it is assumed that all values are valid

        # Now perform the recursive update steps
        log.debug("Updating default metaconfig with given configurations.")
        if userconfig is not None:  # update default with user spec
            defaults = recursive_update(defaults, userconfig)

        # update default_metaconfig with metaconfig
        defaults = recursive_update(defaults, metaconfig)

        return defaults
       
    def _create_sim_dir(self, *, model_name: str, out_dir: str, model_note: str=None) -> None:
        """Create the folder structure for the simulation output.
    
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
        log.debug("Expanding user %s", out_dir)
        out_dir = os.path.expanduser(out_dir)
        sim_dir = os.path.join(out_dir,
                               model_name,
                               time.strftime("%Y%m%d-%H%M%S"))

        # Append a model note, if needed
        if model_note:
            sim_dir += "_" + model_note

        # Inform and store to directory dict
        log.debug("Expanded user and time stamp to %s", sim_dir)
        self.dirs['sim_dir'] = sim_dir

        # Recursively create the whole path to the simulation directory
        try:
            os.makedirs(sim_dir)
        except OSError as err:
            raise RuntimeError("Simulation directory already exists. This "
                               "should not have happened. Try to start the "
                               "simulation again.") from err

        # Make subfolders
        for subdir in ('config', 'eval', 'universes'):
            subdir_path = os.path.join(sim_dir, subdir)
            os.mkdir(subdir_path)
            self.dirs[subdir] = subdir_path

        log.debug("Finished creating simulation directory. Now registered: %s",
                  self.dirs)

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
        uni_path = os.path.join(self.dirs['universes'],
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
        self._wm.add_task(priority=None,
                          setup_func=setup_universe,
                          setup_kwargs=setup_kwargs)

        log.debug("Added simulation task for universe %d.", uni_id)