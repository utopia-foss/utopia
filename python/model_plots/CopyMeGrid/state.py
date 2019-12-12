"""CopyMeGrid-model specific plot function for the state"""

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
            takes care of plot aesthetics (labels, title, ...) and saving.
            To plot on the current axis, use ``hlpr.ax``.
        uni (UniverseGroup): The selected universe data
        mean_of (str): The name of the CopyMeGrid dataset that the mean is to be
            calculated of
        **plot_kwargs: Passed on to matplotlib.pyplot.plot
    """
    # Extract the data
    data = uni['data/CopyMeGrid'][mean_of]

    # Calculate the mean over the spatial dimensions
    mean = data.mean(['x', 'y'])

    # Call the plot function on the currently selected axis in the plot helper
    hlpr.ax.plot(mean.coords['time'], mean, **plot_kwargs)
    # NOTE `hlpr.ax` is just the current matplotlib.axes object. It has the
    #      same interface as `plt`, aka `matplotlib.pyplot`

    # Provide the plot helper with some information that is then used when
    # the helpers are invoked
    hlpr.provide_defaults('set_title', title="Mean '{}'".format(mean_of))
    hlpr.provide_defaults('set_labels', y="<{}>".format(mean_of))
    # NOTE Providing defaults recursively updates an existing configuration
    #      and marks the helper as 'enabled'


@is_plot_func(use_dag=True,
              required_dag_tags=('x_values', 'scatter',
                                 'amplitude', 'offset', 'frequency'))
def some_DAG_based_CopyMeGrid_plot(*, data: dict, hlpr: PlotHelper,
                                   scatter_kwargs: dict=None):
    """This is an example plot to show how to implement a generic DAG-based
    plot function.
    
    In this example, nothing spectacular happens: A scatter plot is made and
    a sine curve with some amplitude and frequency is plotted, both of which
    arguments are provided via the DAG.
    
    Ideally, plot functions should be as generic as possible; just a bridge
    between some data (produced by the DAG) and its visualization.
    This plot is a (completely made-up) example for cases where it makes sense
    to have a rather specific plot...
    
    .. note::
    
        To specify the plot in a generic way, omit the ``creator_type``
        argument in the decorator. If you want to specialize the plot function,
        you can of course specify the creator type, but most often that's not
        necessary.
    
        Check the decorator's signature and the utopya and dantro documentation
        for more information on the DAG framework
    
    Args:
        data (dict): The selected DAG data, contains the required DAG tags.
        hlpr (PlotHelper): The PlotHelper that instantiates the figure and
            takes care of plot aesthetics (labels, title, ...) and saving.
            To plot on the current axis, use ``hlpr.ax``.
        scatter_kwargs (dict, optional): Passed to matplotlib.pyplot.scatter
    """
    x_vals = data['x_values']
    hlpr.ax.scatter(x_vals, data['scatter'],
                    **(scatter_kwargs if scatter_kwargs else {}))

    # Do the sine plot
    sine = (  data['amplitude'] * np.sin(x_vals * data['frequency'])
            + data['offset'])
    hlpr.ax.plot(x_vals, sine, label="my sine")

    # Enable the legend
    hlpr.mark_enabled("set_legend")
