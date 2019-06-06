"""Implements a plotting framework based on dantro

In order to make the plotting framework specific to Utopia, this module derives
both from the dantro PlotManager and some PlotCreator classes.
"""

import os
import logging
from typing import Callable

import dantro as dtr
import dantro.plot_creators
import dantro.plot_mngr

# Import some frequently used objects directly
from dantro.plot_creators import is_plot_func, PlotHelper

from ._path_setup import add_modules_to_path as _add_modules_to_path

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

    # The PlotHelper class to use
    PLOT_HELPER_CLS = PlotHelper

    def _resolve_plot_func(self, **kwargs) -> Callable:
        """Extends the parent method by ensuring that any registered external
        model plot modules are part of the sys.path
        """
        _add_modules_to_path('utopia_python')

        return super()._resolve_plot_func(**kwargs)


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
