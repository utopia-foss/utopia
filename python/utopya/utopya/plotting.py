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

from utopya.info import _CMAKE_SOURCE_DIR as SRC_DIR

# Configure and get logger
log = logging.getLogger(__name__)

# Local constants

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

    # Define path to Utopia's python directory to add to sys.path
    UTOPIA_PYTHON_DIR = os.path.join(SRC_DIR, "python")


    def _resolve_plot_func(self, **kwargs) -> Callable:
        """Extends the parent method by ensuring that the UTOPIA_PYTHON_DIR
        is part of sys.path
        """
        if not self.UTOPIA_PYTHON_DIR in sys.path:
            sys.path.append(self.UTOPIA_PYTHON_DIR)

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
