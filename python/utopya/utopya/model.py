"""Provides the Model class to work interactively with Utopia models"""

import os
import glob
import logging
import warnings
from tempfile import TemporaryDirectory
from typing import List, Tuple, Dict

from dantro.tools import make_columns as _make_columns
from dantro.tools import adjusted_log_levels as _adjusted_log_levels

from .cfg import load_from_cfg_dir
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
        cfg, _, _ = load_model_cfg(info_bundle=self.info_bundle)
        return cfg

    @property
    def default_config_set_search_dirs(self) -> List[str]:
        """Returns the default config set search directories for this model
        in the order of precedence.

        .. note::

            These *may* be relative paths.
        """
        search_dirs = []

        # User-specified search directories, potentially format strings
        utopya_cfg = load_from_cfg_dir("utopya")
        _cs_dirs = utopya_cfg.get("config_set_search_dirs", [])
        if isinstance(_cs_dirs, list):
            search_dirs += [d.format(model_name=self.name) for d in _cs_dirs]
        else:
            raise TypeError(
                "The `config_set_search_dirs` key of the utopya configuration "
                f"needs to be a list! Got: {type(_cs_dirs)} {repr(_cs_dirs)}"
            )

        # Model source directory
        _model_cfgs_dir = os.path.join(
            os.path.dirname(self.info_bundle.paths["default_cfg"]), "cfgs"
        )
        search_dirs.append(_model_cfgs_dir)

        return search_dirs

    @property
    def default_config_sets(self) -> Dict[str, dict]:
        """Config sets at the default search locations.

        To retrieve an *individual* config set, consider using
        :py:meth:`~utopya.model.Model.get_config_set` instead of this property.

        For more information, see :ref:`config_sets`.
        """
        return self.get_config_sets(
            search_dirs=self.default_config_set_search_dirs
        )

    # Simulation control ......................................................

    def create_mv(self, *,
                  from_cfg: str=None,
                  from_cfg_set: str=None,
                  run_cfg_path: str=None,
                  use_tmpdir: bool=None, **update_meta_cfg) -> Multiverse:
        """Creates a :class:`utopya.multiverse.Multiverse` for this model,
        optionally loading a configuration from a file and updating it with
        further keys.

        Args:
            from_cfg (str, optional): The name of the config file (relative to
                the base directory) to be used.
            from_cfg_set (str, optional): Name of the config set to retrieve
                the run config from. Mutually exclusive with ``from_cfg`` and
                ``run_cfg_path``.
            run_cfg_path (str, optional): The path of the run config to use.
                Can not be passed if ``from_cfg`` or ``from_cfg_set`` arguments
                were given.
            use_tmpdir (bool, optional): Whether to use a temporary directory
                to write the data to. If not given, uses default value set
                at initialization.
            **update_meta_cfg: Can be used to update the meta configuration

        Returns:
            Multiverse: The created Multiverse object

        Raises:
            ValueError: If more than one of the run config selecting arguments
                (``from_cfg``, ``from_cfg_set``, ``run_cfg_path``) were given.
        """
        # A dict that can be filled with objects to store in self._mvs
        objs_to_store = dict()

        # May want to use config file relative to base directory or a run
        # directory from a config set. Need to check that only one of them
        # was given.
        if bool(from_cfg) + bool(from_cfg_set) + bool(run_cfg_path) > 1:
            raise ValueError(
                "Can pass at most one of the arguments `from_cfg`, "
                "`from_cfg_set`, or `run_cfg_path` but got more than one!"
            )

        if from_cfg:
            if os.path.isabs(from_cfg) and not self.base_dir:
                raise ValueError("Missing base_dir to handle relative path in "
                                 "`from_cfg` argument.")

            run_cfg_path = os.path.join(self.base_dir, from_cfg)

        elif from_cfg_set:
            run_cfg_path = self.get_config_set(from_cfg_set)["run"]

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

    def create_run_load(self, *,
                        from_cfg: str=None,
                        run_cfg_path: str=None,
                        from_cfg_set: str=None,
                        use_tmpdir: bool=None, print_tree: bool=True,
                        **update_meta_cfg) -> Tuple[Multiverse, DataManager]:
        """Chains the create_mv, mv.run, and mv.dm.load_from_cfg
        methods together and returns a (Multiverse, DataManager) tuple.

        Args:
            from_cfg (str, optional): The name of the config file (relative to
                the base directory) to be used.
            from_cfg_set (str, optional): Name of the config set to retrieve
                the run config from. Mutually exclusive with ``from_cfg`` and
                ``run_cfg_path``.
            run_cfg_path (str, optional): The path of the run config to use.
                Can not be passed if ``from_cfg`` or ``from_cfg_set`` arguments
                were given.
            use_tmpdir (bool, optional): Whether to use a temporary directory
                to write the data to. If not given, uses default value set
                at initialization.
            print_tree (bool, optional): Whether to print the loaded data tree
            **update_meta_cfg: Arguments passed to the create_mv function

        Returns:
            Tuple[Multiverse, DataManager]:
        """
        mv = self.create_mv(from_cfg=from_cfg,
                            from_cfg_set=from_cfg_set,
                            run_cfg_path=run_cfg_path,
                            use_tmpdir=use_tmpdir,
                            **update_meta_cfg)
        mv.run()
        mv.dm.load_from_cfg(print_tree=print_tree)
        return mv, mv.dm


    # Config set retrieval ....................................................

    def get_config_set(self, name: str=None) -> Dict[str, str]:
        """Returns a configuration set: a dict containing paths to run and/or
        eval configuration files. These are accessible via the keys ``run``
        and ``eval``.

        Config sets are retrieved from multiple locations:

            *  The ``cfgs`` directory in the model's source directory
            *  The user-specified lookup directories, specified in the utopya
               configuration as ``config_set_search_dirs``
            *  If ``name`` is an absolute or relative path, and a directory
               exists at the specified location, the parent directory is
               interpreted as a search path.

        This uses :py:meth:`~utopya.model.Model.get_config_sets` to retrieve
        all available configuration sets from the above paths and then selects
        the one with the given ``name``.
        Config sets that are found later overwrite those with the same name
        found in previous searches and log a warning message (which can be
        controlled with the ``warn`` argument); sets are *not* merged.

        For more information, see :ref:`config_sets`.

        Args:
            name (str, optional): The name of the config set to retrieve. This
                may also be a local path, which is looked up prior to the
                default search directories.
        """
        search_dirs = []

        # The name argument may be path-like, and we may want to include that
        # as well, but only if the directory actually exists.
        # In that case, the last path segment should be regarded as the name.
        _path = os.path.normpath(os.path.expanduser(name))
        if not os.path.isabs(_path):
            _path = os.path.abspath(_path)

        # However, we need to robustly identify the actual config set name from
        # the path in such a case.
        # Simplest assumption: check if such a directory actually exists, in
        # which case the user probably wanted to search it for a config set.
        # If there is a typo in the path, the whole directory will be searched
        # in the error message anyway ...
        if os.path.isdir(_path):
            name = os.path.basename(_path)
            search_dirs.append(os.path.dirname(_path))
        else:
            log.remark(
                "Given config set name was not interpretable as an "
                "additional search path (or the directory does not exist)."
            )

        # Append the default search directories, then start searching
        search_dirs += self.default_config_set_search_dirs

        for search_dir in search_dirs:
            cfg_sets = self._find_config_sets(search_dir, cfg_sets={})

            if name not in cfg_sets:
                continue

            cfg_set = cfg_sets[name]
            log.note(
                "Found config set named '%s' (contains: %s).",
                name,
                ", ".join(f"{k}.yml" for k in cfg_set.keys() if k != "dir"),
            )
            return cfg_set

        # else: did not find the config set.
        # Provide a useful error message, in which the local directory is
        # *always* searched, such that the user does not have to check manually
        # whether the directory exists but simply sees in this error message
        # whether there was a typo in `name`.
        search_dirs = (
            [os.path.dirname(_path)] + self.default_config_set_search_dirs
        )
        _search_dirs = "\n".join(f"  - {s}" for s in search_dirs)

        with _adjusted_log_levels(("utopya.model", logging.WARNING)):
            _avail = self.get_config_sets(search_dirs=search_dirs, warn=False)

        raise ValueError(
            f"No config set with name '{name}' could be found in any of the "
            f"following search directories:\n{_search_dirs}\n\n"
            "Check that a subdirectory of the desired name exists at any of "
            "the above locations and contains a `run.yml` and/or `eval.yml` "
            "file.\n"
            "Available config sets:\n"
            f"{_make_columns(_avail) if _avail else '  (none available)'}"
        )

    def get_config_sets(
        self,
        *,
        search_dirs: List[str] = None,
        warn: bool = True,
        cfg_sets: dict = None,
    ) -> Dict[str, dict]:
        """Searches for all available configuration sets in the given search
        directories, aggregating them into one dict.

        The search is done in *reverse* order of the paths given in
        ``search_dirs``, i.e. starting from those directories with the lowest
        precedence. If configuration sets with the same name are encountered,
        warnings are emitted, but the one with higher precedence (appearing
        more towards the front of ``search_dirs``, i.e. the later-searched one)
        will take precedence.

        .. note::

            This will *not* merge configuration sets from different search
            directories, e.g. if one contained only an eval configuration and
            the other contained only a run configuration, a warning will be
            emitted but the entry from the later-searched directory will be
            used.

        Args:
            search_dirs (List[str], optional): The directories to search
                sequentially for config sets. If not given, will use the
                default config set search directories, see
                :py:attr:`~utopya.model.Model.default_config_set_search_dirs`.
            warn (bool, optional): Whether to warn (via log message), if the
                search yields a config set with a name that already existed.
            cfg_sets (dict, optional): If given, aggregate newly found config
                sets into this dict. Otherwise, start with an empty one.
        """
        if search_dirs is None:
            search_dirs = self.default_config_set_search_dirs

        cfg_sets = cfg_sets if cfg_sets is not None else dict()

        for search_dir in reversed(search_dirs):
            cfg_sets = self._find_config_sets(
                search_dir, cfg_sets=cfg_sets, warn=warn
            )
        return cfg_sets


    # Helpers .................................................................

    def _store_mv(self, mv: Multiverse, **kwargs) -> None:
        """Stores a created Multiverse object and all the kwargs in a dict"""
        self._mvs.append(dict(mv=mv, **kwargs))

    def _create_tmpdir(self) -> TemporaryDirectory:
        """Create a TemporaryDirectory"""
        return TemporaryDirectory(prefix=self.name,
                                  suffix="_mv{}".format(len(self._mvs)))


    def _find_config_sets(
        self, search_dir: str, *, cfg_sets: dict, warn: bool = True
    ) -> Dict[str, dict]:
        """Looks for config sets in the given directory and aggregates them
        into the given ``cfg_sets`` dict, warning if an entry already exists.

        Args:
            search_dir (str): The directory to search for configuration sets.
                Can be an absolute or relative path; ``~`` is expanded.
            cfg_sets (dict): The dict to populate with the results, each entry
                being one config set.
            warn (bool, optional): Whether to warn (via log message) if an
                entry already exists.
        """
        # Make absolute
        search_dir = os.path.expanduser(search_dir)
        if not os.path.isabs(search_dir):
            search_dir = os.path.abspath(search_dir)

        log.remark("Searching for config sets in:\n  %s", search_dir)

        if not os.path.isdir(search_dir):
            log.remark("No directory found at given search path.")
            return cfg_sets

        dn = 0
        for cs_name in os.listdir(search_dir):
            log.debug("Inspecting subdirectory '%s' ...", cs_name)

            search_subdir = os.path.join(search_dir, cs_name)
            found_cfgs = dict()

            # Run configuration
            run_cfg = os.path.join(search_subdir, "run.yml")
            if os.path.exists(run_cfg):
                found_cfgs["run"] = run_cfg

            # Eval configuration
            eval_cfg = os.path.join(search_subdir, "eval.yml")
            if os.path.exists(eval_cfg):
                found_cfgs["eval"] = eval_cfg

            # Only add entry, if configs were found
            if found_cfgs:
                dn += 1
                if (
                    warn and
                    cs_name in cfg_sets and
                    search_subdir != cfg_sets[cs_name]["dir"]
                ):
                    log.caution(
                        "A config set named '%s' was already found at:\n  %s\n"
                        "It will be overwritten with the one found at:\n  %s",
                        cs_name, cfg_sets[cs_name]["dir"], search_subdir
                    )

                cfg_sets[cs_name] = dict(dir=search_subdir, **found_cfgs)

        log.remark("Found %d config set%s.", dn, "s" if dn != 1 else "")
        return cfg_sets
