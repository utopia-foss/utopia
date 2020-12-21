"""Implements batch running and evaluation of simulations"""

import os
import logging
import time
from shutil import copy2 as _copy2
from copy import deepcopy as _deepcopy
from pkg_resources import resource_filename as _resource_filename
from typing import Dict, Tuple, Union, Sequence, Callable

from .yaml import load_yml as _load_yml, write_yml as _write_yml
from .tools import (recursive_update as _recursive_update)
from .cfg import (
    load_from_cfg_dir as _load_from_cfg_dir,
    UTOPIA_CFG_FILE_PATHS as _UTOPIA_CFG_FILE_PATHS
)
from .workermanager import WorkerManager
from .reporter import WorkerManagerReporter
from .task import MPProcessTask

_BTM_BASE_CFG_PATH = _resource_filename("utopya", "cfg/btm_cfg.yml")
_BTM_BASE_CFG = _load_yml(_BTM_BASE_CFG_PATH)
_BTM_USER_DEFAULTS = _load_from_cfg_dir("batch")
_BTM_DEFAULTS = _recursive_update(_deepcopy(_BTM_BASE_CFG),
                                  _deepcopy(_BTM_USER_DEFAULTS))

# Substrings that may not appear in task names
INVALID_TASK_NAME_CHARS = ("/", ":", ".", "?", "*",)

log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------
# .. Definition of multiprocessing.Process target callables ...................
# These need to happen here because they need to be importable from the module
# and not be local function definitions.

def _eval_task(
    *,
    task_name: str,
    _batch_name: str,
    _batch_dirs: dict,
    _task_cfg_path: str,
    _create_symlinks: bool,
    model_name: str,
    model_kwargs: dict = {},
    print_tree: Union[bool, str] = 'condensed',
    plot_only: Sequence[str] = None,
    plots_cfg: str = None,
    update_plots_cfg: dict = {},
    **frozen_mv_kwargs,
):
    """The evaluation task target for the multiprocessing.Process. It sets up
    a :py:class:`utopya.model.Model`, loads the data, and performs plots.
    """
    import os
    from .model import Model

    log.hilight("Setting up evaluation task '%s' for model '%s' ...",
                task_name, model_name)

    model = Model(name=model_name, **model_kwargs)
    mv = model.create_frozen_mv(**frozen_mv_kwargs)

    # Create symlinks to improve crosslinking between related files/directories
    if _create_symlinks:
        log.progress("Creating symlinks ...")

        # ... back to the run directory, removing an existing one.
        _dst = os.path.join(mv.dm.dirs['out'], "_run")
        if os.path.islink(_dst):
            os.remove(_dst)
        os.symlink(mv.dm.dirs['data'], _dst)
        log.note("...to original run directory.")

        # ... from the evaluation output directory of the original run to the
        # evaluation data directory of this batch eval task.
        _dst = os.path.join(mv.dm.dirs['data'], "eval",
                            f"{_batch_name}_eval_{task_name}")
        os.symlink(mv.dm.dirs['out'], _dst)
        log.note("...from original run directory back to this evaluation "
                 "output directory.")

        # ... back to the task configuration file, removing an existing one.
        _dst = os.path.join(mv.dm.dirs['out'], "_task_cfg.yml")
        if os.path.islink(_dst):
            os.remove(_dst)
        os.symlink(_task_cfg_path, _dst)
        log.note("...to batch task configuration.")

        # With output directories that are *outside* of the batch run
        # directory, add links back and forth between them:
        if not mv.dm.dirs['out'].startswith(_batch_dirs['eval']):
            # ... back to the batch run directory
            _dst = os.path.join(mv.dm.dirs['out'], "_batch_run")
            if os.path.islink(_dst):
                os.remove(_dst)
            os.symlink(_batch_dirs['batch_run'], _dst)
            log.note("...to batch run directory.")

            # ... from default eval directory to custom one
            _dst = os.path.join(_batch_dirs['eval'], task_name)
            os.symlink(mv.dm.dirs['out'], _dst)
            log.note("...from batch evaluation output directory to custom "
                     "output directory.")

    print("")

    # Load the data tree
    mv.dm.load_from_cfg()

    if print_tree == 'condensed':
        print(mv.dm.tree_condensed)

    elif print_tree:
        print(mv.dm.tree)

    # Start plotting ...
    mv.pm.plot_from_cfg(plots_cfg=plots_cfg, plot_only=plot_only,
                        **update_plots_cfg)


