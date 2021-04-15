import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

from utopya.plotting import is_plot_func, PlotHelper

# -----------------------------------------------------------------------------

@is_plot_func(use_dag=True, required_dag_tags=['opinion'])
def opinion_time_series(
    *,
    hlpr: PlotHelper,
    data: dict,
    bins: int = 100,
    opinion_range = (0., 1.),
    representatives: dict = None,
    density_kwargs: dict = None,
    hist_kwargs: dict = None
):
    """Plots the temporal development of the opinion density and the final
    opinion distribution.

    Args:
        hlpr (PlotHelper): The Plot Helper
        data (dict): The data from DAG selection
        bins (int, optional): The number of bins for the histograms
        representatives (dict, optional): kwargs for representative
            trajectories. Possible configurations:
                'enabled': Whether to show representative trajectories
                    (default: false).
                'max_reps': The maximum total number of chosen representatives
                'rep_threshold': Lower threshold for the final group size above
                    which a second representative per group is allowed
        density_kwargs (dict, optional): Passed to plt.imshow (density plot)
        hist_kwargs (dict, optional): Passed to plt.hist (final distribution)
    """
    opinions = data['opinion']
    final_opinions = opinions.isel(time=-1)

    num_vertices = opinions.coords['vertex_idx'].size
    time = opinions.coords['time'].data

    density_kwargs = density_kwargs if density_kwargs else {}
    hist_kwargs = hist_kwargs if hist_kwargs else {}
    rep_kwargs = representatives if representatives else {}

    # Plot an opinion density heatmap over time ...............................

    densities = np.empty((bins, len(time)))

    for i in range(len(time)):
        densities[:,i] = np.histogram(
            opinions.isel(time=i), range=opinion_range, bins=bins
        )[0]

    density_plot = hlpr.ax.imshow(
        densities,
        extent=[time[0], time[-1], *opinion_range],
        **density_kwargs
    )

    hlpr.provide_defaults("set_limits", y=opinion_range)

    # Plot representative trajectories ........................................
    # The representative vertices are chosen based on the final opinion groups.
    # Reps are first picked for the largest opinion groups.

    if (rep_kwargs.get('enabled', False)):

        max_reps = rep_kwargs.get('max_reps', num_vertices)
        rep_threshold = rep_kwargs.get('rep_threshold', 1)

        reps = [] # store the vertex ids of the representatives here

        hist, bin_edges = np.histogram(
            final_opinions, range=opinion_range, bins=bins
        )

        # idxs that sort hist in descending order
        sort_idxs = np.argsort(hist)[::-1]
        # sorted hist values
        hist_sorted = hist[sort_idxs]
        # lower bin edges sorted from highest to lowest respective hist value
        # (restricted to non-empty bins)
        bins_to_rep = bin_edges[:-1][sort_idxs][hist_sorted > 0]

        # Generate a random sequence of vertex indices
        rand_vertex_idxs = np.arange(num_vertices)
        np.random.shuffle(rand_vertex_idxs)

        # Find representative vertices
        for b, n in zip(bins_to_rep, range(max_reps)):
            for v in rand_vertex_idxs:
                if (
                    final_opinions.isel(vertex_idx=v) > b
                    and final_opinions.isel(vertex_idx=v) < b+1./bins
                ):
                    reps.append(v)
                    break

        # If more reps available, add a second rep for larger opinion groups
        if (len(reps) < max_reps):
            for h, b, n in zip(
                hist_sorted, bins_to_rep, range(max_reps-len(reps))
            ):
                if (h > rep_threshold):
                    for v in rand_vertex_idxs:
                        if (
                            final_opinions.isel(vertex_idx=v) > b
                            and final_opinions.isel(vertex_idx=v) < b+1./b
                            and v not in reps
                        ):
                            reps.append(v)
                            break

        # Plot temporal opinion development for all representatives
        for r in reps:
            hlpr.ax.plot(time, opinions.isel(vertex_idx=r), lw=0.8)

    hlpr.ax.get_yaxis().set_minor_locator(ticker.MultipleLocator(0.1))

    # Plot histogram of the final opinions  ...................................

    hlpr.select_axis(1, 0)
    hist = hlpr.ax.hist(
        final_opinions, range=opinion_range, bins=bins, **hist_kwargs
    )
    hlpr.provide_defaults("set_limits", y=opinion_range)

    # Add colorbar for the density plot
    plt.colorbar(density_plot, ax=hlpr.ax)
