"""Implementation of the Multiverse class.

The Multiverse supplies the main user interface of the frontend.
"""
import os
import time
import copy
import glob
import re
import logging
from shutil import copyfile
from pkg_resources import resource_filename

import paramspace as psp

from utopya.datamanager import DataManager
from utopya.workermanager import WorkerManager
from utopya.plotting import PlotManager
from utopya.reporter import WorkerManagerReporter
from utopya.tools import recursive_update, read_yml, write_yml, load_model_cfg, pformat
from utopya.info import MODELS

# Configure and get logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

class Multiverse:
    """The Multiverse is where everything is orchestrated.
    
    It aims to make all the functionality of the Utopia frontend accessible in
    one place.
    
    Attributes:
        BASE_CFG_PATH (str): The path to the base configuration, supplied with
            the utopya package
        meta_cfg (dict): The parsed Multiverse meta-configuration. All
            further arguments are extracted from this dict.
        model_name (str): The model name associated with this Multiverse
        USER_CFG_SEARCH_PATH (str): The path at which a user config is expected
    """

    BASE_CFG_PATH = resource_filename('utopya', 'cfg/base_cfg.yml')
    USER_CFG_SEARCH_PATH = os.path.expanduser("~/.config/utopia/user_cfg.yml")
    RUN_DIR_TIME_FSTR = "%y%m%d-%H%M%S"

    def __init__(self, *, model_name: str, run_cfg_path: str=None, user_cfg_path: str=None, custom_env: dict=None, **update_meta_cfg):
        """Initialize the Multiverse.
        
        Args:
            model_name (str): A valid name of Utopia model
            run_cfg_path (str, optional): The path to the run configuration.
            user_cfg_path (str, optional): If given, this is used to update the
                base configuration. If None, will look for it in the default
                path, see Multiverse.USER_CFG_SEARCH_PATH.
            **update_meta_cfg: Can be used to update the meta configuration
                generated from the previous configuration levels
        """
        # Initialize property-managed attributes
        self._meta_cfg = None
        self._model_name = None
        self._dirs = {}
        self._resolved_cluster_params = None

        # Set the model name
        self.model_name = model_name
        
        log.info("Initializing Multiverse for '%s' model ...", self.model_name)

        # Save the model binary path and the configuration file
        self._model_binpath = MODELS[self.model_name]['binpath']
        log.debug("Associated executable of model '%s':\n  %s",
                  self.model_name, self.model_binpath)

        # Create meta configuration and list of used config files
        files = self._create_meta_cfg(run_cfg_path=run_cfg_path,
                                      user_cfg_path=user_cfg_path,
                                      update_meta_cfg=update_meta_cfg)
        # NOTE this already stores it in self._meta_cfg

        # Resolve the cluster parameters, if in cluster mode, and make some
        # minor (!!) adjustments.
        if self.cluster_mode:
            self._resolved_cluster_params = self._resolve_cluster_params()

            # To avoid config file collisions in the PlotManager:
            self._meta_cfg['plot_manager']['cfg_exists_action'] = 'skip'

        # Create the run directory and write the meta configuration into it.
        # This already performs the backup of the configuration files.
        self._create_run_dir(**self.meta_cfg['paths'], cfg_parts=files)

        # Provide some information
        log.info("Run directory:\n  %s", self.dirs['run'])

        # Create a data manager
        self._dm = DataManager(self.dirs['run'],
                               name=self.model_name + "_data",
                               **self.meta_cfg['data_manager'])

        # Create a WorkerManager instance and pass the reporter to it
        self._wm = WorkerManager(**self.meta_cfg['worker_manager'])

        # Instantiate the Reporter
        self._reporter = WorkerManagerReporter(self.wm,
                                               report_dir=self.dirs['run'],
                                               **self.meta_cfg['reporter'])

        # And instantiate the PlotManager with the model-specific plot config
        self._pm = PlotManager(dm=self.dm,
                               plots_cfg=MODELS[self.model_name]['plots_cfg'],
                               **self.meta_cfg['plot_manager'])

        log.info("Initialized Multiverse.")

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

        self._model_name = model_name
        log.debug("Set model_name:  %s", model_name)

    @property
    def model_binpath(self) -> str:
        """The path to this model's binary"""
        return self._model_binpath

    @property
    def meta_cfg(self) -> dict:
        """The meta configuration."""
        return self._meta_cfg

    @property
    def dirs(self) -> dict:
        """Information on managed directories."""
        return self._dirs

    @property
    def cluster_mode(self) -> bool:
        """Whether the Multiverse should run in cluster mode"""
        return self.meta_cfg['cluster_mode']

    @property
    def cluster_params(self) -> dict:
        """Returns a copy of the cluster mode configuration parameters"""
        return copy.deepcopy(self.meta_cfg['cluster_params'])

    @property
    def resolved_cluster_params(self) -> dict:
        """Returns a copy of the cluster configuration with all parameters
        resolved. This makes some additional keys available on the top level.
        """
        # Return the cached value as a _copy_ to secure it against changes
        return copy.deepcopy(self._resolved_cluster_params)

    @property
    def dm(self) -> DataManager:
        """The Multiverse's DataManager."""
        return self._dm

    @property
    def wm(self) -> WorkerManager:
        """The Multiverse's WorkerManager."""
        return self._wm

    @property
    def pm(self) -> PlotManager:
        """The Multiverse's PlotManager."""
        return self._pm

    # Public methods ..........................................................

    def run(self):
        """Starts a Utopia run. Whether this will be a single simulation or
        a parameter sweep is decided by the contents of the meta config.

        Note that (currently) each Multiverse instance can _not_ perform
        multiple runs!
        """
        # Depending on the configuration, call the corresponding run method
        if self.meta_cfg.get('perform_sweep'):
            self.run_sweep()
        else:
            self.run_single()
    
    def run_single(self):
        """Runs a single simulation.

        Note that (currently) each Multiverse instance can _not_ perform
        multiple runs!
        """
        # Get the parameter space from the config
        pspace = self.meta_cfg['parameter_space']

        # Get the default state of the parameter space
        uni_cfg = pspace.default

        # Add the task to the worker manager.
        log.info("Adding task for simulation of a single universe ...")
        self._add_sim_task(uni_id_str="0", uni_cfg=uni_cfg, is_sweep=False)

        # Prevent adding further tasks to disallow further runs
        self.wm.tasks.lock()

        # Tell the WorkerManager to start working (is a blocking call)
        self.wm.start_working(**self.meta_cfg['run_kwargs'])

        log.info("Finished single universe run. Yay. :)")

    def run_sweep(self):
        """Runs a parameter sweep.

        If cluster mode is enabled, this will split up the parameter space into
        (ideally) equally sized parts and only run one of these parts,
        depending on the node this Multiverse runs on.

        Note that (currently) each Multiverse instance can _not_ perform
        multiple runs!
        """
        
        # Get the parameter space from the config
        pspace = self.meta_cfg['parameter_space']

        if pspace.volume < 1:
            raise ValueError("The parameter space has no sweeps configured! "
                             "Refusing to run a sweep. You can either call "
                             "the run_single method or add sweeps to your "
                             "run configuration using the !sweep YAML tags.")

        # Get the parameter space iterator
        psp_iter = pspace.iterator(with_info='state_no_str')

        # Distinguish whether to do a regular sweep or we are in cluster mode
        if not self.cluster_mode:
            # Do a sweep over the whole activated parameter space
            log.info("Adding tasks for simulation of %d universes ...",
                     pspace.volume)

            for uni_cfg, uni_id_str in psp_iter:
                self._add_sim_task(uni_id_str=uni_id_str, uni_cfg=uni_cfg,
                                   is_sweep=True)

        else:
            # Prepare a cluster mode sweep
            log.info("Preparing cluster mode sweep ...")

            # Get the resolved cluster parameters
            # These include the following values:
            #    num_nodes:   The total number of nodes to simulate on. This
            #                 is what determines the modulo value.
            #    node_index:  Equivalent to the modulo offset, which depends
            #                 on the position of this Multiverse's node in the
            #                 sequence of all nodes.
            rcps = self.resolved_cluster_params
            num_nodes = rcps['num_nodes']
            node_index = rcps['node_index']

            # Inform about the number of universes to be simulated
            log.info("Adding tasks for cluster-mode simulation of "
                     "%d universes on this node (%d of %d) ...",
                     (   pspace.volume//num_nodes
                      + (pspace.volume%num_nodes > node_index)),
                     node_index + 1, num_nodes)

            for i, (uni_cfg, uni_id_str) in enumerate(psp_iter):
                # Skip if this node is not responsible
                if (i - node_index) % num_nodes != 0:
                    log.debug("Skipping:  %s", uni_id_str)
                    continue

                # Is valid for this node, add the simulation task
                self._add_sim_task(uni_id_str=uni_id_str, uni_cfg=uni_cfg,
                                   is_sweep=True)

        log.info("Added %d tasks.", len(self.wm.tasks))

        # Prevent adding further tasks to disallow further runs
        self.wm.tasks.lock()

        # Tell the WorkerManager to start working (is a blocking call)
        self.wm.start_working(**self.meta_cfg['run_kwargs'])

        log.info("Finished parameter sweep. Wohoo. :)")

    # "Private" methods .......................................................

    def _create_meta_cfg(self, *, run_cfg_path: str, user_cfg_path: str, update_meta_cfg: dict) -> dict:
        """Create the meta configuration from several parts and store it.
        
        The final configuration dict is built from up to four components,
        where one is always recursively updating the previous level:
            1. base: the default configuration, which is always present
            2. user (optional): configuration of user- and machine-related
               parameters
            3. run (optional): the configuration for the current Multiverse
               instance
            4. update (optional): can be used for a last update step

        The resulting configuration is the meta configuration and is stored
        to the `meta_cfg` attribute.

        Note that all model configurations can be loaded into the meta config
        via the yaml !model tag; this will already have occurred during
        loading of that file and does not depend on the model chosen in this
        Multiverse object.

        The parts are recorded in the `cfg_parts` dict and returned such that
        a backup can be created.
        
        Args:
            run_cfg_path (str): path to run_config
            user_cfg_path (str): path to the user_config file
            update_meta_cfg (dict): will be used to update the resulting dict
        
        Returns:
            dict: dict of the parts that were needed to create the meta config.
                The dict-key corresponds to the part name, the value is the
                payload which can be either a path to a cfg file or a dict
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
                # No user cfg will be loaded
                log.debug("No file found at the default search location.")

        elif user_cfg_path is False:
            log.debug("Not loading the user configuration from the default "
                      "search path: %s", self.USER_CFG_SEARCH_PATH)

        if user_cfg_path:
            user_cfg = read_yml(user_cfg_path,
                                error_msg="Did not find user configuration at "
                                          "the specified path {}!"
                                          "".format(user_cfg_path))

            # Check that it does not contain parameter_space
            if user_cfg and 'parameter_space' in user_cfg:
                raise ValueError("There was a 'parameter_space' key found in "
                                 "the user configuration loaded from {}. You "
                                 "need to remove it.".format(user_cfg_path))

        else:
            user_cfg = None

        # Read in the configuration corresponding to the chosen model
        model_cfg, model_cfg_path = load_model_cfg(self.model_name)
        # NOTE Unlike the other configuration files, this does not attach at
        # root level of the meta configuration but parameter_space.<model_name>
        # in order to allow it to be used as the default configuration for an
        # _instance_ of that model.

        # Read in the run configuration
        if run_cfg_path:
            run_cfg = read_yml(run_cfg_path,
                               error_msg="No run config could be found at {}!"
                                         "".format(run_cfg_path))
        else:
            run_cfg = None

        # After this point it is assumed that all values are valid
        # Those keys or values will throw errors once they are needed ...

        # Now perform the recursive update steps
        # Start with the base configuration
        meta_tmp = base_cfg
        log.debug("Performing recursive updates to arrive at meta "
                  "configuration ...")

        # Update with user configuration, if given
        if user_cfg:
            log.debug("Updating with user configuration ...")
            meta_tmp = recursive_update(meta_tmp, user_cfg)

        # In order to incorporate the model config, the parameter space is
        # needed. We can already be sure that the parameter_space key exists,
        # because it is added as part of the base_cfg.
        pspace = meta_tmp['parameter_space']

        # Adjust parameter space to include model configuration
        log.debug("Updating parameter space with model configuration for "
                  "model '%s' ...", self.model_name)
        pspace[self.model_name] = model_cfg
        # NOTE this works because meta_tmp is a dict and thus mutable :)

        # On top of all of that: add the run configuration, if given
        if run_cfg:
            log.debug("Updating with run configuration ...")
            meta_tmp = recursive_update(meta_tmp, run_cfg)

        # ... and the update_meta_cfg dictionary
        if update_meta_cfg:
            log.debug("Updating with given `update_meta_cfg` dictionary ...")
            meta_tmp = recursive_update(meta_tmp,
                                        copy.deepcopy(update_meta_cfg))
            # NOTE using copy to make sure that usage of the dict will not interfere with the Multiverse's meta config
        
        # Make `parameter_space` a ParamSpace object
        pspace = meta_tmp['parameter_space']
        meta_tmp['parameter_space'] = psp.ParamSpace(pspace)
        log.debug("Converted parameter_space to ParamSpace object.")
        
        # Store it
        self._meta_cfg = meta_tmp
        log.info("Loaded meta configuration.")

        # Prepare dict to store paths for config files in (for later backup)
        log.debug("Preparing dict of config parts ...")
        cfg_parts = dict(base=self.BASE_CFG_PATH,
                         user=user_cfg_path,
                         model=model_cfg_path,
                         run=run_cfg_path,
                         update=update_meta_cfg)

        # Done.
        return cfg_parts

    def _create_run_dir(self, *, out_dir: str, model_note: str=None, backup_involved_cfg_files: bool=True, cfg_parts: dict=None) -> None:
        """Create the folder structure for the run output.
        
        This will also write the meta config to the corresponding config
        directory.
        
        The following folder tree will be created
        utopia_output/   # all utopia output should go here
            model_a/
                180301-125410_my_model_note/
                    config/
                    data/
                        uni000/
                        uni001/
                        ...
                    eval/
            model_b/
                180301-125412_my_first_sim/
                180301-125413_my_second_sim/
        
        If running in cluster mode, the cluster parameters are resolved and
        used to determine the name of the simulation. The pattern then does not
        include a timestamp as each node might return not quite the same value.
        Instead, a value from an environment variable is used.
        The resulting path can have different forms, depending on which
        environment variables were present; required parts are denoted by a *
        in the following pattern; if the value of the other entries is not
        available, the connecting underscore will not be used.
            {timestamp}_{job id*}_{cluster}_{job account}_{job name}_{note}

        Args:
            out_dir (str): The base output directory, where all simulation data
                is stored
            model_note (str, optional): The note to add to the model
            backup_involved_cfg_files (bool, optional): If true, saves all
                involved parts of the configuration process to the config
                directory. Note: the meta configuration is always saved there!
            cfg_parts (dict, optional): The parts of the config to backup
        
        Raises:
            RuntimeError: If the simulation directory already existed. This
                should not occur, as the timestamp is unique. If it occurs,
                you either started two simulations very close to each other or 
                something is seriously wrong. Strange time zone perhaps?
        """
        def backup_config():
            # Write the meta config to the config directory.
            write_yml(self.meta_cfg,
                      path=os.path.join(self.dirs['config'], "meta_cfg.yml"))
            log.debug("Stored meta configuration in config directory.")

            # Separately, store the parameter space there
            write_yml(self.meta_cfg['parameter_space'],
                      path=os.path.join(self.dirs['config'],
                                        "parameter_space.yml"))
            log.debug("Stored parameter space.")

            # If configured, backup the other cfg files one by one
            if backup_involved_cfg_files and cfg_parts:
                log.debug("Backing up %d config parts...", len(cfg_parts))

                for part_name, val in cfg_parts.items():
                    _path = os.path.join(self.dirs['config'],
                                         part_name + "_cfg.yml")
                    # Distinguish two types of payload that will be saved:
                    if isinstance(val, str):
                        # Assumed to be path to a config file; copy it
                        log.debug("Copying %s config ...", part_name)
                        copyfile(val, _path)

                    elif isinstance(val, dict):
                        log.debug("Dumping %s config dict ...", part_name)
                        write_yml(val, path=_path)

            log.info("Backed up configuration files.")

        # Create the folder path to the simulation directory
        # NOTE The str cast ensures that out_dir is not a path-like object
        #      which causes problems for python < 3.6
        log.debug("Creating path for run directory inside %s ...", out_dir)
        out_dir = os.path.expanduser(str(out_dir))

        # Distinguish between regular mode and cluster mode
        if not self.cluster_mode:
            # Not in cluster mode; can use the timestamp directly
            timestamp = time.strftime(self.RUN_DIR_TIME_FSTR)

            # Define format string and build kwargs
            fstr = "{ts:}{note:}"
            fstr_kwargs = dict(ts=timestamp,
                               note=("" if not model_note
                                     else "_" + model_note))

        else:
            # In cluster mode, need to resolve cluster parameters first
            rcps = self.resolved_cluster_params

            # Define parts of the format string and the corresponding kwargs
            fstr_parts = []
            fstr_kwargs = dict()
        
            # required: timestamp
            timestamp = time.strftime(self.RUN_DIR_TIME_FSTR,
                                      time.gmtime(rcps['timestamp']))
            fstr_parts += ["{timestamp:}"]
            fstr_kwargs['timestamp'] = timestamp

            # required: job id
            fstr_parts = ["job{job_id:07d}"]
            fstr_kwargs = dict(job_id=rcps['job_id'])

            # optional
            if rcps.get('job_account'):
                fstr_parts += ["{job_account:}"]
                fstr_kwargs['job_account'] = rcps['job_account']

            if rcps.get('cluster_name'):
                fstr_parts += ["{cluster_name:}"]
                fstr_kwargs['cluster_name'] = rcps['cluster_name']

            if rcps.get('job_name'):
                fstr_parts += ["{job_name:}"]
                fstr_kwargs['job_name'] = rcps['job_name']

            if model_note:
                fstr_parts += ["{model_note:}"]
                fstr_kwargs['model_note'] = model_note

            # Connect all parts with an underscore to generate the actual fstr
            fstr = "_".join(fstr_parts)

        # fstr and fstr_kwargs ready now. Carry out the format operation
        run_dir_name = fstr.format(**fstr_kwargs)
        log.debug("Determined run directory name:  %s", run_dir_name)
        
        # Built the run directory path
        run_dir = os.path.join(out_dir, self.model_name, run_dir_name)
        log.debug("Built run directory path:  %s", run_dir)
        self.dirs['run'] = run_dir


        # Recursively create the whole path to the run directory
        try:
            os.makedirs(run_dir, exist_ok=self.cluster_mode)

        except OSError as err:
            raise RuntimeError("Simulation directory already exists. This "
                               "should not have happened and is probably due "
                               "to two simulations having been started at "
                               "almost the same time. Try to start the "
                               "simulation again.") from err

        # Make subfolders
        for subdir in ('config', 'data', 'eval'):
            subdir_path = os.path.join(run_dir, subdir)
            os.makedirs(subdir_path, exist_ok=self.cluster_mode)
            self.dirs[subdir] = subdir_path

        log.debug("Finished creating run directory.")
        log.debug("Paths registered:  %s", self._dirs)

        if not self.cluster_mode:
            # Should backup in any case
            backup_config()

        elif self.resolved_cluster_params['node_index'] == 0:
            # In cluster mode, the first node is responsible for backing up
            # the configuration; all others can relax.
            backup_config()

        else:
            log.debug("Not backing up config files, because it was already "
                      "taken care of by the first node.")
            # NOTE Not taking a try-except approach here because it might get
            #      messy when multiple nodes try to backup the configuration
            #      at the same time ...

        # Done.

    def _resolve_cluster_params(self) -> dict:
        """This resolves the cluster parameters, e.g. by setting parameters
        depending on certain environment variables. This function is called by
        the resolved_cluster_params property.
        
        Returns:
            dict: The resolved cluster configuration parameters
        
        Raises:
            ValueError: If a required environment variable was missing or empty
        """
        log.debug("Resolving cluster parameters from environment ...")

        # Get a copy of the meta configuration parameters
        cps = self.cluster_params

        # Determine the environment to use; defaults to os.environ
        env = cps.get('env') if cps.get('env') else dict(os.environ)

        # Get the mapping of environment variables to target variables
        mngr = cps['manager']
        var_map = cps['env_var_names'][mngr]

        # Resolve the variables from the environment, requiring them to not
        # be empty
        resolved = {target_key: env.get(var_name)
                    for target_key, var_name in var_map.items()
                    if env.get(var_name)}

        # Check that all required keys are available
        required = ('job_id', 'num_nodes', 'node_list', 'node_name')
        if any([var not in resolved for var in required]):
            raise ValueError("Missing environment variable for one or more of "
                             "the required parameters:  {}.  Make sure that "
                             "the corresponding environment variables are set!"
                             "\nManager: '{}'\n"
                             "Mapping:\n{}\n"
                             "Environment keys:\n  - {}\n\n"
                             "Full environment:\n{}"
                             "".format(", ".join(required), mngr,
                                       pformat(var_map),
                                       "\n  - ".join(env.keys()),
                                       pformat(env)))

        # Now do some postprocessing on some of the values
        # Ensure integers
        resolved['job_id'] = int(resolved['job_id'])
        resolved['num_nodes'] = int(resolved['num_nodes'])

        if 'num_procs' in resolved:
            resolved['num_procs'] = int(resolved['num_procs'])
        
        if 'timestamp' in resolved:
            resolved['timestamp'] = int(resolved['timestamp'])

        # Ensure reproducible node list format: ordered list
        # Achieve this by removing whitespace, then splitting and sorting
        delim = cps['node_list_delimiters'][mngr]
        
        node_list = resolved['node_list'].replace(" ", "")
        node_list = sorted(node_list.split(delim))
        resolved['node_list'] = node_list

        # Consistency checks
        if resolved['num_nodes'] != len(resolved['node_list']):
            raise ValueError("Cluster parameter `node_list` has a different "
                             "length ({}) than specified by the  `num_nodes` "
                             "parameter ({})."
                             "".format(len(resolved['node_list']),
                                       resolved['num_nodes']))

        if resolved['node_name'] not in node_list:
            raise ValueError("`node_name` '{}' is not part of `node_list` {}!"
                             "".format(resolved['node_name'], node_list))

        # Calculated values, needed in Multiverse.run
        # node_index: the offset in the modulo operation
        resolved['node_index'] = node_list.index(resolved['node_name'])

        # Return the resolved values
        log.debug("Resolved cluster parameters:\n%s", pformat(resolved))
        return resolved

    def _add_sim_task(self, *, uni_id_str: str, uni_cfg: dict, is_sweep: bool) -> None:
        """Helper function that handles task assignment to the WorkerManager.
        
        This function creates a WorkerTask that will perform the following
        actions __once it is grabbed and worked at__:
          - Create a universe (folder) for the task (simulation)
          - Write that universe's configuration to a yaml file in that folder
          - Create the correct arguments for the call to the model binary
        
        To that end, this method gathers all necessary arguments and registers
        a WorkerTask with the WorkerManager.
        
        Args:
            uni_id_str (str): The zero-padded uni id string
            uni_cfg (dict): given by ParamSpace. Defines how many simulations
                should be started
            is_sweep (bool): Flag is needed to distinguish between sweeps
                and single simulations. With this information, the forwarding
                of a simulation's output stream can be controlled.
        
        Raises:
            RuntimeError: If adding the simulation task failed
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
            uni_dir = os.path.join(self.dirs['data'], uni_basename)

            # Now create the folder
            os.mkdir(uni_dir)
            log.debug("Created universe directory:\n  %s", uni_dir)

            # Generate a path to the output hdf5 file and add it to the dict
            output_path = os.path.join(uni_dir, "data.h5")
            uni_cfg['output_path'] = output_path

            # write essential part of config to file:
            uni_cfg_path = os.path.join(uni_dir, "config.yml")
            write_yml(uni_cfg, path=uni_cfg_path)

            # building args tuple for task assignment
            # assuming the binary takes as only argument the path to the config
            args = (model_binpath, uni_cfg_path)

            # Generate a new worker_kwargs dict, carrying over the given ones
            wk = dict(args=args,
                      read_stdout=True,
                      stdout_parser="yaml_dict",
                      **(worker_kwargs if worker_kwargs else {}))

            # Determine whether to save the streams
            if wk.get('save_streams', True):
                # Generate a path and store in the worker kwargs
                wk['save_streams_to'] = os.path.join(uni_dir, "{name:}.log")

            return wk

        # Generate the universe basename, which will be used for the folder
        # and the task name
        uni_basename = "uni" + uni_id_str

        # Create the dict that will be passed as arguments to setup_universe
        setup_kwargs = dict(model_name=self.model_name,
                            model_binpath=self.model_binpath,
                            uni_cfg=uni_cfg,
                            uni_basename=uni_basename)

        # Process worker_kwargs
        wk = self.meta_cfg.get('worker_kwargs')

        if wk and wk.get('forward_streams') == 'in_single_run':
            # Reverse the flag to determine whether to forward streams
            wk['forward_streams'] = (not is_sweep)
            wk['forward_kwargs'] = dict(forward_raw=True)

        # Try to add a task to the worker manager
        try:
            self.wm.add_task(name=uni_basename,
                             priority=None,
                             setup_func=setup_universe,
                             setup_kwargs=setup_kwargs,
                             worker_kwargs=wk)

        except RuntimeError as err:
            # Task list was locked, probably due to a run already having taken
            # place...
            raise RuntimeError("Could not add simulation task for universe "
                               "'{}'! Did you already perform a run with this "
                               "Multiverse?"
                               "".format(uni_basename)) from err

        log.debug("Added simulation task: %s.", uni_basename)


