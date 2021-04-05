import numpy as np
import matplotlib.pyplot as plt
import matplotlib.cm as cm
import matplotlib.ticker as ticker
from scipy.signal import find_peaks

from dantro.tools import recursive_update
from utopya import DataManager, UniverseGroup
from utopya.plotting import is_plot_func, PlotHelper, MultiversePlotCreator

# -----------------------------------------------------------------------------

# -----------------------------------------------------------------------------
# FIXME Rewrite this such that the actual calculations are done via the DAG
#       (adding localization, number_of_peaks, etc. as custom operations)
# -----------------------------------------------------------------------------

@is_plot_func(creator_type=MultiversePlotCreator)
def sweep( dm: DataManager, *,
           hlpr: PlotHelper,
           mv_data,
           plot_prop: str,
           dim: str,
           dim2: str=None,
           stack: bool=False,
           no_errors: bool=False,
           data_name: str='opinion_u',
           bin_number: int=100,
           plot_kwargs: dict=None):
    """Plots a bifurcation diagram for one parameter dimension (dim)
        i.e. plots the chosen final distribution measure over the parameter,
        or - if second parameter dimension (dim2) is given - plots the
        2d parameter space as a heatmap (if not stacked).
    
    Configuration:
        - use the `select/field` key to associate one or multiple datasets
        - change `data_name` if needed
        - choose the dimension `dim` (and `dim2`) in which the sweep was
          performed.
    
    Arguments:
        dm (DataManager): The data manager from which to retrieve the data
        hlpr (PlotHelper): Description
        mv_data (xr.Dataset): The extracted multidimensional dataset
        plot_prop (str): The quantity that is extracted from the
            data. Available are: ['number_of_peaks', 'localization',
            'max_distance', 'polarization']
        dim (str): The parameter dimension of the diagram
        dim2 (str, optional): The second parameter dimension of the diagram
        stack (bool, optional): Whether the plots for dim2 are stacked
            or extend to heatmap
        data_name (str, optional): Description
        bin_number (int, optional): default: 100
            number of bins for the discretization of the final distribution
        plot_kwargs (dict, optional): passed to the plot function
    
    Raises:
        TypeError: for a parameter dimesion higher than 5
            (and higher than 4 if not sweeped over seed)
        ValueError: If 'data_name' data does not exist
    """

    # Drop coordinates that only have a single value
    # (i.e. when certain value selected in fields/select/..)
    # for coord in mv_data.coords:
    #     if len(mv_data[coord]) == 1:
    #         mv_data = mv_data[{coord: 0}]
    #         mv_data = mv_data.drop(coord)

    if not dim in mv_data.dims:
        raise ValueError("Dimension `dim` not available in multiverse data."
                         " Was: {} with value: '{}'."
                         " Available: {}"
                         "".format(type(dim), 
                                   dim,
                                   mv_data.coords))
    if len(mv_data.coords) > 5:
        raise TypeError("mv_data has more than two extra parameter dimensions."
                        " Are: {}. Chosen dim: {}. (Max: ['vertex', 'time', "
                        "'seed'] + 2)".format(mv_data.coords, dim))

    if (len(mv_data.coords) > 4) and ('seed' not in mv_data.coords):
        raise TypeError("mv_data has more than two extra parameter dimensions."
                        " Are: {}. Chosen dim: {}. (Max: ['vertex', 'time', "
                        "'seed'] + 2)".format(mv_data.coords, dim))

    plot_kwargs = (plot_kwargs if plot_kwargs else {})

    # Default plot configurations
    plot_kwargs_default_1d = {'fmt': 'o', 'ls': '-', 'lw': .4, 'mec': None,
                              'capsize': 1, 'mew': .6, 'ms': 2}
    
    if no_errors:
        plot_kwargs_default_1d = {'marker': 'o', 'ms': 2, 'lw': 0.4}

    plot_kwargs_default_2d = {'origin': 'lower', 'cmap': 'Spectral_r'}

    # analysis and plot functions ..............................................

    def plot_data_1d(param_plot, data_plot, std, plot_kwargs):
        if no_errors:
            hlpr.ax.plot(param_plot, data_plot, **plot_kwargs)
        else:
            hlpr.ax.errorbar(param_plot, data_plot, yerr=std, **plot_kwargs)

    def plot_data_2d(data_plot, param1, param2, plot_kwargs):

        heatmap = hlpr.ax.imshow(data_plot, **plot_kwargs) #'RdYlGn_r'

        if len(param1) <= 10:
            hlpr.ax.set_xticks(np.arange(len(param1)))
            xticklabels = np.array(["{:.2f}".format(p) for p in param1])
        else:
            hlpr.ax.set_xticks(np.arange(len(param1))
                            [::(int)(np.ceil(len(param1)/10))])
            xticklabels = np.array(["{:.2f}".format(p) 
                            for p in param1[::(int)(np.ceil(len(param1)/10))]])
        if len(param2) <= 10:
            hlpr.ax.set_yticks(np.arange(len(param2)))
            yticklabels = np.array(["{:.2f}".format(p) for p in param2])
        else:
            hlpr.ax.set_yticks(np.arange(len(param2))
                            [::(int)(np.ceil(len(param2)/10))])
            yticklabels = np.array(["{:.2f}".format(p) 
                            for p in param2[::(int)(np.ceil(len(param2)/10))]])
        
        hlpr.ax.set_xticklabels(xticklabels)
        hlpr.ax.set_yticklabels(yticklabels)
        plt.colorbar(heatmap)

    def get_number_of_peaks(raw_data):
        final_state = raw_data.isel(time=-1)
        final_state = final_state[~np.isnan(final_state)]

        # binning of the final opinion distribution with binsize=1/bin_number
        hist_data,bin_edges = np.histogram(final_state, range=(0.,1.),
                                            bins=bin_number)

        peak_number = len(find_peaks(hist_data, prominence=15, distance=5)[0])

        return peak_number

    def get_localization(raw_data):
        final_state = raw_data.isel(time=-1)
        final_state = final_state[~np.isnan(final_state)]

        hist, bins = np.histogram(final_state, range=(0.,1.), bins=bin_number,
                                    density=True)
        hist *= 1/bin_number

        l = 0
        norm = 0
        for i in range(len(hist)):
            norm += hist[i]**4
            l += hist[i]**2

        l = norm/l**2

        return l

    def get_max_distance(raw_data):
        final_state = raw_data.isel(time=-1)
        final_state = final_state[~np.isnan(final_state)]
        min = 1.
        max = 0.
        for val in final_state:
            if val > max:
                max = val
            elif val < min:
                min = val

        return max-min

    def get_polarization(raw_data):
        final_state = raw_data.isel(time=-1)
        final_state = final_state[~np.isnan(final_state)]

        p = 0
        for i in range(len(final_state)):
            for j in range(len(final_state)):
                p += (final_state[i] - final_state[j])**2

        return p

    def get_final_variance(raw_data):
        final_state = raw_data.isel(time=-1)
        final_state = final_state[~np.isnan(final_state)]

        var = np.var(final_state.values)

        return var

    def get_convergence_time(raw_data):
        max_tol = 0.05
        min_tol = 0.005
        ct = 0.
        for t in raw_data.coords['time']:
            nxt = False
            data = raw_data.sel(time=t)
            sorted_data = np.sort(data)
            for diff in np.diff(sorted_data):
                if diff > min_tol and diff < max_tol:
                    nxt = True
                    break
            if not nxt:
                ct = t
                break

        return ct

    def get_property(data, plot_prop: str=None):

        if plot_prop == 'number_of_peaks':
            return get_number_of_peaks(data)
        elif plot_prop == 'localization':
            return get_localization(data)
        elif plot_prop == 'max_distance':
            return get_max_distance(data)
        elif plot_prop == 'polarization':    
            return get_polarization(data)
        elif plot_prop == 'final_variance':    
            return get_final_variance(data)
        elif plot_prop == 'convergence_time':    
            return get_convergence_time(data)
        else:
            raise ValueError("'plot_prop' invalid! Was: {}".format(plot_prop))

    # data handling and plot setup .............................................
    legend = False
    heatmap = False

    if not data_name in mv_data.data_vars:
        raise ValueError("'{}' not available in multiverse data."
                         " Available in multiverse field: {}"
                         "".format(data_name, mv_data.data_vars))

    # this is the dataset containing the chosen data to plot
    # for all parameter combinations
    dataset = mv_data[data_name]

    # number of different parameter values i.e. number of points in the graph
    num_param = len(dataset[dim])

    # initialize arrays containing the data to plot:
    data_plot = np.zeros(num_param)
    param_plot = np.zeros(num_param)
    std = np.zeros_like(data_plot)

    # Get additional information for plotting
    leg_title = dim2
    if leg_title == "num_vertices":
        leg_title = "$N$"
    elif leg_title == "weighting":
        leg_title = "$\kappa$"
    elif leg_title == "rewiring":
        leg_title = "$r$"
    elif leg_title == "p_rewire":
        leg_title = "$p_{rewire}$"

    cmap_kwargs = plot_kwargs.pop("cmap_kwargs", None)
    if cmap_kwargs:
        cmin = cmap_kwargs.get("min", 0.)
        cmax = cmap_kwargs.get("max", 1.)
        cmap = cmap_kwargs.get("cmap")
        cmap = cm.get_cmap(cmap)

    markers = plot_kwargs.pop("markers", None)

    # If only one parameter sweep (dim) is done, the calculated quantity
    # is plotted against the parameter value.
    if (len(mv_data.coords) == 3):

        plot_kwargs = recursive_update(plot_kwargs_default_1d, plot_kwargs)
        param_index = 0
        for data in dataset:

            data_plot[param_index] = get_property(data, plot_prop)
            param_plot[param_index] = data[dim]
            param_index += 1

        if markers:
            plot_kwargs['marker'] = markers[0]

        plot_data_1d(param_plot, data_plot, std, plot_kwargs)

    # if two sweeps are done, check if the seed is sweeped
    elif (len(mv_data.coords) == 4):

        # average over the seed in this case
        if 'seed' in mv_data.coords:

            plot_kwargs = recursive_update(plot_kwargs_default_1d, plot_kwargs)

            for i in range(len(dataset[dim])):

                num_seeds = len(dataset['seed'])
                arr = np.zeros(num_seeds)

                for j in range(num_seeds):

                    data = dataset[{dim: i, 'seed': j}]
                    arr[j] = get_property(data, plot_prop)

                data_plot[i] = np.mean(arr)
                param_plot[i] = dataset[dim][i]
                std[i] = np.std(arr)

            if markers:
                plot_kwargs['marker'] = markers[0]

            plot_data_1d(param_plot, data_plot, std, plot_kwargs)
        
        # If 'stack', plot data of both dimensions against dim values (1d),
        # otherwise map data on 2d sweep parameter grid (color-coded).
        elif stack:

            legend = True
            plot_kwargs = recursive_update(plot_kwargs_default_1d, plot_kwargs)
            param_plot = dataset[dim]

            
            for i in range(len(dataset[dim2])):

                for j in range(num_param):

                    data = dataset[{dim2: i, dim: j}]
                    data_plot[j] = get_property(data, plot_prop)

                recursive_update(plot_kwargs, {'label': "{}"
                                    "".format(dataset[dim2][i].data)})
                if cmap_kwargs:
                    c = i * (cmax - cmin) / (len(dataset[dim2])-1.) + cmin
                    recursive_update(plot_kwargs, {'color': cmap(c)})

                if markers:
                    plot_kwargs['marker'] = markers[i%len(markers)]

                plot_data_1d(param_plot, data_plot, std, plot_kwargs)

        else:

            plot_kwargs = recursive_update(plot_kwargs_default_2d, plot_kwargs)
            heatmap = True
            num_param2 = len(dataset[dim2])
            data_plot = np.zeros((num_param2, num_param))
            param1 = np.zeros(num_param)
            param2 = np.zeros(num_param2)

            for i in range(num_param):
                param1[i] = dataset[dim][i]

                for j in range(num_param2):
                    param2[j] = dataset[dim2][j]
                    data = dataset[{dim: i, dim2: j}]
                    data_plot[j,i] = get_property(data, plot_prop)
            
            plot_data_2d(data_plot, param1, param2, plot_kwargs)

    elif (len(mv_data.coords) == 5):

        num_seeds = len(dataset['seed'])

        if stack:

            legend = True
            plot_kwargs = recursive_update(plot_kwargs_default_1d, plot_kwargs)
            param_plot = dataset[dim]
            param2 = dataset[dim2]
            
            for i in range(len(param2)):

                for j in range(num_param):

                    arr = np.zeros(num_seeds)

                    for k in range(num_seeds):
                        data = dataset[{dim2: i, dim: j, 'seed': k}]
                        arr[k] = get_property(data, plot_prop)

                    data_plot[j] = np.mean(arr)

                recursive_update(plot_kwargs, {'label': "{}"
                                    "".format(dataset[dim2][i].data)})
                if cmap_kwargs:
                    c = i * (cmax - cmin) / (len(dataset[dim2])-1.) + cmin
                    recursive_update(plot_kwargs, {'color': cmap(c)})

                if markers:
                    plot_kwargs['marker'] = markers[i%len(markers)]

                plot_data_1d(param_plot, data_plot, std, plot_kwargs)

        else:

            plot_kwargs = recursive_update(plot_kwargs_default_2d, plot_kwargs)
            heatmap = True
            num_param2 = len(dataset[dim2])
            data_plot = np.zeros((num_param2, num_param))
            param1 = np.zeros(num_param)
            param2 = np.zeros(num_param2)

            for i in range(num_param):
                param1[i] = dataset[dim][i]

                for j in range(num_param2):
                    param2[j] = dataset[dim2][j]
                    arr = np.zeros(num_seeds)

                    for k in range(num_seeds):
                        data = dataset[{dim: i, dim2: j, 'seed': k}]
                        arr[k] = get_property(data, plot_prop)

                    data_plot[j,i] = np.mean(arr)

            plot_data_2d(data_plot, param1, param2, plot_kwargs)

    # else: Error raised

    # Add labels and title
    if heatmap:
        hlpr.provide_defaults('set_labels', **{'x': dim, 'y': dim2})
        hlpr.provide_defaults('set_title', **{'title': plot_prop})

    else:
        hlpr.provide_defaults('set_labels',
             **{'x': ("$\epsilon$" if dim == "tolerance_u" else dim)})
        if plot_prop == "localization":
            hlpr.provide_defaults('set_labels', **{'y': "$L$"})
        elif plot_prop == "final_variance":
            hlpr.provide_defaults('set_labels', **{'y': "var($\sigma$)"})
        elif plot_prop == 'number_of_peaks':
            hlpr.provide_defaults('set_labels', **{'y': "$N_{peaks}$"})
        elif plot_prop == 'max_distance':
            hlpr.provide_defaults('set_labels', **{'y': "$d_{max}$"})
        elif plot_prop == 'convergence_time':    
            hlpr.provide_defaults('set_labels', **{'y': "$T_{conv}$"})
        else:    
            hlpr.provide_defaults('set_labels', **{'y': plot_prop})


        if legend:
            hlpr.ax.legend(title=leg_title)

        # Set minor ticks
        if plot_prop == "max_distance" or plot_prop == "localization":
            hlpr.ax.get_xaxis().set_major_locator(ticker.MultipleLocator(0.1))
            hlpr.ax.get_xaxis().set_minor_locator(ticker.MultipleLocator(0.05))
            hlpr.ax.get_yaxis().set_minor_locator(ticker.MultipleLocator(0.1))

