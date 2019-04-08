"""Common code for plot functions in the basic_uni and basic_mv modules"""

import copy

import xarray as xr

from ..plotting import PlotHelper
from ..dataprocessing import transform

# -----------------------------------------------------------------------------

def _errorbar(*, hlpr: PlotHelper, data: xr.DataArray, std: xr.DataArray,
              fill_between: bool=True, fill_between_kwargs: dict=None,
              **errorbar_kwargs):
    """Given the data and (optionally) the standard deviation data, plots a
    single errorbar line.
    
    Args:
        hlpr (PlotHelper): The helper
        data (xr.DataArray): The data
        std (xr.DataArray): The y-error data
        fill_between (bool, optional): Whether to use plt.fill_between or
            plt.errorbar to plot y-errors
        fill_between_kwargs (dict, optional): Passed on to plt.fill_between
        **errorbar_kwargs: Passed on to plt.errorbar
    
    Raises:
        ValueError: On non-1D data
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
    # Decide on whether yerr is done by errorbar or by fill_between
    yerr = std if not fill_between else None

    # Plot the data against its coordinates, including standard deviation
    ebar = hlpr.ax.errorbar(data.coords[data.dims[0]], data,
                            yerr=yerr, **errorbar_kwargs)

    # Now plot the confidence interval via 
    if fill_between:
        # Find out the colour of the error bar line. Get line collection
        lc, _, _ = ebar
        line_color = lc.get_c()
        line_alpha = lc.get_alpha() if lc.get_alpha() else 1.
        line_label = errorbar_kwargs.get('label', None)

        # Prepare kwargs
        fb_kwargs = (copy.deepcopy(fill_between_kwargs) if fill_between_kwargs
                     else {})

        if 'color' not in fb_kwargs:
            fb_kwargs['color'] = line_color
        if 'alpha' not in fb_kwargs:
            fb_kwargs['alpha'] = line_alpha * .2
        if 'label' not in fb_kwargs and line_label:
            fb_kwargs['label'] = line_label + " (std. dev.)"

        # Fill.
        hlpr.ax.fill_between(data.coords[data.dims[0]],
                             y1=(data - std), y2=(data + std),
                             **fb_kwargs)

    # TODO Manually add the legend patch
