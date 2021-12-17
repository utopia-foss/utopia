"""Implementation of the Multiverse class.

The Multiverse supplies the main user interface of the frontend.
"""
import os
import time
import copy
import re
import logging
import itertools
from tempfile import TemporaryDirectory
from shutil import copy2
from pkg_resources import resource_filename
from collections import defaultdict

import paramspace as psp

from .model_registry import ModelInfoBundle, get_info_bundle, load_model_cfg
from .cfg import get_cfg_path as _get_cfg_path
from .datamanager import DataManager
from .workermanager import WorkerManager
from .parameter import ValidationError
from .plotting import PlotManager
from .reporter import WorkerManagerReporter
from .yaml import load_yml, write_yml
from .tools import recursive_update, pformat, parse_num_steps
from ._cluster import parse_node_list

# Configure and get logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------


class Multiverse:
    """The Multiverse is where a single simulation run is orchestrated from.

    It spawns multiple universes, each of which represents a single simulation
    of the selected model with the parameters specified by the meta
    configuration.

    The WorkerManager takes care to perform these simulations in parallel, the
    DataManager allows loading the created data, and the PlotManager handles
    plotting of that data.
    """

    # Where the default meta configuration can be found
    BASE_META_CFG_PATH = resource_filename('utopya', 'cfg/base_cfg.yml')

    # Where to look for the user configuration
    USER_CFG_SEARCH_PATH = _get_cfg_path('user')

    # The time format string for the run directory
    RUN_DIR_TIME_FSTR = "%y%m%d-%H%M%S"

    # Where the utopya base plots configuration can be found; this is passed to
    # the PlotManager
    UTOPYA_BASE_PLOTS_PATH = resource_filename('utopya',
                                               'plot_funcs/base_plots.yml')

    def __init__(self, *,
                 model_name: str=None, info_bundle: ModelInfoBundle=None,
                 run_cfg_path: str=None, user_cfg_path: str=None,
                 _shared_worker_manager: WorkerManager=None,
                 **update_meta_cfg):
        """Initialize the Multiverse.

        Args:
            model_name (str, optional): The name of the model to run
            info_bundle (ModelInfoBundle, optional): The model information
                bundle that includes information about the binary path etc.
                If not given, will attempt to read it from the model registry.
            run_cfg_path (str, optional): The path to the run configuration.
            user_cfg_path (str, optional): If given, this is used to update the
                base configuration. If None, will look for it in the default
                path, see Multiverse.USER_CFG_SEARCH_PATH.
            _shared_worker_manager (WorkerManager, optional): If given, this
                already existing WorkerManager instance (and its reporter)
                will be used instead of initializing new instances.

                .. warning::

                    This argument is only exposed for internal purposes.
                    It should not be used for production code and behavior of
                    this argument may change at any time.

            **update_meta_cfg: Can be used to update the meta configuration
                generated from the previous configuration levels
        """
        # First things first: get the info bundle
        self._info_bundle = get_info_bundle(model_name=model_name,
                                            info_bundle=info_bundle)
        log.progress("Initializing Multiverse for '%s' model ...",
                     self.model_name)

        # Setup property-managed attributes
        self._dirs = dict()
        self._model_binpath = None
        self._tmpdir = None
        self._resolved_cluster_params = None

        # Create meta configuration and list of used config files
        mcfg, cfg_parts = self._create_meta_cfg(
            run_cfg_path=run_cfg_path,
            user_cfg_path=user_cfg_path,
            update_meta_cfg=update_meta_cfg
        )
        self._meta_cfg = mcfg
        log.info("Loaded meta configuration.")

        # In cluster mode, need to make some adjustments via additional dicts
        dm_cluster_kwargs = dict()
        wm_cluster_kwargs = dict()
        if self.cluster_mode:
            log.note("Cluster mode enabled.")
            self._resolved_cluster_params = self._resolve_cluster_params()
            rcps = self.resolved_cluster_params  # creates a deep copy

            log.note("This is node %d of %d.",
                     rcps['node_index'] + 1, rcps['num_nodes'])

            # Changes to the meta configuration
            # To avoid config file collisions in the PlotManager:
            self._meta_cfg['plot_manager']['cfg_exists_action'] = 'skip'

            # _Additional_ arguments to pass to *Manager initializations below
            # ... for DataManager
            timestamp = rcps['timestamp']
            dm_cluster_kwargs = dict(
                out_dir_kwargs=dict(timestamp=timestamp, exist_ok=True)
            )

            # ... for WorkerManager
            wm_cluster_kwargs = dict(
                cluster_mode=True, resolved_cluster_params=rcps
            )

        # Create the run directory and write the meta configuration into it.
        self._create_run_dir(**self.meta_cfg['paths'])
        log.note("Run directory:\n  %s", self.dirs['run'])

        # Backup involved files, if not in cluster mode or on the relevant node
        if (   not self.cluster_mode
            or self.resolved_cluster_params['node_index'] == 0):
            # If not in cluster mode, should backup in any case.
            # In cluster mode, the first node is responsible for backing up
            # the configuration; all others can relax.
            self._perform_backup(**self.meta_cfg['backups'],
                                 cfg_parts=cfg_parts)

        else:
            log.debug("Not backing up config files, because it was already "
                      "taken care of by the first node.")
            # NOTE Not taking a try-except approach here because it might get
            #      messy when multiple nodes try to backup the configuration
            #      at the same time ...

        # Validate the parameters specified in the meta configuration
        self._validate_meta_cfg()

        # Prepare the executable
        self._prepare_executable(**self.meta_cfg['executable_control'])

        # Create a DataManager instance
        self._dm = DataManager(self.dirs['run'],
                               name=self.model_name + "_data",
                               **self.meta_cfg['data_manager'],
                               **dm_cluster_kwargs)
        log.progress("Initialized DataManager.")

        # Either create a WorkerManager instance and its associated reporter
        # or use an already existing WorkerManager that is also used elsewhere
        if not _shared_worker_manager:
            self._wm = WorkerManager(**self.meta_cfg['worker_manager'],
                                     **wm_cluster_kwargs)
            self._reporter = WorkerManagerReporter(self.wm, mv=self,
                                                   report_dir=self.dirs['run'],
                                                   **self.meta_cfg['reporter'])
        else:
            self._wm = _shared_worker_manager
            self._reporter = self.wm.reporter
            log.info("Using a shared WorkerManager instance and reporter.")

        # And instantiate the PlotManager with the model-specific plot config
        self._pm = self._setup_pm()

        log.progress("Initialized Multiverse.\n")

    # Properties ..............................................................

    @property
    def info_bundle(self) -> ModelInfoBundle:
        """The model info bundle for this Multiverse"""
        return self._info_bundle

    @property
    def model_name(self) -> str:
        """The model name associated with this Multiverse"""
        return self.info_bundle.model_name

    @property
    def model_binpath(self) -> str:
        """The path to this model's binary"""
        if self._model_binpath is not None:
            return self._model_binpath
        return self.info_bundle.paths['binary']

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

    def run(self, *, sweep: bool=None):
        """Starts a Utopia simulation run.

        Specifically, this method adds simulation tasks to the associated
        WorkerManager, locks its task list, and then invokes the
        :py:meth:`~utopya.workermanager.WorkerManager.start_working` method
        which performs all the simulation tasks.

        If cluster mode is enabled, this will split up the parameter space into
        (ideally) equally sized parts and only run one of these parts,
        depending on the cluster node this Multiverse is being invoked on.

        .. note::

            As this method locks the task list of the
            :py:class:`~utopya.workermanager.WorkerManager`, no further tasks
            can be added henceforth. This means, that each Multiverse instance
            can only perform a single simulation run.

        Args:
            sweep (bool, optional): Whether to perform a sweep or not. If None,
                the value will be read from the ``perform_sweep`` key of the
                meta-configuration.
        """
        log.info("Preparing for simulation run ...")

        # Add tasks, then prevent adding further tasks to disallow further runs
        self._add_sim_tasks(sweep=sweep)
        self.wm.tasks.lock()

        # Tell the WorkerManager to start working (is a blocking call)
        self.wm.start_working(**self.meta_cfg['run_kwargs'])

        # Done! :)
        log.success("Finished run. Wohoo. :)")

    def run_single(self):
        """Runs a single simulation using the parameter space's default value.

        See :py:meth:`~utopya.multiverse.Multiverse.run` for more information.
        """
        return self.run(sweep=False)

    def run_sweep(self):
        """Runs a parameter sweep.

        See :py:meth:`~utopya.multiverse.Multiverse.run` for more information.
        """
        return self.run(sweep=True)

    def renew_plot_manager(self, **update_kwargs):
        """Tries to set up a new PlotManager. If this succeeds, the old one is
        discarded and the new one is associated with this Multiverse.

        Args:
            **update_kwargs: Passed on to PlotManager.__init__
        """
        try:
            pm = self._setup_pm(**update_kwargs)

        except Exception as exc:
            raise ValueError("Failed setting up a new PlotManager! The old "
                             "PlotManager remains."
                             "") from exc

        self._pm = pm

    # Helpers .................................................................

    def _create_meta_cfg(self, *, run_cfg_path: str, user_cfg_path: str,
                         update_meta_cfg: dict) -> dict:
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
        log.info("Building meta-configuration ...")

        # Read in the base meta configuration
        base_cfg = load_yml(self.BASE_META_CFG_PATH)

        # Decide whether to read in the user configuration from the default
        # search location or use a user-passed one
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

        user_cfg = None
        if user_cfg_path:
            user_cfg = load_yml(user_cfg_path)

        # Read in the configuration corresponding to the chosen model
        (model_cfg,
         model_cfg_path,
         params_to_validate) = load_model_cfg(info_bundle=self.info_bundle)
        # NOTE Unlike the other configuration files, this does not attach at
        # root level of the meta configuration but parameter_space.<model_name>
        # in order to allow it to be used as the default configuration for an
        # _instance_ of that model.

        # Read in the run configuration
        run_cfg = None
        if run_cfg_path:
            try:
                run_cfg = load_yml(run_cfg_path)
            except FileNotFoundError as err:
                raise FileNotFoundError(
                    f"No run config could be found at {run_cfg_path}!"
                ) from err
            log.note("Run configuration:\n  %s", run_cfg_path)
        else:
            log.note("Using default run configuration:\n  %s\n",
                     model_cfg_path)
        # After this point it is assumed that all values are valid.
        # Those keys or values will throw errors once they are used ...

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
        pspace[self.model_name] = recursive_update(
            pspace.get(self.model_name, {}), model_cfg
        )
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
            # NOTE using deep copy to make sure that usage of the dict will not
            #      interfere with the Multiverse's meta config

        # Make `parameter_space` a ParamSpace object
        pspace = meta_tmp['parameter_space']
        meta_tmp['parameter_space'] = psp.ParamSpace(pspace)
        log.debug("Converted parameter_space to ParamSpace object.")

        # Add the parameters that require validation
        meta_tmp['parameters_to_validate'] = params_to_validate
        log.debug("Added %d parameters requiring validation.",
                  len(params_to_validate))

        # Prepare dict to store paths for config files in (for later backup)
        log.debug("Preparing dict of config parts ...")
        cfg_parts = dict(base=self.BASE_META_CFG_PATH,
                         user=user_cfg_path,
                         model=model_cfg_path,
                         run=run_cfg_path,
                         update=update_meta_cfg)
        return meta_tmp, cfg_parts

    def _create_run_dir(self, *, out_dir: str, model_note: str=None) -> None:
        """Create the folder structure for the run output.

        For the chosen model name and current timestamp, the run directory
        will be of form <timestamp>_<model_note> and be part of the following
        directory tree:

        ::

            utopia_output
                model_a
                    180301-125410_my_model_note
                        config
                        data
                            uni000
                            uni001
                            ...
                        eval
                model_b
                    180301-125412_my_first_sim
                    180301-125413_my_second_sim

        If running in cluster mode, the cluster parameters are resolved and
        used to determine the name of the simulation. The pattern then does not
        include a timestamp as each node might return not quite the same value.
        Instead, a value from an environment variable is used.
        The resulting path can have different forms, depending on which
        environment variables were present; required parts are denoted by a
        ``*`` in the following pattern; if the value of the other entries is
        not available, the connecting underscore will not be used:

        ::

            {timestamp}_{job id*}_{cluster}_{job account}_{job name}_{note}

        Args:
            out_dir (str): The base output directory, where all Utopia output
                is stored.
            model_note (str, optional): The note to add to the run directory
                of the current run.

        Raises:
            RuntimeError: If the simulation directory already existed. This
                should not occur, as the timestamp is unique. If it occurs,
                you either started two simulations very close to each other or
                something is seriously wrong. Strange time zone perhaps?
        """
        # Define a list of format string parts, starting with timestamp
        fstr_parts = ['{timestamp:}']

        # Add respective information, depending on mode
        if not self.cluster_mode:
            # Available information is only the timestamp and the model note
            fstr_kwargs = dict(timestamp=time.strftime(self.RUN_DIR_TIME_FSTR),
                               model_note=model_note)

        else:
            # In cluster mode, need to resolve cluster parameters first
            rcps = self.resolved_cluster_params

            # Now, gather all information for the format string that will
            # determine the name of the output directory. Make all the info
            # available that was supplied from environment variables
            fstr_kwargs = {k: v for k, v in rcps.items()
                           if k not in ('custom_out_dir',)}

            # Parse timestamp and model note separately
            timestr = time.strftime(self.RUN_DIR_TIME_FSTR,
                                    time.gmtime(rcps['timestamp']))
            fstr_kwargs['timestamp'] = timestr       # overwrites existing
            fstr_kwargs['model_note'] = model_note   # may be None

            # Add the additional run dir format string parts; its the user's
            # responsibility to supply something reasonable here.
            if self.cluster_params.get('additional_run_dir_fstrs'):
                fstr_parts += self.cluster_params['additional_run_dir_fstrs']

            # Now, also allow a custom output directory
            if rcps.get('custom_out_dir'):
                out_dir = rcps['custom_out_dir']

        # Have the model note as suffix
        if model_note:
            fstr_parts += ['{model_note:}']

        # fstr_parts and fstr_kwargs ready now. Carry out the format operation.
        fstr = "_".join(fstr_parts)
        run_dir_name = fstr.format(**fstr_kwargs)
        log.debug("Determined run directory name:  %s", run_dir_name)

        # Parse the output directory, then build the run directory path
        log.debug("Creating path for run directory inside %s ...", out_dir)
        out_dir = os.path.expanduser(str(out_dir))

        run_dir = os.path.join(out_dir, self.model_name, run_dir_name)
        log.debug("Built run directory path:  %s", run_dir)
        self.dirs['run'] = run_dir

        # ... and create it. In cluster mode, it may already exist.
        try:
            os.makedirs(run_dir, exist_ok=self.cluster_mode)

        except OSError as err:
            raise RuntimeError("Simulation directory already exists. This "
                               "should not have happened and is probably due "
                               "to two simulations having been started at "
                               "almost the same time. Try to start the "
                               "simulation again or add a unique model note."
                               ) from err

        log.debug("Created run directory.")

        # Create the subfolders that are always assumed to be present
        for subdir in ('config', 'data', 'eval'):
            subdir_path = os.path.join(run_dir, subdir)
            os.makedirs(subdir_path, exist_ok=self.cluster_mode)
            self.dirs[subdir] = subdir_path

        log.debug("Created subdirectories:  %s", self._dirs)

    def _setup_pm(self, **update_kwargs) -> PlotManager:
        """Helper function to setup a PlotManager instance"""
        paths = self.info_bundle.paths
        pm_kwargs = copy.deepcopy(self.meta_cfg['plot_manager'])

        if update_kwargs:
            pm_kwargs = recursive_update(pm_kwargs, update_kwargs)

        log.info("Initializing PlotManager ...")
        pm = PlotManager(
            dm=self.dm,
            _model_info_bundle=self.info_bundle,
            base_cfg_pools=[
                ("utopya", self.UTOPYA_BASE_PLOTS_PATH),
                (f"{self.model_name}_base", paths.get('base_plots', {})),
            ],
            default_plots_cfg=paths.get('default_plots'),
            **pm_kwargs,
        )

        log.progress("Initialized PlotManager.")
        log.note("Available base configuration pools:  %s",
                 ", ".join(pm.base_cfg_pools.keys()))
        log.note("Output directory:  %s",
                 pm._out_dir if pm._out_dir else "\n  " + self.dm.dirs['out'])

        return pm

    def _perform_backup(self, *, cfg_parts: dict,
                        backup_cfg_files: bool=True,
                        backup_executable: bool=False) -> None:
        """Performs a backup of that information that can be used to recreate a
        simulation.

        The configuration files are backed up into the ``config`` subdirectory
        of the run directory. All other relevant information is stored in an
        additionally created ``backup`` subdirectory.

        .. warning::

            These backups are created prior to the start of the actual
            simulation run and contains information known at that point.
            Any changes to the meta configuration made *after* initialization
            of the Multiverse will not be reflected in these backups.

            In particular, the ``perform_sweep`` and ``parameter_space``
            entries of the meta configuration may not reflect which form of
            parameter space iteration was actually performed, because the
            ``run_single`` and ``run_sweep`` methods overwrite this behavior.
            To that end, that information is separately stored once the ``run``
            methods are invoked.

        Args:
            cfg_parts (dict): A dict of either paths to configuration files or
                dict-like data that is to be dumped into a configuration file.
            backup_cfg_files (bool, optional): Whether to backup the individual
                configuration files (i.e. the ``cfg_parts`` information). If
                false, the meta configuration will still be backed up.
            backup_executable (bool, optional): Whether to backup the
                executable. Note that these files can sometimes be quite large.
        """
        log.info("Performing backups ...")
        cfg_dir = self.dirs['config']

        # Write the meta config to the config directory.
        write_yml(self.meta_cfg,
                  path=os.path.join(cfg_dir, "meta_cfg.yml"))
        log.note("  Backed up meta configuration.")

        # Store the *full* parameter space and its metadata
        # NOTE This data may not be equivalent to the parameter space that is
        #      used for a simulation run; another backup is performed when
        #      adding the corresponding simulation tasks.
        _pspace_info = dict(perform_sweep=self.meta_cfg.get('perform_sweep'))
        self._perform_pspace_backup(self.meta_cfg['parameter_space'],
                                    filename="full_parameter_space",
                                    **_pspace_info)

        # If configured, backup the other cfg files one by one.
        if backup_cfg_files:
            log.debug("Backing up %d involved configuration parts...",
                      len(cfg_parts))

            for part_name, val in cfg_parts.items():
                _path = os.path.join(cfg_dir, part_name + "_cfg.yml")

                # Distinguish two types of payload that will be saved:
                if isinstance(val, str):
                    # Assumed to be path to a config file; copy it
                    log.debug("Copying %s config ...", part_name)
                    copy2(val, _path)

                elif isinstance(val, dict):
                    log.debug("Dumping %s config dict ...", part_name)
                    write_yml(val, path=_path)

            log.note("  Backed up all involved configuration files.")

        # If enabled, back up the executable as well
        if backup_executable:
            backup_dir = os.path.join(self.dirs['run'], 'backup')
            os.makedirs(backup_dir, exist_ok=True)

            copy2(self.model_binpath,
                  os.path.join(backup_dir, self.model_name))
            log.note("  Backed up executable.")

    def _perform_pspace_backup(self, pspace: psp.ParamSpace, *,
                               filename: str = "parameter_space",
                               **info_kwargs):
        """Stores the given parameter space and its metadata into the
        ``config`` directory.
        Two files will be produced:

            - ``config/{filename}.yml``: the passed ``pspace`` object
            - ``config/{filename}_info.yml``: the passed ``pspace`` object's
                info dictionary containing relevant metadata (and the
                additionally passed ``info_kwargs``)

        .. note::

            This method is separated from the regular backup method
            :py:`~utopya.multiverse.Multiverse._perform_backup` because the
            parameter space that is *used* during a simulation run may be a
            lower-dimensional version of the one the Multiverse was
            initialized with.
            To that end, :py:`~utopya.multiverse.Multiverse.run` will invoke
            this backup function again once the relevant information is fully
            determined. This is important because it is needed to communicate
            the correct information about the sweep to objects downstream in
            the pipeline (e.g. :py:`~utopya.plotting.MultiversePlotCreator`).

        Args:
            pspace (psp.ParamSpace): The ParamSpace object to save
            filename (str, optional): The filename (without extension!) to use.
                (Also used for the log message, with underscores changed to
                spaces.)
            **info_kwargs: Additional kwargs that are to be stored in the meta-
                data dict.
        """
        cfg_dir = self.dirs["config"]
        write_yml(pspace, path=os.path.join(cfg_dir, f"{filename}.yml"))
        write_yml(dict(**pspace.get_info_dict(), **info_kwargs),
                  path=os.path.join(cfg_dir, f"{filename}_info.yml"))
        log.note("  Backed up %s and metadata.", filename.replace("_", " "))

    def _prepare_executable(self, *, run_from_tmpdir: bool=False) -> None:
        """Prepares the model executable, potentially copying it to a temporary
        location.

        Note that ``run_from_tmpdir`` requires the executable to be relocatable
        to another location, i.e. be position-independent.

        Args:
            run_from_tmpdir (bool, optional): Whether to copy the executable
                to a temporary directory that goes out of scope once the
                Multiverse instance goes out of scope.

        Raises:
            FileNotFoundError: On missing file at model binary path
            PermissionError: On wrong access rights of file at the binary path
        """
        binpath = self.info_bundle.paths['binary']

        # Make sure it exists and is executable
        if not os.path.isfile(binpath):
            raise FileNotFoundError("No file found at the specified binary "
                                    "path for model '{}'! Did you build it?\n"
                                    "Expected file at:  {}"
                                    "".format(self.model_name, binpath))

        elif not os.access(binpath, os.X_OK):
            raise PermissionError("The specified binary for model '{}' is not "
                                  "executable. Did you set the correct access "
                                  "rights?\nBinary path:  {}"
                                  "".format(self.model_name, binpath))

        if run_from_tmpdir:
            self._tmpdir = TemporaryDirectory(prefix=self.model_name)
            tmp_binpath = os.path.join(self._tmpdir.name,
                                       os.path.basename(binpath))

            log.info("Copying executable to temporary directory ...")
            log.debug("  Original:   %s", binpath)
            log.debug("  Temporary:  %s", tmp_binpath)
            copy2(binpath, tmp_binpath)
            binpath = tmp_binpath

        self._model_binpath = binpath

    def _resolve_cluster_params(self) -> dict:  # TODO Outsource!
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
            _missing = ", ".join([k for k in required if k not in resolved])
            raise ValueError(
                f"Missing required environment variable(s):  {_missing} ! "
                "Make sure that the corresponding environment variables are "
                "set and that the mapping is correct!\n"
                f"  Mapping for manager '{mngr}':\n{pformat(var_map)}\n\n"
                f"  Full environment:\n{pformat(env)}\n\n"
            )

        # Now do some postprocessing on some of the values
        # Ensure integers
        resolved['job_id'] = int(resolved['job_id'])
        resolved['num_nodes'] = int(resolved['num_nodes'])

        if 'num_procs' in resolved:
            resolved['num_procs'] = int(resolved['num_procs'])

        if 'timestamp' in resolved:
            resolved['timestamp'] = int(resolved['timestamp'])

        # Ensure reproducible node list format: ordered list
        parse_mode = self.cluster_params['node_list_parser_params'][mngr]
        try:
            node_list = parse_node_list(
                resolved['node_list'], mode=parse_mode, rcps=resolved
            )
        except Exception as exc:
            raise ValueError(
                f"Failed parsing node list {resolved['node_list']} into a "
                f"uniform format using parsing mode '{parse_mode}' and "
                f"cluster manager '{mngr}'! "
                "Check the cluster mode configuration, the relevant "
                "environment variables, and the chained error message.\n"
                f"Cluster parameters:\n{pformat(self.cluster_params)}\n\n"
                f"Parameters resolved so far:\n{pformat(resolved)}"
            ) from exc

        resolved['node_list'] = node_list

        # Calculated values, needed in Multiverse.run
        # node_index: the offset in the modulo operation
        resolved['node_index'] = node_list.index(resolved['node_name'])

        # Return the resolved values
        log.debug("Resolved cluster parameters:\n%s", pformat(resolved))
        return resolved

    def _add_sim_task(self, *, uni_id_str: str, uni_cfg: dict,
                      is_sweep: bool) -> None:
        """Helper function that handles task assignment to the WorkerManager.

        This function creates a WorkerTask that will perform the following
        actions **once it is grabbed and worked at**:

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
        def setup_universe(*, worker_kwargs: dict, model_name: str,
                           model_binpath: str, uni_cfg: dict,
                           uni_basename: str) -> dict:
            """The callable that will setup everything needed for a universe.

            This is called before the worker process starts working on the
            universe.

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
            # Create universe directory path using the basename
            uni_dir = os.path.join(self.dirs['data'], uni_basename)

            # Now create the folder
            os.mkdir(uni_dir)
            log.debug("Created universe directory:\n  %s", uni_dir)

            # Store it in the configuration
            uni_cfg['output_dir'] = uni_dir

            # Generate a path to the output hdf5 file and add it to the dict
            output_path = os.path.join(uni_dir, "data.h5")
            uni_cfg['output_path'] = output_path

            # Parse the potentially string-valued number of steps values, and
            # other step-like arguments. Raises an error if they are negative.
            uni_cfg['num_steps'] = parse_num_steps(uni_cfg['num_steps'])
            uni_cfg['write_every'] = parse_num_steps(uni_cfg['write_every'])
            uni_cfg['write_start'] = parse_num_steps(uni_cfg['write_start'])

            # write essential part of config to file:
            uni_cfg_path = os.path.join(uni_dir, "config.yml")
            write_yml(uni_cfg, path=uni_cfg_path)

            # Build args tuple for task assignment; only need to pass the path
            # to the configuration file ...
            args = (model_binpath, uni_cfg_path)

            # Generate a new worker_kwargs dict, carrying over the given ones
            wk = dict(args=args,
                      read_stdout=True,
                      stdout_parser="yaml_dict",
                      **(worker_kwargs if worker_kwargs else {}))

            # Determine whether to save the streams (True by default)
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

    def _add_sim_tasks(self, *, sweep: bool=None) -> int:
        """Adds the simulation tasks needed for a single run or for a sweep.

        Args:
            sweep (bool, optional): Whether tasks for a parameter sweep should
                be added or only for a single universe. If None, will read the
                ``perform_sweep`` key from the meta-configuration.

        Returns:
            int: The number of added tasks.

        Raises:
            ValueError: On ``sweep == True`` and zero-volume parameter space.
        """
        if sweep is None:
            sweep = self.meta_cfg.get('perform_sweep', False)

        pspace = self.meta_cfg['parameter_space']

        if not sweep:
            # Only need the default state of the parameter space
            uni_cfg = pspace.default

            # Make a backup of the parameter space that is *actually* used
            self._perform_pspace_backup(psp.ParamSpace(uni_cfg),
                                        filename="parameter_space",
                                        perform_sweep=False)

            # Add the task to the worker manager.
            log.progress("Adding task for simulation of a single universe ...")
            self._add_sim_task(uni_id_str="0", uni_cfg=uni_cfg, is_sweep=False)

            return 1
        # -- else: tasks for parameter sweep needed

        if pspace.volume < 1:
            raise ValueError("The parameter space has no sweeps configured! "
                             "Refusing to run a sweep. You can either call "
                             "the run_single method or add sweeps to your "
                             "run configuration using the !sweep YAML tags.")

        # Get the parameter space iterator and the number of already-existing
        # tasks (to later compute the number of _added_ tasks)
        psp_iter = pspace.iterator(with_info='state_no_str')
        _num_tasks = len(self.wm.tasks)

        # Distinguish whether to do a regular sweep or we are in cluster mode
        if not self.cluster_mode:
            # Make a backup of the parameter space that is *actually* used
            self._perform_pspace_backup(pspace,
                                        filename="parameter_space",
                                        perform_sweep=True)

            # Do a sweep over the whole activated parameter space
            log.progress("Adding tasks for simulation of %d universes ...",
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

            # Back up the actually-used parameter space. Do this only on the
            # first node to avoid file-writing conflicts between nodes
            if node_index == 0:
                self._perform_pspace_backup(pspace,
                                            filename="parameter_space",
                                            perform_sweep=True)

            # Inform about the number of universes to be simulated
            log.progress("Adding tasks for cluster-mode simulation of "
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

        num_new_tasks = len(self.wm.tasks) - _num_tasks
        log.info("Added %d tasks.", num_new_tasks)
        return num_new_tasks

    def _validate_meta_cfg(self) -> bool:
        """Goes through the parameters that require validation, validates them,
        and creates a useful error message if there were invalid parameters.

        Returns:
            bool: True if all parameters are valid; None if no check was done.
                Note that False will never be returned, but a ValidationError
                will be raised instead.

        Raises:
            ValidationError: If validation failed.
        """
        to_validate = self.meta_cfg.get('parameters_to_validate', {})

        if not (to_validate or self.meta_cfg.get('perform_validation', True)):
            log.info("Not performing parameter validation.")
            return None
        log.info("Validating %d parameters ...", len(to_validate))

        pspace = self.meta_cfg['parameter_space']
        log.remark("Parameter space volume:      %d", pspace.volume)

        if pspace.volume >= 1000:
            log.note("This may take a few seconds. To skip validation, set "
                     "the `perform_validation` flag to False.")

        # The dict to collect details on invalid parameters in.
        #   - Keys are key sequences (tuple of str)
        #   - Values are sets of error _messages_, hence suppressing duplicates
        invalid_params = defaultdict(set)

        # Iterate over the whole parameter space, including the default point
        # TODO Can improve performance by directly checking sweep dimensions.
        #      ... but be very careful about robustness here!
        for params in itertools.chain(pspace, [pspace.default]):
            for key_seq, param in to_validate.items():
                # Retrieve the value from this point in parameter space
                value = psp.tools.recursive_getitem(params, keys=key_seq)

                # Validate it and store the error _message_ if invalid
                try:
                    param.validate(value)

                except ValidationError as exc:
                    invalid_params[key_seq].add(str(exc))

        if not invalid_params:
            log.note("All parameters valid.")
            return True

        # else: Validation failed. Create an informative error message
        msg = (f"Validation failed for {len(invalid_params)} "
               f"parameter{'s' if len(invalid_params) > 1 else ''}:\n\n")

        # Get the length of longest key sequence (used for alignment)
        _nd = max([len(".".join(ks)) for ks in invalid_params.keys()])

        for key_seq, errs in invalid_params.items():
            path = ".".join(key_seq)

            if len(errs) == 1:
                msg += f"  - {path:<{_nd}s}  :  {list(errs)[0]}\n"
            else:
                _details = "\n".join([f"     - {e}" for e in errs])
                msg += (f"  - {path:<{_nd}s}  :  validation failed for "
                        f"{len(errs)} sweep values:\n{_details}\n")

        msg += ("\nInspect the details above and adjust the run configuration "
                "accordingly.\n")
        raise ValidationError(msg)


# -----------------------------------------------------------------------------
# -----------------------------------------------------------------------------


class FrozenMultiverse(Multiverse):
    """A frozen Multiverse is like a Multiverse, but frozen.

    It is initialized from a finished Multiverse run and re-creates all the
    attributes from that data, e.g.: the meta configuration, a DataManager,
    and a PlotManager.

    Note that it is no longer able to perform any simulations.
    """

    def __init__(self, *,
                 model_name: str=None, info_bundle: ModelInfoBundle=None,
                 run_dir: str=None, run_cfg_path: str=None,
                 user_cfg_path: str=None,
                 use_meta_cfg_from_run_dir: bool=False, **update_meta_cfg):
        """Initializes the FrozenMultiverse from a model name and the name
        of a run directory.

        Note that this also takes arguments to specify the run configuration to
        use.

        Args:
            model_name (str): The name of the model to load. From this, the
                model output directory is determined and the run_dir will be
                seen as relative to that directory.
            info_bundle (ModelInfoBundle, optional): The model information
                bundle that includes information about the binary path etc.
                If not given, will attempt to read it from the model registry.
            run_dir (str, optional): The run directory to load. Can be a path
                relative to the current working directory, an absolute path,
                or the timestamp of the run directory. If not given, will use
                the most recent timestamp.
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
        # First things first: get the info bundle
        self._info_bundle = get_info_bundle(model_name=model_name,
                                            info_bundle=info_bundle)
        log.progress("Initializing FrozenMultiverse for '%s' model ...",
                     self.model_name)

        # Initialize property-managed attributes
        self._meta_cfg = None
        self._dirs = dict()
        self._resolved_cluster_params = None

        # Decide whether to load the meta configuration from the given run
        # directory or the currently available one.
        if (    use_meta_cfg_from_run_dir
            and isinstance(run_dir, str)
            and os.path.isabs(run_dir)):

            raise NotImplementedError("use_meta_cfg_from_run_dir")

            # Find the meta config backup file and load it
            # Alternatively, create it from the singular backup files ...
            # log.info("Trying to load meta configuration from given absolute "
            #          "run directory ...")
            # Update it with the given update_meta_cfg dict

        else:
            # Need to create a meta configuration from the currently available
            # values.
            mcfg, _ = self._create_meta_cfg(run_cfg_path=run_cfg_path,
                                            user_cfg_path=user_cfg_path,
                                            update_meta_cfg=update_meta_cfg)

        # Only keep selected entries from the meta configuration. The rest is
        # not needed and is deleted in order to not confuse the user with
        # potentially varying versions of the meta config.
        self._meta_cfg = {k: v for k, v in mcfg.items()
                          if k in ('paths', 'data_manager', 'plot_manager',
                                   'cluster_mode', 'cluster_params')}

        # Need to make some DataManager adjustments; do so via update dicts
        dm_cluster_kwargs = dict()
        if self.cluster_mode:
            log.note("Cluster mode enabled.")
            self._resolved_cluster_params = self._resolve_cluster_params()
            rcps = self.resolved_cluster_params  # creates a deep copy

            log.note("This is node %d of %d.",
                     rcps['node_index'] + 1, rcps['num_nodes'])

            # Changes to the meta configuration
            # To avoid config file collisions in the PlotManager:
            self._meta_cfg['plot_manager']['cfg_exists_action'] = 'skip'

            # _Additional_ arguments to pass to DataManager.__init__ below
            timestamp = rcps['timestamp']
            dm_cluster_kwargs = dict(out_dir_kwargs=dict(timestamp=timestamp,
                                                         exist_ok=True))

        # Generate the path to the run directory that is to be loaded
        self._create_run_dir(**self.meta_cfg['paths'], run_dir=run_dir)
        log.note("Run directory:\n  %s", self.dirs['run'])

        # Create a data manager
        self._dm = DataManager(self.dirs['run'],
                               name=self.model_name + "_data",
                               **self.meta_cfg['data_manager'],
                               **dm_cluster_kwargs)
        log.progress("Initialized DataManager.")

        # Instantiate the PlotManager via the helper method
        self._pm = self._setup_pm()

        log.progress("Initialized FrozenMultiverse.\n")

    def _create_run_dir(self, *, out_dir: str, run_dir: str, **__):
        """Helper function to find the run directory from arguments given
        to :py:meth:`~utopya.multiverse.Multiverse.__init__`.

        Overwrites the method from the parent Multiverse class, because the
        FrozenMultiverse does not require setting up a *new* run directory but
        should instead identify the existing one and create an appropriate
        output directory.

        Args:
            out_dir (str): The output directory
            run_dir (str): The run directory to use
            ``**__``: ignored

        Raises:
            IOError: No directory found to use as run directory
            TypeError: When run_dir was not a string
        """
        # Create model directory path
        out_dir = os.path.expanduser(str(out_dir))
        model_dir = os.path.join(out_dir, self.model_name)

        # Distinguish different types of values for the run_dir argument
        if run_dir is None:
            log.info("Trying to identify most recent run directory ...")

            # Create list of _directories_ matching timestamp pattern
            dirs = [d for d in sorted(os.listdir(model_dir))
                    if  os.path.isdir(os.path.join(model_dir, d))
                    and re.match(r'\d{6}-\d{6}_?.*', os.path.basename(d))]

            # Use the latest to choose the run directory
            run_dir = os.path.join(model_dir, dirs[-1])

        elif isinstance(run_dir, str):
            run_dir = os.path.expanduser(run_dir)

            # Distinguish absolute and relative paths and those starting with
            # a timestamp-like pattern, which can be looked up from the model
            # directory.
            if os.path.isabs(run_dir):
                log.debug("Received absolute run_dir, using that one.")

            elif re.match(r'\d{6}-\d{6}_?.*', run_dir):
                # Is a timestamp, look relative to the model directory
                log.info("Received timestamp '%s' for run_dir; trying to find "
                         "one within the model output directory ...", run_dir)
                run_dir = os.path.join(model_dir, run_dir)

            else:
                # Is not an absolute path and not a timestamp; thus a path
                # relative to the current working directory
                run_dir = os.path.join(os.getcwd(), run_dir)

        else:
            raise TypeError("Argument run_dir needs to be None, an absolute "
                            "path, or a path relative to the model output "
                            f"directory, but it was: {run_dir}")

        # Check if the directory exists
        if not os.path.isdir(run_dir):
            raise IOError(f"No run directory found at '{run_dir}'!")

        # Store the path and associate the subdirectories
        self.dirs['run'] = run_dir

        for subdir in ('config', 'eval', 'data'):
            subdir_path = os.path.join(run_dir, subdir)
            self.dirs[subdir] = subdir_path
            # TODO Consider checking if it exists?
