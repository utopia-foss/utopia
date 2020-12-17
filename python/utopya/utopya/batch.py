"""Implements batch running and evaluation of simulations"""

import os
import logging
import time
from shutil import copy2 as _copy2
from copy import deepcopy as _deepcopy
from pkg_resources import resource_filename as _resource_filename
from typing import Dict, Tuple, Union, Sequence

from .yaml import load_yml as _load_yml, write_yml as _write_yml
from .tools import recursive_update as _recursive_update
from .cfg import load_from_cfg_dir as _load_from_cfg_dir
from .workermanager import WorkerManager
from .reporter import WorkerManagerReporter
from .task import MPProcessTask

_BTM_BASE_CFG = _load_yml(_resource_filename("utopya", "cfg/btm_cfg.yml"))
_BTM_USER_DEFAULTS = _load_from_cfg_dir("batch")
_BTM_DEFAULTS = _recursive_update(_deepcopy(_BTM_BASE_CFG),
                                  _deepcopy(_BTM_USER_DEFAULTS))

log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------
# .. Definition of multiprocessing.Process target callables ...................
# These need to happen here because they need to be importable from the module
# and not be local function definitions.

def _eval_task(
    *,
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
    from .model import Model

    model = Model(name=model_name, **model_kwargs)
    mv = model.create_frozen_mv(**frozen_mv_kwargs)

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
        # Parse and store all configuration options
        self._cfg = self._setup_batch_cfg(batch_cfg_path, **update_batch_cfg)

        # Associate a fixed timestamp string with this batch run
        self._timestamp_str = time.strftime(self.RUN_DIR_TIME_FSTR)

        # Setup directories and perform backup
        self._dirs = self._setup_dirs(**self._cfg["paths"])
        log.note("Batch run directory:\n  %s", self.dirs["batch_run"])

        self._perform_backup(batch_file=batch_cfg_path,
                             update_cfg=update_batch_cfg,
                             batch_cfg=self._cfg)

        # Some features are not yet implemented
        if self._cfg["cluster_mode"]:
            raise NotImplementedError("Cluster mode is not supported yet!")

        # Set up the WorkerManager and its reporter
        self._wm = WorkerManager(**self._cfg["worker_manager"])
        self._reporter = WorkerManagerReporter(
            self._wm,
            report_dir=self.dirs["batch_run"],
            **self._cfg['reporter'],
        )

        # All done.
        log.progress("Initialized BatchTaskManager.\n")


    # .........................................................................

    @property
    def debug(self) -> bool:
        """Whether debug mode was enabled."""
        return self._cfg["debug"]

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

    def perform_all_tasks(self):
        """Perform all run and eval tasks."""
        num_run_tasks = self._add_run_tasks()
        num_eval_tasks = self._add_eval_tasks()
        self._wm.start_working()
        log.success(f"Finished {num_run_tasks} run tasks and "
                    f"{num_eval_tasks} evaluation tasks.")

    def perform_run_tasks(self):
        """Performs the configured run tasks"""
        self._add_run_tasks()
        self._wm.start_working()
        log.success("Batch simulation runs finished.")

    def perform_eval_tasks(self):
        """Performs the configured evaluation tasks"""
        self._add_eval_tasks()
        self._wm.start_working()
        log.success("Batch evaluation finished.")


    # .........................................................................

    @staticmethod
    def _setup_batch_cfg(batch_cfg_path: str, **update_batch_cfg) -> dict:
        """Sets up the BatchTaskManager configuration"""
        # Update defaults with configuration from file
        batch_cfg = _load_yml(batch_cfg_path)
        batch_cfg = _recursive_update(_deepcopy(_BTM_DEFAULTS), batch_cfg)

        # Update again
        batch_cfg = _recursive_update(batch_cfg, _deepcopy(update_batch_cfg))

        # For debug mode, adjust some parameters
        if batch_cfg["debug"]:
            log.note("  Using debug mode.")

            batch_cfg["worker_manager"]["nonzero_exit_handling"] = "raise"

            eval_defaults = batch_cfg["task_defaults"]["eval"]
            eval_defaults["plot_manager"]["raise_exc"] = True

        return batch_cfg

    def _setup_dirs(self, out_dir: str, note: str = None) -> Dict[str, str]:
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
        subdirs = ("config", "eval", "run", "logs")
        for subdir in subdirs:
            dirs[subdir] = os.path.join(batch_run_dir, subdir)
            os.makedirs(dirs[subdir])

        return dirs

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
                log.debug("Copying %s config ...", part_name)
                _copy2(val, _path)

            else:
                log.debug("Dumping %s config dict ...", part_name)
                _write_yml(val, path=_path)

        log.note("  Backed up all involved configuration files.")

    # .........................................................................

    def _add_run_tasks(self) -> int:
        """Adds all configured run tasks to the WorkerManager's task queue"""
        run_tasks = _deepcopy(self._cfg["tasks"]["run"])

        log.progress("Adding %d simulation run tasks ...", len(run_tasks))

        for task_name, task_cfg in run_tasks.items():
            task_cfg = _recursive_update(self.run_defaults, task_cfg)
            self._add_run_task(task_name, **task_cfg)

        return len(run_tasks)

    def _add_eval_tasks(self) -> int:
        """Adds all configured evaluation tasks to the WorkerManager's task
        queue"""
        eval_tasks = _deepcopy(self._cfg["tasks"]["eval"])

        log.progress("Adding %d evaluation tasks ...", len(eval_tasks))

        for task_name, task_cfg in eval_tasks.items():
            task_cfg = _recursive_update(self.eval_defaults, task_cfg)
            self._add_eval_task(task_name, **task_cfg)

        return len(eval_tasks)

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
        create_symlink: bool = False,  # TODO
        enabled: bool = True,
        priority: int = None,
        **eval_task_kwargs
    ):
        """Adds a single evaluation task to the WorkerManager

        Args:
            name (str): Name of this task
            model_name (str): Model name; required in task, thus already
                requiring it here.
            out_dir (str): The path to the data output directory, i.e. the
                directory where all plots will ned up in.
                This may be a format string containing any of the following
                keys: ``task_name``, ``model_name``, ``timestamp``.
                Relative paths are evaluated relative to the ``eval`` batch
                run directory.
            enabled (bool, optional): If False, will *not* add this task.
            priority (int, optional): Task priority
            **eval_task_kwargs: All further eval task arguments.
        """
        def setup_eval_task(worker_kwargs: dict,
                            model_name: str,
                            out_dir: str,
                            create_symlink: bool,
                            eval_task_kwargs: dict) -> dict:
            """Run before the task starts; sets up all arguments for it ..."""
            # Prepare the DataManager's output directory path
            out_dir = out_dir.format(task_name=name, model_name=model_name,
                                     timestamp=self._timestamp_str)
            out_dir = os.path.expanduser(out_dir)

            if not os.path.isabs(out_dir):
                out_dir = os.path.join(self.dirs['eval'], out_dir)


            # Configure the data manager accordingly
            eval_task_kwargs = _recursive_update(
                eval_task_kwargs,
                dict(model_name=model_name, data_manager=dict(out_dir=out_dir))
            )

            # TODO Should also store full eval_task_kwargs to out_dir!!
            # TODO Symlink. Either here or in the process ...
            # FIXME The above probably both conflict with DataManager failing
            #       if the directory already exists.

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
                            create_symlink=create_symlink)

        # Add the task
        self._wm.add_task(
            TaskCls=MPProcessTask,
            name=name,
            priority=priority,
            setup_func=setup_eval_task,
            setup_kwargs=setup_kwargs,
            worker_kwargs=worker_kwargs,
        )
