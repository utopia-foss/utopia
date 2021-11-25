"""Spatially resolved plots, e.g. of agent position"""

import logging

import xarray as xr

from utopya.plotting import is_plot_func, PlotHelper

log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

def draw_agents(
    ds: xr.Dataset, *,
    hlpr: PlotHelper,
    collection = None,
    **plot_kwargs,
):
    """Expects an xr.Dataset and draws a single frame of agents in a domain"""
    if not collection:
        # Create collection
        collection = hlpr.ax.scatter(ds["x"], ds["y"], **plot_kwargs)

    else:
        # Update collection
        # TODO Make this more efficient
        da = ds.to_array()
        pos = da.sel(variable=["x", "y"]).transpose()
        collection.set_offsets(pos)

    return collection


# -----------------------------------------------------------------------------

@is_plot_func(
    use_dag=True, required_dag_tags=('data',), supports_animation=True
)
def agents_in_domain(
    *, data: dict,
    hlpr: PlotHelper,
    x: str = "x",
    y: str = "y",
    orientation: str = "orientation",
    **plot_kwargs,
):
    """Plots agent positions and orientations in the domain"""
    # Extract and check data
    ds = data["data"]

    if not isinstance(ds, xr.Dataset):
        raise TypeError(f"Expected xr.Dataset, got {type(ds)}!")

    # Apply encoding
    ds = ds.rename({
        x: "x",
        y: "y",
        orientation: "orientation",
    })

    # Extract space information and set axis and aspect ratio accordingly
    # TODO

    # Draw initial frame
    # draw_agents(ds.isel(time=0), hlpr=hlpr, **plot_kwargs)  # FIXME ?

    def update_position_and_orientation():
        collection = None
        for _t, _ds in ds.groupby("time"):
            collection = draw_agents(
                _ds,
                hlpr=hlpr,
                collection=collection,
                **plot_kwargs
            )
            # Done with this frame; yield control to the animation framework
            # which will grab the frame...
            yield

        log.info("Animation finished.")

    hlpr.register_animation_update(update_position_and_orientation)
