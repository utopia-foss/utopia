"""Generic DAG-based plotting functions"""

import copy
import logging
from typing import Union, Sequence

import xarray as xr

from utopya.plotting import is_plot_func, PlotHelper


# Local variables
log = logging.getLogger(__name__)


# -----------------------------------------------------------------------------

@is_plot_func(
    use_dag=True, required_dag_tags=('counts',),
    supports_animation=True,
    helper_defaults=dict(
        set_legend=dict(use_legend=True, loc="best", fontsize="small"),
        set_labels=dict(y="Counts"),
    )
)
def histogram(*, data: dict, hlpr: PlotHelper,
              x: str, hue: str=None, frames: str=None,
              coarsen_by: int=None,
              align: str='edge',
              bin_widths: Union[str, Sequence[float]]=None,
              suptitle_kwargs: dict=None,
              show_histogram_info: bool=True,
              **bar_kwargs):
    """Shows a distribution as a stacked bar plot, allowing animation.

    Expects as DAG result ``counts`` an xr.DataArray of one, two, or three
    dimensions. Depending on the ``hue`` and ``frames`` arguments, this will
    be represented as a stacked barplot and as an animation, respectively.

    Args:
        data (dict): The DAG results
        hlpr (PlotHelper): The PlotHelper
        x (str): The name of the dimension that represents the position of the
            histogram bins. By default, these are the bin *centers*.
        hue (str, optional): Which dimension to represent by stacking bars of
            different hue on top of each other
        frames (str, optional): Which dimension to represent by animation
        coarsen_by (int, optional): By which factor to coarsen the dimension
            specified by ``x``. Uses xr.DataArray.coarsen and pads boundary
            values.
        align (str, optional): Where to align bins. By default, uses ``edge``
            for alignment, as this is more exact for histograms.
        bin_widths (Union[str, Sequence[float]], optional): If not given, will
            use the difference between the ``x`` coordinates as bin widths,
            padding on the right side using the last value
            If a string, assume that it is a DAG result and retrieve it from
            ``data``. Otherwise, use it directly for the ``width`` argument of
            ``plt.bar``, i.e. assume it's a scalar or a sequence of bin widths.
        suptitle_kwargs (dict, optional): Description
        show_histogram_info (bool, optional): Whether to draw a box with
            information about the histogram.
        **bar_kwargs: Passed on ``hlpr.ax.bar`` invocation

    Returns:
        None

    Raises:
        ValueError: Bad dimensionality or missing ``bin_widths`` DAG result
    """
    def stacked_bar_plot(ax, dists: xr.DataArray, bin_widths):
        """Given a 2D xr.DataArray, plots a stacked barplot"""
        bottom = None  # to keep track of the bottom edges for stacking

        # Create the iterator
        if hue:
            hues = [c.item() for c in dists.coords[hue]]
            original_sorting = lambda c: hues.index(c[0])
            dist_iter = sorted(dists.groupby(hue), key=original_sorting)
        else:
            dist_iter = [(None, dists)]

        # Create the plots for each hue value
        for label, dist in dist_iter:
            dist = dist.squeeze(drop=True)
            ax.bar(dist.coords[x], dist,
                   align=align, width=bin_widths,
                   bottom=bottom, label=label, **bar_kwargs)
            bottom = dist.data if bottom is None else bottom + dist.data

        # Annotate it
        if not show_histogram_info:
            return
        total_sum = dists.sum().item()
        hlpr.ax.text(1, 1,
                     (f"$N_{{bins}} = {dist.coords[x].size}$, "
                      fr"$\Sigma_{{{x}}} = {total_sum:.4g}$"),
                     transform=hlpr.ax.transAxes,
                     verticalalignment='bottom', horizontalalignment='right',
                     fontdict=dict(fontsize="smaller"),
                     bbox=dict(facecolor="white", linewidth=.5, pad=2))

    # Retrieve the data
    dists = data['counts']

    # Check expected dimensions
    expected_ndim = 1 + bool(hue) + bool(frames)
    if dists.ndim != expected_ndim:
        raise ValueError(f"With `hue: {hue}` and `frames: {frames}`, expected "
                         f"{expected_ndim}-dimensional data, but got:\n"
                         f"{dists}")

    # Calculate bin widths
    if bin_widths is None:
        bin_widths = dists.coords[x].diff(x)
        bin_widths = bin_widths.pad({x: (0, 1)}, mode='edge')

    elif isinstance(bin_widths, str):
        log.remark("Using DAG result '%s' for bin widths ...", bin_widths)
        try:
            bin_widths = data[bin_widths]
        except KeyError:
            raise ValueError(f"No DAG result '{bin_widths}' available for bin "
                             "widths. Make sure `compute_only` is set such "
                             "that the result will be computed.")

    # Allow dynamically plotting without animation
    if not frames:
        hlpr.disable_animation()
        stacked_bar_plot(hlpr.ax, dists, bin_widths)
        return
    # else: want an animation. Everything below here is only for that case.
    hlpr.enable_animation()

    # Determine the maximum, such that the scale is always the same
    max_counts = dists.sum(hue).max() if hue else dists.max()

    # Prepare some parameters for the update routine
    suptitle_kwargs = suptitle_kwargs if suptitle_kwargs else {}
    if 'title' not in suptitle_kwargs:
        suptitle_kwargs['title'] = "{dim:} = {value:d}"

    # Define an animation update function. All frames are plotted therein.
    # There is no need to plot the first frame _outside_ the update function,
    # because it would be discarded anyway.
    def update():
        """The animation update function: a python generator"""
        log.note("Commencing histogram animation for %d time steps ...",
                 len(dists.coords[frames]))

        for t, _dists in dists.groupby(frames):
            # Plot a frame onto an empty canvas
            hlpr.ax.clear()
            stacked_bar_plot(hlpr.ax, _dists, bin_widths)

            # Set the y-limits
            hlpr.invoke_helper('set_limits', y=[0, max_counts*1.05])

            # Apply the suptitle format string, then invoke the helper
            st_kwargs = copy.deepcopy(suptitle_kwargs)
            st_kwargs['title'] = st_kwargs['title'].format(dim='time', value=t)
            hlpr.invoke_helper('set_suptitle', **st_kwargs)

            # Done with this frame. Let the writer grab it.
            yield

    # Register the animation update with the helper
    hlpr.register_animation_update(update, invoke_helpers_before_grab=True)
