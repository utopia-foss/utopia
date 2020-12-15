"""Implements batch running and evaluation of simulations"""

import os
import logging
import time
from shutil import copy2 as _copy2
from copy import deepcopy as _deepcopy
from pkg_resources import resource_filename as _resource_filename
from typing import Dict, Tuple

from .yaml import load_yml as _load_yml, write_yml as _write_yml
from .tools import recursive_update as _recursive_update
from .cfg import load_from_cfg_dir as _load_from_cfg_dir
from .workermanager import WorkerManager
from .reporter import WorkerManagerReporter

_BTM_BASE_CFG = _load_yml(_resource_filename("utopya", "cfg/btm_cfg.yml"))
_BTM_USER_DEFAULTS = _load_from_cfg_dir("batch")
_BTM_DEFAULTS = _recursive_update(_deepcopy(_BTM_BASE_CFG),
                                  _deepcopy(_BTM_USER_DEFAULTS))

log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

class BatchTaskManager:
    """A manager for batch tasks"""

    # The time format string for the run directory
    RUN_DIR_TIME_FSTR = "%y%m%d-%H%M%S"

    # .........................................................................

    def __init__(self, *, utopia_cli: str, batch_cfg_path: str = None,
                 **update_batch_cfg):
        """Sets up a BatchTaskManager.

        Args:
            utopia_cli (str): Path to the Utopia executable, i.e.: the CLI
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

        # Setup directories and perform backup
        self._dirs = self._setup_dirs(**self._cfg["paths"])
        log.note("Batch run directory:\n  %s", self._dirs["batch_run"])

        self._perform_backup(batch_file=batch_cfg_path,
                             update_cfg=update_batch_cfg,
                             batch_cfg=self._cfg)

        # Some features are not yet implemented
        if self._cfg["run_tasks"]:
            raise NotImplementedError(
                "Run tasks are not supported yet! Remove the corresponding "
                "entries from the BatchTaskManager configuration."
            )

        if self._cfg["cluster_mode"]:
            raise NotImplementedError("Cluster mode is not supported yet!")

        # Need the path to the Utopia CLI
        self._utopia_cli = utopia_cli

        # Set up the WorkerManager and its reporter
        self._wm = WorkerManager(**self._cfg["worker_manager"])
        self._reporter = WorkerManagerReporter(
            self._wm,
            report_dir=self.dirs["batch_run"],
            **self._cfg['reporter'],
        )

        # All done.
        log.info("BatchTaskManager set up.")


    # .........................................................................

    @property
    def debug(self) -> bool:
        """Whether debug mode was enabled."""
        return self._cfg["debug"]

    @property
    def eval_defaults(self) -> dict:
        """A deepcopy of the eval task defaults"""
        return _deepcopy(self._cfg["task_defaults"]["eval"])

    # .........................................................................

    def perform_run_tasks(self):
        """Performs the configured run tasks"""
        raise NotImplementedError("perform_run_tasks")

    def perform_eval_tasks(self):
        """Performs the configured evaluation tasks"""
        for task_name, task_cfg in self._cfg["eval_tasks"].items():
            task_cfg = _recursive_update(self.eval_defaults, task_cfg)
            self._add_eval_task(task_name, **task_cfg)


    # .........................................................................

    @staticmethod
    def _setup_batch_cfg(batch_cfg_path: str, **update_batch_cfg) -> dict:
        """Sets up the BatchTaskManager configuration"""
        # Update defaults with configuration from file
        batch_cfg = _load_yml(batch_cfg_path)
        batch_cfg = recursive_update(_deepcopy(_BTM_DEFAULTS), batch_cfg)

        # Update again
        batch_cfg = recursive_update(batch_cfg, _deepcopy(update_batch_cfg))

        # For debug mode, adjust some parameters (relying on mutability)
        task_defaults = batch_cfg["task_defaults"]
        wm_opts = batch_cfg["worker_manager_options"]

        if batch_cfg["debug"]:
            log.note("  Using debug mode.")

            wm_opts["worker_manager"]["nonzero_exit_handling"] = "raise"
            # task_defaults["eval"]["plot_manager"]  # TODO

        return batch_cfg

    @staticmethod
    def _setup_dirs(out_dir: str, note: str = None) -> Dict[str, str]:
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
        batch_run_dir_name = fstr.format(
            timestamp=time.strftime(self.RUN_DIR_TIME_FSTR),
            note=note,
        )
        batch_run_dir = os.path.join(out_dir, batch_run_dir_name)

        # Create batch run directory
        dirs = dict(out=out_dir, batch_run=batch_run_dir)
        os.makedirs(batch_run_dir)

        # Subdirectories
        subdirs = ("config", "eval", "run",)
        for subdir in subdirs:
            dirs[subdir] = os.path.join(batch_run_dir, subdir)
            os.makedirs(dirs[subdir])

        return dirs

    def _perform_backup(self, **parts):
        """Stores the given configuration parts in the config directory"""
        log.info("Performing backups ...")
        cfg_dir = self._dirs["config"]

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


    def _add_eval_task(
            self, name: str, *,
            model_name: str, run_dir: str,
            out_dir: str = None,
            plot_only: list = None, update_plots_cfg: dict = None,
            **mv_kwargs,
        ):
        """Adds an evaluation task to the WorkerManager"""
        # Parse arguments
