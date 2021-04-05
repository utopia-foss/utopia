import numpy as np
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

from utopya.plotting import is_plot_func, PlotHelper

# -----------------------------------------------------------------------------

@is_plot_func(use_dag=True,
              required_dag_tags=['opinion_u'])
def opinion_time_series(*, hlpr: PlotHelper,
                        data: dict,
                        bins: int=100,
                        rep_kwargs: dict=None,
                        densities_kwargs: dict=None,
                        hist_kwargs: dict=None):
    """
    Args:
        hlpr (PlotHelper): The Plot Helper
        data (dict): The data from DAG selection
        bins (int, optional): The number of bins for the density plot
        rep_kwargs (dict, optional): Kwargs for representative trajectories.
            Possible configurations:
                'enabled': Whether to show representative trajectories
                'max_reps': The maximum number of chosen representatives
                'rep_threshold': Lower threshold for the final group size above
                    which a second representative per group is allowed
        densities_kwargs (dict, optional): Passed to plt.imshow (density plot)
        hist_kwargs (dict, optional): Passed to plt.hist (final distribution)
    """
    opinion_u = data['opinion_u']
    final_opinions_u = opinion_u.isel(time=-1)

    num_vertices = opinion_u.coords['vertex_idx'].size
    time = opinion_u.coords['time'].data

    densities_kwargs = densities_kwargs if densities_kwargs else {}
    hist_kwargs = hist_kwargs if hist_kwargs else {}
    rep_kwargs = rep_kwargs if rep_kwargs else {}

    # Plot an opinion density heatmap over time ...............................

    densities = np.empty((bins, len(time)))

    for i in range(len(time)):
        densities[:,i] = np.histogram(opinion_u.isel(time=i),
                                      range=(0.,1.),
                                      bins=bins)[0]

    density_plot = hlpr.ax.imshow(densities,
                                  extent=[time[0], time[-1], 0, 1],
                                  **densities_kwargs)

    # Plot representative trajectories ........................................
    # The representative vertices are chosen based on the final opinion groups.
    # Reps are first picked for the largest opinion groups.

    if (rep_kwargs.get('enabled', False)):

        max_reps = rep_kwargs.get('max_reps', num_vertices)
        rep_threshold = rep_kwargs.get('rep_threshold', 1)

        reps = [] # store the vertex id's of the representatives here

        hist, bin_edges = np.histogram(final_opinions_u,
                                       range=(0.,1.),
                                       bins=bins)

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
                if (final_opinions_u.isel(vertex_idx=v) > b
                    and final_opinions_u.isel(vertex_idx=v) < b+1./bins):
                    reps.append(v)
                    break

        # If more reps available, add a second rep for larger opinion groups
        if (len(reps) < max_reps):
            for h, b, n in zip(hist_sorted,
                               bins_to_rep,
                               range(max_reps-len(reps))):
                if (h > rep_threshold):
                    for v in rand_vertex_idxs:
                        if (final_opinions_u.isel(vertex_idx=v) > b
                            and final_opinions_u.isel(vertex_idx=v) < b+1./b
                            and v not in reps):
                            reps.append(v)
                            break

        # Plot temporal opinion development for all representatives
        for r in reps:
            hlpr.ax.plot(time, opinion_u.isel(vertex_idx=r), lw=0.8)

    hlpr.ax.get_yaxis().set_minor_locator(ticker.MultipleLocator(0.1))

    # Plot histogram of the final user opinions  ..............................

    hlpr.select_axis(1,0)
    hist_u = hlpr.ax.hist(final_opinions_u, **hist_kwargs)

    # FIXME Depending on the histogram data the tick labels might be missing
    #       or overlapping. Find a way to automatically adapt the ticks and
    #       labels.
    # NOTE The code below allows setting fixed tick locations. Note that in
    #      this case the xscale must not be set to 'log' via the PlotHelper as
    #      that messes with the ticks.
    
    # hlpr.ax.set_xscale('log')
    # hlpr.ax.get_xaxis().set_major_locator(ticker.FixedLocator(
    #     locs=[10,100,1000]))
    # hlpr.ax.get_xaxis().set_minor_locator(ticker.FixedLocator(
    #     locs=[2,4,6,8,20,40,60,80,200,400,600,800]))
    # hlpr.ax.get_xaxis().set_minor_formatter(ticker.NullFormatter())

    plt.colorbar(density_plot)