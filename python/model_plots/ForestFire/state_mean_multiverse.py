import numpy as np
import matplotlib.pyplot as plt
import xarray as xr

from utopya import DataManager, UniverseGroup, MultiverseGroup
from utopya.plotting import is_plot_func, PlotHelper, UniversePlotCreator

@is_plot_func(creator_type=UniversePlotCreator,
              # Provide some (static) default values for helpers
              helper_defaults=dict(
                set_labels=dict(x="Time", y="Density"),
                set_scale=dict(x='linear', y='log'),
                set_title=dict(title='Mean tree density'),
                set_legend=dict(use_legend=True),
                save_figure=dict(bbox_inches="tight")
                )
              )
def state_mean_multiverse(  dm: DataManager, *,
                            mv_data: xr.Dataset,
                            hlpr: PlotHelper,
                            save_kwargs: dict=None,
                            plot_kwargs: dict=None):
    '''Plots the cluster distribution for multiple universes'''

    data = mv_data['mean_density']

    # Calculate the mean over all dimensions except for the time
    data.mean(dim=[d for d in data.dims if (d != 'time' or d!= 'p_lightning')])

    for p_light in data['p_lightning']:
        data_sel = data.sel(p_lightning=p_light)

        hlpr.ax.plot(data_sel['time'], data_sel, 
                    label=r'${}$'.format(p_light.values), 
                    **plot_kwargs)

        hlpr.provide_defaults('set_legend', title='{}'.format(p_light.name))
