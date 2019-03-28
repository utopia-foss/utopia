"""CopyMe-model specific plot function for the state"""

import numpy as np

from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator

# -----------------------------------------------------------------------------

@is_plot_func(creator_type=UniversePlotCreator,
              # Provide some (static) default values for helpers
              helper_defaults=dict(
                set_labels=dict(x="Time"),
                set_limits=dict(x=[0, None]),
                )
              )
def state_mean(dm: DataManager, *, hlpr: PlotHelper, uni: UniverseGroup, 
               mean_of: str, **plot_kwargs):
    """Calculates the mean of the `mean_of` dataset and performs a lineplot
    over time.
    
    Args:
        dm (DataManager): The data manager from which to retrieve the data
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving
        uni (UniverseGroup): The selected universe data
        mean_of (str): The name of the CopyMe dataset that the mean is to be
            calculated of
        **plot_kwargs: Passed on to matplotlib.pyplot.plot
    """
    # Get the x-data, i.e. the times. Can query the UniverseGroup for that
    times = uni.get_times_array()

    # Extract the data that is to be plotted on the y-axis, i.e.: 'some_state'
    # averaged over all grid cells for every time step
    mean = [np.mean(d) for d in uni['data/CopyMe'][mean_of]]

    # Call the plot function on the currently selected axis in the plot helper
    hlpr.ax.plot(times, mean, **plot_kwargs)
    # NOTE `hlpr.ax` is just the current matplotlib.axes object. It has the
    #      same interface as `plt`, aka `matplotlib.pyplot`

    # Provide the plot helper with some information that is then used when
    # the helpers are invoked
    hlpr.provide_defaults('set_title', title="Mean '{}'".format(mean_of))
    hlpr.provide_defaults('set_labels', y="<{}>".format(mean_of))
    # NOTE Providing defaults recursively updates an existing configuration
    #      and marks the helper as 'enabled'
