"""Implements a plotting framework based on dantro

In order to make the plotting framework specific to Utopia, this module derives
both from the dantro PlotManager and some PlotCreator classes.
"""

import os
import sys
import logging
from typing import Callable

import dantro as dtr
import dantro.plot_creators
import dantro.plot_mngr
from dantro.plot_creators import is_plot_func

from .cfg import load_from_cfg_dir as _load_from_cfg_dir
from ._path_setup import temporary_sys_path as _tmp_sys_path
from ._path_setup import temporary_sys_modules as _tmp_sys_modules


# Configure and get logger
log = logging.getLogger(__name__)

# Local constants

# -----------------------------------------------------------------------------
# PlotHelper specialisations

class PlotHelper(dtr.plot_creators.PlotHelper):
    """A specialization of the dantro PlotHelper for ExternalPlotCreator-
    derived plot creators.

    This can be used to add additional helpers for use in utopya without
    requiring changes on dantro-side.

    NOTE The helpers implemented here should try to adhere to the interface
         exemplified by the dantro PlotHelper class, with the aim that they can
         then be migrated into dantro in the long run.
    """

    # .. Helper methods .......................................................
    # Can add helper methods here, prefixed with _hlpr_


# -----------------------------------------------------------------------------
# Plot creators

class ExternalPlotCreator(dtr.plot_creators.ExternalPlotCreator):
    """This is the Utopia-specific version of the dantro ExternalPlotCreator

    Its main purpose is to define common settings for plotting. By adding this
    extra layer, it allows for future extensibility as well.

    One of the common settings is that it sets as BASE_PKG the utopya sub-
    package `plot_funcs`, which is an extension of those functions supplied by
    dantro.
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
                
                except ModuleNotFoundError:
                    pass

                else:
                    log.debug("Found module '%s' after having added custom "
                              "plot module path labelled '%s' (%s) to the "
                              "sys.path.", mod, key, path)
                    return mod

        raise ModuleNotFoundError("Could not import module '{}'! It was found "
                                  "neither among the installed packages nor "
                                  "among the following custom plot module "
                                  "paths: {}"
                                  "".format(module,
                                            self.CUSTOM_PLOT_MODULE_PATHS))


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
