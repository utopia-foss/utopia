from utopya.plotting import is_plot_func, PlotHelper

# -----------------------------------------------------------------------------

@is_plot_func(use_dag=True,
              required_dag_tags=['x_data', 'y_data'])
def scatter(*, hlpr: PlotHelper, data: dict, **kwargs):
    """
    Args:
        hlpr (PlotHelper): Plot Helper
        data (dict): Data provided by DAG selection
        **kwargs: Passed to plt.scatter
    """

    hlpr.ax.scatter(data['x_data'], data['y_data'], **kwargs)