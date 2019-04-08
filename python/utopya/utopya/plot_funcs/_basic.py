"""Common code for plot functions in the basic_uni and basic_mv modules"""

import xarray as xr

from ..plotting import PlotHelper
from ..dataprocessing import transform

# -----------------------------------------------------------------------------

def _errorbar(*, hlpr: PlotHelper, data: xr.DataArray, std: xr.DataArray,
              **errorbar_kwargs):
    """Given the data and (optionally) the standard deviation data, plots a
    single errorbar line.
    """
    # Check dimensionality
    if data.ndim != 1:
        raise ValueError("Requiring 1D data to plot a single errorbar "
                         "line but got {}D data with shape {}:\n{}\n"
                         "Apply dimensionality reducing transformations "
                         "using the `transform_data` argument to arrive "
                         "at plottable data."
                         "".format(data.ndim, data.shape, data))

    elif std is not None and std.ndim != 1:
        raise ValueError("Requiring 1D standard deviation data to plot the "
                         "error markers of a single errorbar line but "
                         "got {}D data with shape {}:\n{}\n"
                         "Apply dimensionality reducing transformations "
                         "using the `transform_std` argument to arrive "
                         "at plottable data."
                         "".format(std.ndim, std.shape, std))

    # Data is ok.
    # Plot the data against its coordinates, including standard deviation
    hlpr.ax.errorbar(data.coords[data.dims[0]], data,
                     yerr=std, **errorbar_kwargs)