# -----------------------------------------------------------------------------

class BatchTaskManager:
    """A manager for batch tasks"""

    # The time format string for the run directory
    RUN_DIR_TIME_FSTR = "%y%m%d-%H%M%S"

    # .........................................................................

    def __init__(self, *, batch_cfg_path: str = None, **update_batch_cfg):
        """Sets up a BatchTaskManager.

        Args:
            batch_cfg_path (str, optional): The batch file with all the task
                definitions.
            **update_batch_cfg: Additional arguments that are used to update
                the batch configuration.

        Raises:
            NotImplementedError: If ``run_tasks`` or ``cluster_mode`` were set
                in the batch configuration.
        """
        log.progress("Initializing BatchTaskManager ...")

        self._cfg = self._setup_batch_cfg(batch_cfg_path, **update_batch_cfg)
        self._timestamp_str = time.strftime(self.RUN_DIR_TIME_FSTR)

        self._dirs, self._name = self._setup_dirs(**self._cfg["paths"])
        log.note("Batch run directory:\n  %s", self.dirs["batch_run"])

        self._perform_backup(
            base_cfg=_BTM_BASE_CFG_PATH,
            user_cfg=_UTOPIA_CFG_FILE_PATHS.get('batch'),
            batch_file=batch_cfg_path,
            update_cfg=update_batch_cfg,
            batch_cfg=self._cfg,
        )

        # Some features are not yet implemented ...
        if self._cfg["cluster_mode"]:
            raise NotImplementedError("Cluster mode is not supported yet!")

        # Set up the WorkerManager and its reporter
        self._wm = WorkerManager(**self._cfg["worker_manager"])
        self._reporter = WorkerManagerReporter(
            self._wm,
            report_dir=self.dirs["batch_run"],
            **self._cfg['reporter'],
        )

        log.progress("Initialized BatchTaskManager.")
        log.note("  Parallelization level:     %s", self.parallelization_level)
        log.note("  Debug mode?                %s", self.debug)

    def __str__(self) -> str:
        return f"<BatchTaskManager '{self._name}'>"

    # .........................................................................

    @property
    def debug(self) -> bool:
        """Whether debug mode was enabled."""
        return self._cfg["debug"]

    @property
    def parallelization_level(self) -> str:
        return self._cfg["parallelization_level"]

    @property
    def run_defaults(self) -> dict:
        """A deepcopy of the run task defaults"""
        return _deepcopy(self._cfg["task_defaults"]["run"])

    @property
    def eval_defaults(self) -> dict:
        """A deepcopy of the eval task defaults"""
        return _deepcopy(self._cfg["task_defaults"]["eval"])

    @property
    def dirs(self) -> dict:
        """The directories associated with this BatchTaskManager"""
        return self._dirs


    # .........................................................................

    def perform_tasks(self):
        """Perform all run and eval tasks."""
        n_run = self._add_tasks(tasks=self._cfg["tasks"]["run"],
                                defaults=self.run_defaults,
                                add_task=self._add_run_task)
        n_eval = self._add_tasks(tasks=self._cfg["tasks"]["eval"],
                                 defaults=self.eval_defaults,
                                 add_task=self._add_eval_task)

        self._wm.start_working()
        log.success(
            "Finished %d run tasks and %d evaluation tasks.", n_run, n_eval
        )


    # .........................................................................

    @staticmethod
    def _setup_batch_cfg(batch_cfg_path: str, **update_batch_cfg) -> dict:
        """Sets up the BatchTaskManager configuration"""
        # Update defaults with configuration from file, if given
        batch_cfg = _load_yml(batch_cfg_path) if batch_cfg_path else {}
        batch_cfg = _recursive_update(_deepcopy(_BTM_DEFAULTS), batch_cfg)

        # Update again
        batch_cfg = _recursive_update(batch_cfg, _deepcopy(update_batch_cfg))

        # For debug mode, let the WorkerManager raise directly rather than only
        # issuing warnings (default).
        if batch_cfg["debug"]:
            batch_cfg["worker_manager"]["nonzero_exit_handling"] = "raise"

        # Evaluate parallelization level
        plevel = batch_cfg["parallelization_level"]

        if plevel == "task":
            batch_cfg["worker_manager"]["num_workers"] = 1
            batch_cfg["worker_kwargs"]["forward_streams"] = True

        elif plevel == "batch":
            task_defaults = batch_cfg["task_defaults"]

            task_defaults["run"] = _recursive_update(
                task_defaults["run"],
                dict(
                    worker_manager=dict(num_workers=1),
                    parameter_space=dict(
                        parallel_execution=dict(enabled=False)
                    )
                )
            )

        else:
            raise ValueError(
                f"Invalid parallelization_level '{plevel}'! "
                "Valid options are:  batch, task"
            )

        log.info("Loaded batch configuration.")
        return batch_cfg

    def _setup_dirs(self, out_dir: str, note: str = None
                    ) -> Tuple[Dict[str, str], str]:
        """Sets up directories"""
        out_dir = os.path.expanduser(out_dir)

        if not os.path.isabs(out_dir):
            raise ValueError(
                f"Batch output directory needs to be absolute! Was: {out_dir}"
            )

        # Batch run directory
        fstr = "{timestamp:}"
        if note:
            fstr += "_{note:}"
        batch_run_dir_name = fstr.format(timestamp=self._timestamp_str,
                                         note=note)
        batch_run_dir = os.path.join(out_dir, batch_run_dir_name)

        # Create batch run directory
        dirs = dict(out=out_dir, batch_run=batch_run_dir)
        os.makedirs(batch_run_dir)

        # Subdirectories
        subdirs = ("config", "config/tasks", "eval", "logs",)
        for subdir in subdirs:
            dirs[subdir] = os.path.join(batch_run_dir, subdir)
            os.makedirs(dirs[subdir])

        return dirs, batch_run_dir_name

    def _perform_backup(self, **parts):
        """Stores the given configuration parts in the config directory"""
        log.info("Performing backups ...")
        cfg_dir = self.dirs["config"]

        for part_name, val in parts.items():
            _path = os.path.join(cfg_dir, part_name + ".yml")

            # If string, assume it's a path and store original file, which
            # preserves the comments and structure etc. Otherwise store the
            # (most probably dict-like) data as a YAML dump.
            if isinstance(val, str):
                if not os.path.exists(val):
                    continue
                log.debug("Copying %s config ...", part_name)
                _copy2(val, _path)

            elif not val:
                log.debug("'%s' was empty, nothing to back up.", part_name)

            else:
                log.debug("Dumping %s config dict ...", part_name)
                _write_yml(val, path=_path)

        log.note("  Backed up all involved configuration files.")

    # .........................................................................

    def _add_tasks(self, tasks: dict, defaults: dict,
                   add_task: Callable) -> int:
        """Adds all configured run tasks to the WorkerManager's task queue"""
        tasks = _deepcopy(tasks)

        for task_name, task_cfg in tasks.items():
            if any(s in task_name for s in INVALID_TASK_NAME_CHARS):
                raise ValueError(
                    f"Invalid task name '{task_name}'! May not contain any of "
                    "the following characters or substrings:  "
                    + " ".join(INVALID_TASK_NAME_CHARS)
                )

            task_cfg = _recursive_update(_deepcopy(defaults), task_cfg)
            add_task(task_name, **task_cfg)

        return len(tasks)

    def _add_run_task(self, name: str, **_):
        """Adds a single run task to the WorkerManager"""
        raise NotImplementedError(
            "Run tasks are not supported yet! Remove the corresponding "
            "entries from the BatchTaskManager configuration."
        )

    def _add_eval_task(
        self,
        name: str,
        *,
        model_name: str,
        out_dir: str,
        enabled: bool = True,
        priority: int = None,
        create_symlinks: bool = False,
        **eval_task_kwargs
    ):
        """Adds a single evaluation task to the WorkerManager.

        Args:
            name (str): Name of this task
            model_name (str): Model name; required in task, thus already
                requiring it here.
            out_dir (str): The path to the data output directory, i.e. the
                directory where all plots will ned up in.
                This may be a format string containing any of the following
                keys: ``task_name``, ``model_name``, ``timestamp``,
                ``batch_name`` (combination of ``timestamp`` and the note).
                Relative paths are evaluated relative to the ``eval`` batch
                run directory.
            enabled (bool, optional): If False, will *not* add this task.
            priority (int, optional): Task priority; tasks with smaller value
                will be picked first.
            create_symlinks (bool, optional): Whether to create symlinks that
                add crosslinks between related directories, e.g.: from the
                output directory, link back to the task configuration; from the
                evaluation output directory alongside the simulation data, link
                to the batch output directory
            **eval_task_kwargs: All further evaluation task arguments.
        """
        def setup_eval_task(worker_kwargs: dict,
                            model_name: str,
                            out_dir: str,
                            create_symlinks: bool,
                            eval_task_kwargs: dict) -> dict:
            """Run before the task starts; sets up all arguments for it ..."""
            # Prepare the DataManager's output directory path
            out_dir = out_dir.format(
                task_name=name,
                model_name=model_name,
                timestamp=self._timestamp_str,
                batch_name=self._name,
            )
            out_dir = os.path.expanduser(out_dir)

            if not os.path.isabs(out_dir):
                out_dir = os.path.join(self.dirs['eval'], out_dir)

            # Update the task arguments accordingly, setting the DataManager's
            # output directory and adding metadata
            eval_task_kwargs = _recursive_update(
                eval_task_kwargs,
                dict(
                    _batch_name=self._name,
                    task_name=name,
                    model_name=model_name,
                    data_manager=dict(out_dir=out_dir)
                )
            )

            # Backup the configuration, such that the task can create a symlink
            # back to it. This needs to be done by the task, because the
            # DataManager will create the output directory and will fail if it
            # already exists (which is good).
            task_cfg_path = os.path.join(self.dirs["config/tasks"],
                                         f"eval_{name}.yml")
            _write_yml(eval_task_kwargs, path=task_cfg_path)
            eval_task_kwargs["_batch_dirs"] = self.dirs
            eval_task_kwargs["_task_cfg_path"] = task_cfg_path
            eval_task_kwargs["_create_symlinks"] = create_symlinks

            # Generate a new worker_kwargs dict, carrying over the given ones
            worker_kwargs = dict(
                args=(_eval_task,),
                read_stdout=True,
                stdout_parser='yaml_dict',
                **worker_kwargs,
            )
            worker_kwargs["popen_kwargs"]["kwargs"] = eval_task_kwargs

            # If the streams are to be saved, save them to the logs output
            # directory inside the batch directory.
            if worker_kwargs.get('save_streams', True):
                worker_kwargs['save_streams_to'] = os.path.join(
                    self.dirs['logs'], f"eval_{name}_" + "{name:}.log"
                )

            return worker_kwargs

        if not enabled:
            log.debug("Task '%s' was not enabled. Skipping.", name)
            return

        # Determine setup and worker kwargs
        worker_kwargs = _deepcopy(self._cfg['worker_kwargs'])
        setup_kwargs = dict(model_name=model_name,
                            out_dir=out_dir,
                            eval_task_kwargs=eval_task_kwargs,
                            create_symlinks=create_symlinks)

        # Add the task
        self._wm.add_task(
            TaskCls=MPProcessTask,
            name=name,
            priority=priority,
            setup_func=setup_eval_task,
            setup_kwargs=setup_kwargs,
            worker_kwargs=worker_kwargs,
        )
