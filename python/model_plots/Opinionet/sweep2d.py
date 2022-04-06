import logging

import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
from mpl_toolkits.axes_grid1 import make_axes_locatable

from utopya.eval import PlotHelper, is_plot_func

log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------


@is_plot_func(use_dag=True, required_dag_tags=("data",))
def sweep2d(
    data, hlpr: PlotHelper, x: str, y: str, z: str, plot_kwargs: dict = {}
):

    """For multiverse runs, this produces a two dimensional plot showing
    specified values.

    Arguments:
        data (xarray): the dataset
        hlpr (PlotHelper): description
        x (str): the first parameter dimension of the diagram.
        y (str): the second parameter dimension of the diagram.
        z (str): the data dimension.
        plot_kwargs (dict, optional): kwargs passed to the pcolor plot function
    """

    df = pd.DataFrame(
        data["data"][z].data,
        index=data["data"][y].data,
        columns=data["data"][x].data,
    )
    im = hlpr.ax.pcolor(df, **plot_kwargs)

    hlpr.ax.set_yticks(
        [
            i
            for i in np.linspace(
                0.5, len(data["data"][y].data) - 0.5, len(data["data"][y].data)
            )
        ]
    )
    hlpr.ax.set_yticklabels([np.around(i, 3) for i in data["data"][y].data])

    hlpr.ax.set_xticks(
        [
            i
            for i in np.linspace(
                0.5, len(data["data"][x].data) - 0.5, len(data["data"][x].data)
            )
        ]
    )
    hlpr.ax.set_xticklabels([np.around(i, 3) for i in data["data"][x].data])

    divider = make_axes_locatable(hlpr.ax)
    cax = divider.append_axes("right", size="5%", pad=0.2)
    cbar = hlpr.fig.colorbar(im, cax=cax)
    cbar.set_label(z)

    hlpr.select_axis(0, 0)
