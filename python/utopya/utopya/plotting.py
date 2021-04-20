"""Implements a plotting framework based on dantro

In order to make the plotting framework specific to Utopia, this module derives
both from the dantro PlotManager and some PlotCreator classes.
"""

import os
import sys
import logging
import traceback
import importlib
from typing import Callable

import dantro as dtr
import dantro.plot_creators
import dantro.plot_mngr
from dantro.plot_creators import is_plot_func
from dantro.utils import register_operation as _register_operation

from .cfg import load_from_cfg_dir as _load_from_cfg_dir
from .model_registry import ModelInfoBundle
from ._path_setup import temporary_sys_path as _tmp_sys_path
from ._path_setup import temporary_sys_modules as _tmp_sys_modules


# Configure and get logger
log = logging.getLogger(__name__)

# Local constants

# -----------------------------------------------------------------------------

def register_operation(*, skip_existing=True, **kws) -> None:
    """Register an operation with the dantro data operations database.

    This invokes :py:func:`~dantro.utils.data_ops.register_operation`, but
    has  ``skip_existing == True`` as default in order to reduce number of
    arguments that need to be specified in Utopia model plots.

    Args:
        skip_existing (bool, optional): Whether to skip (without an error) if
            an operation
        **kws: Passed to :py:func:`~dantro.utils.data_ops.register_operation`
    """
    return _register_operation(**kws, skip_existing=skip_existing)


# -----------------------------------------------------------------------------
# PlotHelper specialisations

class PlotHelper(dtr.plot_creators.PlotHelper):
    """A specialization of the dantro ``PlotHelper`` used in plot creators that
    are derived from ``ExternalPlotCreator``.

    This can be used to add additional helpers for use in Utopia without
    requiring changes on dantro-side.

    .. note::

        The helpers implemented here should try to adhere to the interface
        exemplified by the dantro ``PlotHelper`` class, with the aim that they
        can then be migrated into dantro in the long run.
    """

    # .. Helper methods .......................................................
    # Can add helper methods here, prefixed with _hlpr_


# -----------------------------------------------------------------------------
# Plot creators

class ExternalPlotCreator(dtr.plot_creators.ExternalPlotCreator):
    """This is the Utopia-specific version of dantro's ``ExternalPlotCreator``.

    Its main purpose is to define common settings for plotting. By adding this
    extra layer, it allows for future extensibility as well.

    One of the common settings is that it sets as ``BASE_PKG`` the utopya
    :py:mod:`utopya.plot_funcs`, which is an extension of those functions
    supplied by dantro.
    """
    # Extensions
    EXTENSIONS = 'all'   # no checks performed
    DEFAULT_EXT = 'pdf'  # most common

    # Use utopya.plot_funcs as base package for relative module imports
    BASE_PKG = 'utopya.plot_funcs'

    # The PlotHelper class to use; here, the utopya-specific one
    PLOT_HELPER_CLS = PlotHelper

    # Additional parameters necessary for making model-specific plots available
    CUSTOM_PLOT_MODULE_NAME = "model_plots"
    CUSTOM_PLOT_MODULE_PATHS = _load_from_cfg_dir('plot_module_paths')


    def _get_module_via_import(self, module: str):
        """Extends the parent method by making the custom modules available if
        the regular import failed.
        """
        try:
            return super()._get_module_via_import(module)

        except ModuleNotFoundError as err:
            if (   not self.CUSTOM_PLOT_MODULE_NAME
                or not self.CUSTOM_PLOT_MODULE_PATHS
                or not module.startswith(self.CUSTOM_PLOT_MODULE_NAME)):
                # Should raise.
                # This also implicitly asserts that no python package with a
                # name equal to the prefix may be installed.
                raise

        log.debug("Module '%s' could not be imported with a default sys.path, "
                  "but is marked as a custom plot module. Attempting to "
                  "import it from %d additional path(s).",
                  module, len(self.CUSTOM_PLOT_MODULE_PATHS))

        # A dict to gather error information in
        errors = dict()

        # Go over the specified custom paths and try to import them
        for key, path in self.CUSTOM_PLOT_MODULE_PATHS.items():
            # In order to be able to import modules at the given path, the
            # sys.path needs to include the _parent_ directory of this path.
            parent_dir = os.path.join(*os.path.split(path)[:-1])

            # Enter two context managers, taking care to return both sys.path
            # and sys.modules back to the same state as they were before their
            # invocation.
            # The latter context manager is crucial because module imports lead
            # to a cache entry even if a subsequent attempt to import a part of
            # the module string was the cause of an error (which makes sense).
            # Example: a failing `model_plots.foo` import would still lead to a
            # cache entry of the `model_plots` module; however, attempting to
            # then import `model_plots.bar` will make the lookup _only_ in the
            # cached module. As we want several import attempts here, the cache
            # is not desired.
            with _tmp_sys_path(parent_dir), _tmp_sys_modules():
                try:
                    mod = super()._get_module_via_import(module)

                except ModuleNotFoundError as err:
                    # Gather some information on the error
                    tb = err.__traceback__
                    errors[parent_dir] = dict(err=err, tb=tb,
                                              tb_lines=traceback.format_tb(tb))

                else:
                    log.debug("Found module '%s' after having added custom "
                              "plot module path labelled '%s' (%s) to the "
                              "sys.path.", mod, key, path)
                    return mod

        # All imports failed. Inform extensively about errors to help debugging
        raise ModuleNotFoundError("Could not import module '{mod:}'! It was "
            "found neither among the installed packages nor among the custom "
            "plot module paths.\n\n"
            "The following errors were encountered at the respective custom "
            "plot module search paths:\n\n{info:}\n"
            "NOTE: This error can have two reasons:\n"
            "  (1) the '{mod:}' module does not exist in the specified search "
            "location.\n"
            "  (2) during import of the plot module you specified, an "
            "_unrelated_ ModuleNotFoundError occurred somewhere inside _your_ "
            "code.\n"
            "To debug, check the error messages and tracebacks above to find "
            "out which of the two is preventing module import."
            "".format(mod=module,
                      info="\n".join(["-- Error at custom plot module path "
                                      "{} : {}\n\n  Shortened traceback:\n{}"
                                      "".format(p, e['err'],
                                                "".join([e['tb_lines'][0],
                                                         "  ...\n",
                                                         e['tb_lines'][-1]]))
                                      for p, e in errors.items()])))


