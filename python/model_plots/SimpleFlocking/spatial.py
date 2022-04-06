"""Spatially resolved plots, e.g. of agent position"""

import logging

import numpy as np
import xarray as xr

from utopya.eval import PlotHelper, is_plot_func

log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------


def draw_agents(
    ds: xr.Dataset,
    *,
    hlpr: PlotHelper,
    collection=None,
    **plot_kwargs,
):
    """Expects an xr.Dataset and draws a single frame of agents in a domain"""
    if not collection:
        # Create collection
        collection = hlpr.ax.scatter(
            ds["x"],
            ds["y"],
            c=ds["orientation"],
            vmin=-np.pi,
            vmax=+np.pi,
            **plot_kwargs,
        )

    else:
        # Update the collection's offsets ...
        # TODO Make this more efficient?
        da = ds.to_array()
        pos = da.sel(variable=["x", "y"]).transpose()
        collection.set_offsets(pos)

        # ... and colors
        collection.set_array(da.sel(variable="orientation"))

    return collection


# -----------------------------------------------------------------------------


@is_plot_func(
    use_dag=True, required_dag_tags=("data",), supports_animation=True
)
def agents_in_domain(
    *,
    data: dict,
    hlpr: PlotHelper,
    space_extent: tuple,
    x: str = "x",
    y: str = "y",
    orientation: str = "orientation",
    title_fstr: str = "$t$ = {time:5d}",
    **plot_kwargs,
):
    """Plots agent positions and orientations in the domain"""
    # Extract and check data
    ds = data["data"]

    if not isinstance(ds, xr.Dataset):
        raise TypeError(f"Expected xr.Dataset, got {type(ds)}!")

    # Apply encoding
    ds = ds.rename(
        {
            x: "x",
            y: "y",
            orientation: "orientation",
        }
    )

    # Extract space information and set axis and aspect ratio accordingly
    hlpr.ax.set_xlim(0, space_extent[0])
    hlpr.ax.set_ylim(0, space_extent[1])
    hlpr.ax.set_aspect("equal")

    # Draw initial frame
    # draw_agents(ds.isel(time=0), hlpr=hlpr, **plot_kwargs)  # FIXME Needed?

    def update_position_and_orientation():
        collection = None
        for _t, _ds in ds.groupby("time"):
            hlpr.ax.set_title(
                title_fstr.format(time=_t.item()),
                fontfamily="monospace",
            )

            collection = draw_agents(
                _ds, hlpr=hlpr, collection=collection, **plot_kwargs
            )

            # Done with this frame; yield control to the animation framework
            # which will grab the frame...
            yield

        log.info("Animation finished.")

    hlpr.register_animation_update(update_position_and_orientation)