# -----------------------------------------------------------------------------
# TODO Add cluster mode support when automatically determining run directory

class FrozenMultiverse(Multiverse):
    """A frozen Multiverse is like a Multiverse, but frozen.

    It is initialized from a finished Multiverse run and re-creates all the
    attributes from that data, e.g.: the meta configuration, a DataManager,
    and a PlotManager.

    Note that it is no longer able to perform any simulations.
    """

    def __init__(self, *, model_name: str, run_dir: str=None, run_cfg_path: str=None, user_cfg_path: str=None, use_meta_cfg_from_run_dir: bool=False, **update_meta_cfg):
        """Initializes the FrozenMultiverse from a model name and the name 
        of a run directory.
        
        Note that this also takes arguments to specify the run configuration to
        use.
        
        Args:
            model_name (str): The name of the model to load. From this, the
                model output directory is computed and run_dir will be seen
                as relative to that directory.
            run_dir (str, optional): The run directory to load. Can be a path
                relative to the current working directory, an absolute path,
                or the timestamp of the run directory. If not given, will use the most recent timestamp.
            run_cfg_path (str, optional): The path to the run configuration.
            user_cfg_path (str, optional): If given, this is used to update the
                base configuration. If None, will look for it in the default
                path, see Multiverse.USER_CFG_SEARCH_PATH.
            use_meta_cfg_from_run_dir (bool, optional): If True, will load the
                meta configuration from the given run directory; only works for
                absolute run directories.
            **update_meta_cfg: Can be used to update the meta configuration
                generated from the previous configuration levels
        """
        # Initialize property-managed attributes
        self._meta_cfg = None
        self._model_name = None
        self._dirs = {}
        self._resolved_cluster_params = None

        # Set the model name
        self.model_name = model_name

        log.info("Initializing FrozenMultiverse for '%s' model ...",
                 self.model_name)

        # Decide whether to load the meta configuration from the given run
        # directory or the currently available one.
        if (use_meta_cfg_from_run_dir
            and isinstance(run_dir, str)
            and os.path.isabs(run_dir)):
            # Find the meta config backup file and load it
            # Alternatively, create it from the singular backup files ...
            # log.info("Trying to load meta configuration from given absolute "
            #          "run directory ...")

            raise NotImplementedError

            # Update it with the given update_meta_cfg dict

        else:
            # Need to create a meta configuration from the currently available
            # values.
            self._create_meta_cfg(run_cfg_path=run_cfg_path,
                                  user_cfg_path=user_cfg_path,
                                  update_meta_cfg=update_meta_cfg)
            # NOTE this already stores it in self._meta_cfg

        # Only keep selected entries from the meta configuration. The rest is
        # not needed and is deleted in order to not confuse the user with
        # potentially varying versions of the meta config.
        self._meta_cfg = {k: v for k, v in self._meta_cfg.items()
                          if k in ('paths', 'data_manager', 'plot_manager',
                                   'cluster_mode', 'cluster_params')}

        # Resolve the cluster parameters, if in cluster mode
        if self.cluster_mode:
            self._resolved_cluster_params = self._resolve_cluster_params()

        # Generate the path to the run directory that is to be loaded
        self._create_run_dir(**self.meta_cfg['paths'], run_dir=run_dir)
        log.info("Run directory:\n  %s", self.dirs['run'])

        # Create a data manager
        self._dm = DataManager(self.dirs['run'],
                               name=self.model_name + "_data",
                               **self.meta_cfg['data_manager'])

        # Instantiate the PlotManager with the model-specific plot config
        self._pm = PlotManager(dm=self.dm,
                               plots_cfg=MODELS[self.model_name]['plots_cfg'],
                               **self.meta_cfg['plot_manager'])

        log.info("Initialized FrozenMultiverse.")

    def _create_run_dir(self, *, out_dir: str, run_dir: str, **kwargs):
        """Helper function to find the run directory from arguments given
        to __init__.
        
        Args:
            out_dir (str): The output directory
            run_dir (str): The run directory to use
            **kwargs: ignored
        
        Raises:
            IOError: No directory found to use as run directory
            TypeError: When run_dir was not a string
        """
        # Create model directory
        out_dir = os.path.expanduser(str(out_dir))
        model_dir = os.path.join(out_dir, self.model_name)

        # Distinguish different types of values for the run_dir argument
        if run_dir is None:
            # Is absolute, can leave it as it is
            log.info("Trying to identify most recent run directory ...")

            # Create list of _directories_ matching timestamp pattern
            dirs = [d for d in sorted(os.listdir(model_dir))
                    if  os.path.isdir(os.path.join(model_dir, d))
                    and re.match(r'\d{6}-\d{6}_?.*', os.path.basename(d))]
            
            # Use the latest to choose the run directory
            run_dir = os.path.join(model_dir, dirs[-1])

        elif isinstance(run_dir, str):
            # Can now expand the user
            run_dir = os.path.expanduser(run_dir)

            # Distinguish absolute and relative paths and time stamps
            if os.path.isabs(run_dir):
                log.debug("Received absolute run_dir, using that one.")

            elif re.match(r'\d{6}-\d{6}_?.*', run_dir):
                # Is a timestamp, look relative to the model directory
                log.info("Received timestamp '%s' for run_dir; trying to find "
                         "one within the model directory ...", run_dir)
                run_dir = os.path.join(model_dir, run_dir)

            else:
                # Is not an absolute path and not a timestamp; thus a relative
                # path to the current working directory
                run_dir = os.path.join(os.getcwd(), run_dir)

        else:
            raise TypeError("Argument run_dir needs to be None, an absolute "
                            "path, or a path relative to the model output "
                            "directory, but it was: {}".format(run_dir))
            
        # Check if the directory exists
        if not os.path.isdir(run_dir):
            raise IOError("No directory found at run path '{}'!"
                          "".format(run_dir))

        # It does. Store it as attribute.
        self.dirs['run'] = run_dir

        # Also associate the sub directories
        for subdir in ('config', 'eval', 'data'):
            # Check if the directory exists
            subdir_path = os.path.join(run_dir, subdir)

            # Now store
            self.dirs[subdir] = subdir_path

        # Done

    # TODO return errors in properties that are unavailable?!