class UniversePlotCreator(dtr.plot_creators.UniversePlotCreator,
                          ExternalPlotCreator):
    """Makes plotting with data from a single universe more convenient"""
    PSGRP_PATH = 'multiverse'


class MultiversePlotCreator(dtr.plot_creators.MultiversePlotCreator,
                            ExternalPlotCreator):
    """Makes plotting with data from a all universes more convenient"""
    PSGRP_PATH = 'multiverse'


# -----------------------------------------------------------------------------

class PlotManager(dtr.plot_mngr.PlotManager):
    """This is the Utopia-specific version of the dantro PlotManager class

    It registers the Utopia-specific plot creators and allows for custom
    interface specifications.
    """

    # Register the supported plot creators
    CREATORS = dict(universe=UniversePlotCreator,
                    multiverse=MultiversePlotCreator)


    def __init__(self, *args, _model_info_bundle: ModelInfoBundle=None,
                 **kwargs):
        """Sets up a PlotManager.

        This additionally stores some Utopia-specific metadata about the
        model this PlotManager is used with. That information is then used to
        load some additional model-specific information once a creator is
        invoked.
        """
        super().__init__(*args, **kwargs)

        self._model_info_bundle = _model_info_bundle

    @property
    def common_out_dir(self) -> str:
        """The common output directory of all plots that were created with
        this plot manager instance. This uses the plot output paths stored in
        the plot information dict, specifically the ``target_dir`` entry.

        If there was no plot information yet, the return value will be empty.
        """
        return os.path.commonprefix([d['target_dir'] for d in self.plot_info])

    def _get_plot_creator(self, *args, **kwargs):
        """Before actually retrieving the plot creator, pre-loads the
        model-specific plot function module. This allows to register custom
        model-specific dantro data operations and have them available prior to
        the invocation of the creator.
        """
        def preload_module(*, mod_path, model_name):
            """Helper function to carry out preloading of the module"""
            # Compile the module name
            mod_str = "model_plots." + model_name

            # Determine the parent directory from which import is possible
            model_plots_parent_dir = os.path.dirname(os.path.dirname(mod_path))

            # Now, try to import
            try:
                # Use the _tmp_sys_modules environment to prevent that a failed
                # import ends up generating a cache entry. If the import is
                # successful, a cache entry will be added (below) and further
                # importlib.import_module call will use the cached module.
                with _tmp_sys_path(model_plots_parent_dir), _tmp_sys_modules():
                    mod = importlib.import_module(mod_str)

            except Exception as exc:
                if self.raise_exc:
                    raise RuntimeError("Failed pre-loading the model-"
                                       "specific plot module of the '{}' "
                                       "model! Make sure that {}/__init__.py "
                                       "can be loaded without errors; to "
                                       "debug, inspect the chained traceback "
                                       "above to find the cause of this error."
                                       "".format(model_name, mod_path)
                                       ) from exc
                log.debug("Pre-loading model-specific plot module from %s "
                          "failed: %s", mod_path, exc)
                return

            else:
                log.debug("Pre-loading was successful.")

            # Add the module to the cache
            if mod_str not in sys.modules:
                sys.modules[mod_str] = mod
                log.debug("Added '%s' module to sys.modules cache.", mod_str)

        # Invoke the preloading routine
        mib = self._model_info_bundle

        if mib is not None and mib.paths.get('python_model_plots_dir'):
            mod_path = mib.paths.get('python_model_plots_dir')
            if mod_path:
                preload_module(mod_path=mod_path, model_name=mib.model_name)

        # Now get to the actual retrieval of the plot creator
        return super()._get_plot_creator(*args, **kwargs)
