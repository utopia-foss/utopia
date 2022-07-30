import logging

import matplotlib.pyplot as plt
import numpy as np

from utopya.eval import DataManager, PlotHelper, UniverseGroup, is_plot_func

log = logging.getLogger(__name__)
logging.getLogger("matplotlib.animation").setLevel(logging.WARNING)

# -------------------------------------------------------------------------------
@is_plot_func(
    creator="universe",
    supports_animation=True,
    helper_defaults=dict(set_labels=dict(x="Values", y="Counts")),
)
def opinion_animated(
    dm: DataManager,
    *,
    uni: UniverseGroup,
    hlpr: PlotHelper,
    num_bins: int = 100,
    time_idx: int,
    **plot_kwargs,
):

    """Plots an animated histogram of the opinion distribution over time.

    Arguments:
        num_bins(int): Binning of the histogram
        time_idx (int, optional): Only plot one single frame (eg. last frame)
        plot_kwargs (dict, optional): Passed to plt.bar
    """

    # datasets...................................................................
    opinions = uni["data/Opinionet/nw/opinion"]
    time = opinions["time"].data
    time_steps = time.size
    cfg_op_space = uni["cfg"]["Opinionet"]["opinion_space"]
    if (cfg_op_space["type"]) == "continuous":
        val_range = cfg_op_space["interval"]
    elif (cfg_op_space["type"]) == "discrete":
        val_range = tuple((0, cfg_op_space["num_opinions"]))

    # get histograms.............................................................
    def get_hist_data(input_data):
        counts, bin_edges = np.histogram(
            input_data, range=val_range, bins=num_bins
        )
        bin_pos = bin_edges[:-1] + (np.diff(bin_edges) / 2.0)

        return counts, bin_edges, bin_pos

    t = time_idx if time_idx else range(time_steps)

    # calculate histograms, set axis ranges
    counts, bin_edges, pos = get_hist_data(opinions[t, :])
    hlpr.ax.set_xlim(val_range)
    bars = hlpr.ax.bar(pos, counts, width=np.diff(bin_edges), **plot_kwargs)
    text = hlpr.ax.text(
        0.02, 0.93, f"step {time[t]}", transform=hlpr.ax.transAxes
    )

    # animate....................................................................
    def update_data(stepsize: int = 1):
        """Updates the data of the imshow objects"""
        if time_idx:
            log.info(
                f"Plotting distribution at time step {time[time_idx]} ..."
            )
        else:
            log.info(
                f"Plotting animation with {opinions.shape[0] // stepsize} "
                "frames ..."
            )
        next_frame_idx = 0
        if time_steps < stepsize:
            log.warn(
                "Stepsize is greater than number of steps. Continue by "
                "plotting first and last frame."
            )
            stepsize = time_steps - 1
        for t in range(time_steps):
            if t < next_frame_idx:
                continue
            if time_idx:
                t = time_idx
            data = opinions[t, :]
            counts_at_t, _, _ = get_hist_data(data)
            for idx, rect in enumerate(bars):
                rect.set_height(counts_at_t[idx])
                text.set_text(f"step {time[t]}")
                hlpr.ax.relim()
                hlpr.ax.autoscale_view(scalex=False)
            if time_idx:
                yield
                break
            next_frame_idx = t + stepsize
            yield

    hlpr.register_animation_update(update_data)
