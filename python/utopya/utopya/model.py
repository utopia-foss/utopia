"""Provides the Model class to work interactively with Utopia models"""

import os
import glob
import logging
from tempfile import TemporaryDirectory
from typing import Tuple, Dict

from .model_registry import ModelInfoBundle, get_info_bundle, load_model_cfg
from .multiverse import Multiverse, FrozenMultiverse
from .datamanager import DataManager

# Get a logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

class Model:
    """A class to work with Utopia models interactively.

    It attaches to a certain model and makes it easy to load config files,
    create a Multiverse from them, run it, and work with it further...
    """

    def __init__(self, *, name: str=None, info_bundle: ModelInfoBundle=None,
                 base_dir: str=None, sim_errors: str=None,
                 use_tmpdir: bool=False):
        """Initialize the ModelTest for the given model name

        Args:
            name (str, optional): Name of the model to attach to. If not
                given, need to pass info_bundle.
            info_bundle (ModelInfoBundle, optional): The required information
                to work with this model. If not given, will attempt to find the
                model in the model registry via ``name``.
            base_dir (str, optional): For convenience, can specify this path
                which will be seen as the base path for config files; if set,
                arguments that allow specifying configuration files can specify
                them relative to this directory.
            sim_errors (str, optional): Whether to raise errors from Multiverse
            use_tmpdir (bool, optional): Whether to use a temporary directory
                to write data to. The default value can be set here; but the
                flag can be overwritten in the create_mv and create_run_load
                methods. For ``false``, the regular model output
                directory is used.

        Raises:
            ValueError: Upon bad base_dir
        """
        # Determine model info bundle to use (via Multiverse class method)
        self._info_bundle = get_info_bundle(model_name=name,
                                            info_bundle=info_bundle)
        log.progress("Initializing '%s' model ...", self.name)

        # Store other attributes
        self._sim_errors = sim_errors
        self._use_tmpdir = use_tmpdir

        self._base_dir = ""
        if base_dir:
            base_dir = os.path.expanduser(base_dir)

            if not os.path.isabs(base_dir):
                raise ValueError("Given base_dir path {} should be absolute, "
                                 "but was not!".format(base_dir))

            elif not os.path.exists(base_dir) or not os.path.isdir(base_dir):
                raise ValueError("Given base_dir path {} does not seem to "
                                 "exist or is not a directory!"
                                 "".format(base_dir))

            self._base_dir = base_dir

        # Need to store Multiverses etc. such that they don't go out of scope
        self._mvs = []

    def __str__(self) -> str:
        """Returns an informative string for this Model instance"""
        return "<Utopia '{}' model>".format(self.name)

    # Properties ..............................................................

    @property
    def info_bundle(self) -> ModelInfoBundle:
        """The model info bundle"""
        return self._info_bundle

    @property
    def name(self) -> str:
        """The name of this Model object, which is at the same time the name
        of the attached model.
        """
        return self.info_bundle.model_name

    @property
    def base_dir(self) -> str:
        """Returns the path to the base directory, if set during init.

        This is the path to a directory from which config files can be loaded
        using relative paths.
        """
        return self._base_dir

    @property
    def default_model_cfg(self) -> dict:
        """Returns the default model configuration by loading it from the file
        specified in the info bundle.
        """
        cfg, _ = load_model_cfg(info_bundle=self.info_bundle)
        return cfg

    def run_and_eval_cfg_paths(self, *, search_dir: str='cfgs'
                               ) -> Dict[str, dict]:
        """Searches a directory, ``search_dir``, relative to the model's source
        directory, for run- and eval configuration pairs.
        Each pair is expected to be in a subdirectory within the given search
        directory.

        For each of these subdirectories, a dict entry with the paths for the
        specified ``run`` and ``eval`` configurations is created.
        These are detected by their file name suffix, i.e. ``*run.yml`` and
        ``*eval.yml``, respectively. Both are optional, but if none of them
        was found, the directory will not have an entry in the returned dict.

        Args:
            search_dir (str, optional): Path to the directory to search. If
                this is a relative path, will look relative to the directory
                the default model configuration is located in.

        Returns:
            Dict[str, dict]: Listed under the name of each subdirectory, the
                values of the returned dict contain at least one path to a
                ``run`` configuration and, if available, a path to an ``eval``
                configuration.
                If no files were found, the dict is empty.

        Raises:
            ValueError: If more than one run or eval configuration was found
                inside any of the subdirectories.
        """
        # Construct the absolute search path
        search_dir = os.path.expanduser(search_dir)
        if not os.path.isabs(search_dir):
            dcfg_dir = os.path.dirname(self.info_bundle.paths['default_cfg'])
            search_dir = os.path.join(dcfg_dir, search_dir)

        if not os.path.isdir(search_dir):
            # Search directory does not exist, just return an empty dict
            return dict()

        # Go through subdirectories and aggregate information
        log.info("Searching for run and eval configurations in %s ...",
                 search_dir)
        cfgs = dict()
        for cfg_name in os.listdir(search_dir):
            log.debug("Looking for run and eval configurations in '%s' "
                      "subdirectory ...", cfg_name)

            subdir_abs_path = os.path.join(search_dir, cfg_name)
            found_cfgs = dict()

            # First: run configuration
            run_cfgs = glob.glob(os.path.join(subdir_abs_path, '*run.yml'))

            if len(run_cfgs) > 1:
                raise ValueError("Can have at most one `*run.yml`-named file "
                                 "for a run configuration, got: {} in {} !"
                                 "".format(run_cfgs, subdir_abs_path))
            elif len(run_cfgs) == 1:
                found_cfgs['run'] = run_cfgs[0]

            # Now: eval configuration
            eval_cfgs = glob.glob(os.path.join(subdir_abs_path, '*eval.yml'))
            if len(eval_cfgs) > 1:
                raise ValueError("Can have at most one `*eval.yml`-named file "
                                 "for an evaluation configuration, got: {} in "
                                 "{} !".format(eval_cfgs, subdir_abs_path))
            elif len(eval_cfgs) == 1:
                found_cfgs['eval'] = eval_cfgs[0]

            # Add them
            if found_cfgs:
                cfgs[cfg_name] = found_cfgs

        log.info("Found %d configuration entries for '%s' model.",
                 len(cfgs), self.name)
        return cfgs

    # Public methods ..........................................................

    def create_mv(self, *, from_cfg: str=None, run_cfg_path: str=None,
                  use_tmpdir: bool=None, **update_meta_cfg) -> Multiverse:
        """Creates a :class:`utopya.Multiverse` for this model, optionally
        loading a configuration from a file and updating it with further keys.

        Args:
            from_cfg (str, optional): The name of the config file (relative to
                the base directory) to be used.
            run_cfg_path (str, optional): The path of the run_cfg to use. Can
                not be passed if from_cfg argument evaluates to True.
            use_tmpdir (bool, optional): Whether to use a temporary directory
                to write the data to. If not given, uses default value set
                at initialization.
            **update_meta_cfg: Can be used to update the meta configuration

        Returns:
            Multiverse: The created Multiverse object

        Raises:
            ValueError: If both from_cfg and run_cfg_path were given
        """
        # A dict that can be filled with objects to store in self._mvs
        objs_to_store = dict()

        # May want to use config file relative to base directory
        if from_cfg:
            if run_cfg_path:
                raise ValueError("Can only pass either argument `from_cfg` OR "
                                 "`run_cfg_path`, but got both!")

            elif os.path.isabs(from_cfg) and not self.base_dir:
                raise ValueError("Missing base_dir to handle relative path in "
                                 "`from_cfg` argument.")

            run_cfg_path = os.path.join(self.base_dir, from_cfg)

        # Check whether a temporary directory is desired
        use_tmpdir = use_tmpdir if use_tmpdir is not None else self._use_tmpdir
        if use_tmpdir:
            tmpdir = self._create_tmpdir()
            objs_to_store['out_dir'] = tmpdir

            # Use update_meta_cfg to communicate it to the Multiverse
            if 'paths' not in update_meta_cfg:
                update_meta_cfg['paths'] = dict(out_dir=tmpdir.name)
            else:
                update_meta_cfg['paths']['out_dir'] = tmpdir.name

        # Also set the exit handling value, if not already set
        # TODO do this in a more elegant way
        _se = self._sim_errors
        if _se and 'worker_manager' not in update_meta_cfg:
            update_meta_cfg['worker_manager'] = dict(nonzero_exit_handling=_se)

        elif _se and 'nonzero_exit_handling' not in update_meta_cfg['worker_manager']:
            update_meta_cfg['worker_manager']['nonzero_exit_handling'] = _se

        # else: entry was already set; don't set it again

        # Create the Multiverse and store it, to not let it go out of scope
        mv = Multiverse(model_name=self.name,
                        run_cfg_path=run_cfg_path,
                        **update_meta_cfg)
        self._store_mv(mv, **objs_to_store)

        return mv

    def create_frozen_mv(self, **fmv_kwargs) -> FrozenMultiverse:
        """Create a :class:`utopya.multiverse.FrozenMultiverse`, coupling it
        to a run directory.

        Use this method if you want to load an existing simulation run.

        Args:
            **fmv_kwargs: Passed on to FrozenMultiverse.__init__
        """
        mv = FrozenMultiverse(model_name=self.name, **fmv_kwargs)
        self._store_mv(mv)

        return mv

    def create_run_load(self, *, from_cfg: str=None, run_cfg_path: str=None,
                        use_tmpdir: bool=None, print_tree: bool=True,
                        **update_meta_cfg) -> Tuple[Multiverse, DataManager]:
        """Chains the create_mv, mv.run, and mv.dm.load_from_cfg
        methods together and returns a (Multiverse, DataManager) tuple.

        Args:
            from_cfg (str, optional): The name of the config file (relative to
                the base directory) to be used.
            run_cfg_path (str, optional): The path of the run_cfg to use. Can
                not be passed if from_cfg argument evaluates to True.
            use_tmpdir (bool, optional): Whether to use a temporary directory
                to write the data to. If not given, uses default value set
                at initialization.
            print_tree (bool, optional): Whether to print the loaded data tree
            **update_meta_cfg: Arguments passed to the create_mv function

        Returns:
            Tuple[Multiverse, DataManager]:
        """
        mv = self.create_mv(from_cfg=from_cfg,
                            run_cfg_path=run_cfg_path,
                            use_tmpdir=use_tmpdir,
                            **update_meta_cfg)
        mv.run()
        mv.dm.load_from_cfg(print_tree=print_tree)
        return mv, mv.dm


    # Helpers .................................................................

    def _store_mv(self, mv: Multiverse, **kwargs) -> None:
        """Stores a created Multiverse object and all the kwargs in a dict"""
        self._mvs.append(dict(mv=mv, **kwargs))

    def _create_tmpdir(self) -> TemporaryDirectory:
        """Create a TemporaryDirectory"""
        return TemporaryDirectory(prefix=self.name,
                                  suffix="_mv{}".format(len(self._mvs)))
