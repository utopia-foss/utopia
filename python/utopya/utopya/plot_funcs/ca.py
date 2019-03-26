"""This module provides plotting functions to visualize cellular automata."""

from ._setup import *

import os
import logging
from typing import Union, Dict, Callable

import numpy as np

import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.animation
from matplotlib.colors import ListedColormap

from utopya import DataManager

# Get a logger
log = logging.getLogger(__name__)

# Increase log threshold for animation plotting
logging.getLogger('matplotlib.animation').setLevel(logging.WARNING)

# -----------------------------------------------------------------------------
# Helper function

class FileWriter():
    """The FileWriter class yields functionality to save individual frames.

    It adheres to the corresponding matplotlib animation interface.
    """
    def __init__(self, *,
                 file_format: str='png',
                 name_padding: int=6,
                 fstr: str="{dir:}/{num:0{pad:}d}.{ext:}",
                 **savefig_kwargs):
        """
        Initialize a FileWriter, which adheres to the matplotlib.animation
        interface and can be used to write individual files.

        Args:
            name_padding (int, optional): How wide the numbering should be
            file_format (str, optional): The file extension
            fstr (str, optional): The format string to generate the name
            **savefig_kwargs: kwargs to pass to figure.savefig
        """
        # Save arguments
        self.cntr = 0
        self.name_padding = name_padding
        self.fstr = fstr
        self.file_format = file_format
        self.savefig_kwargs = savefig_kwargs

        # Other attributes
        self.cm = None

    def saving(self, fig, base_outfile: str, **kwargs):
        """Create an instance of the context manager"""
        # Parse the given base file path to get a directory
        out_dir = os.path.splitext(base_outfile)[0]

        # Create and store the context manager
        self.cm = FileWriterContextManager(fig=fig, out_dir=out_dir, **kwargs)
        return self.cm

    def grab_frame(self):
        """Stores a single frame"""
        # Build the output path from the info of the context manager
        outfile = self.fstr.format(dir=self.cm.out_dir,
                                   num=self.cntr,
                                   pad=self.name_padding,
                                   ext=self.file_format)

        # Save the frame using the context manager, then increment the cntr
        self.cm.fig.savefig(outfile, format=self.file_format,
                            **self.cm.kwargs, **self.savefig_kwargs)
        self.cntr += 1

class FileWriterContextManager():
    """This class is needed by the file writer to provide the same interface
    as the matplotlib movie writers do.
    """

    def __init__(self, *, fig, out_dir: str, **kwargs):
        # Store arguments
        self.fig = fig
        self.out_dir = out_dir
        self.kwargs = kwargs

    def __enter__(self):
        """Called when entering context"""
        # Create the directory of the output file
        os.makedirs(self.out_dir)

    def __exit__(self, *args):
        """Called when exiting context"""
        # Need to close the figure
        plt.close(self.fig)

# -----------------------------------------------------------------------------

def state_anim(dm: DataManager, *, 
               out_path: str, 
               uni: UniverseGroup, 
               model_name: str,
               to_plot: dict,
               writer: str,
               frames_kwargs: dict=None, 
               base_figsize: tuple=None,
               fps: int=2, 
               dpi: int=96,
               preprocess_funcs: Dict[str, Callable]=None) -> None:
    """Create an animation of the states of a two dimensional cellular automaton.
    The function can use different writers, e.g. write out only the frames or create
    an animation with an external program (e.g. ffmpeg). 
    Multiple properties can be plotted next to each other, specified by the to_plot dict.
    
    Arguments:
        dm (DataManager): The DataManager object containing the data
        out_path (str): The output path
        uni (UniverseGroup): The selected universe data
        model_name (str): The name of the model instance, in which the data is
            located.
        to_plot (dict): The plotting configuration. The entries of this key
            refer to a path within the data and can include forward slashes to
            navigate to data of submodels.
        writer (str): The writer that should be used. Additional to the
            external writers such as 'ffmpeg', it is possible to create and
            save the individual frames with 'frames'
        frames_kwargs (dict, optional): The frames configuration that is used
            if the 'frames' writer is used.
        base_figsize (tuple, optional): The size of the figure to use for a
            single property
        fps (int, optional): The frames per second
        dpi (int, optional): The dpi setting
        preprocess_funcs (Dict[str, Callable], optional): For keys matching
            the keys in `to_plot`, the given function is called before the
            data is passed on to the plotting function.
    
    Raises:
        ValueError: For an invalid `writer` argument
    
    No Longer Raises:
        TypeError: For unknown dataset shape
    """

    def plot_property(*, data, ax, cmap, limits: list=None, title: str=None):
        """Helper function to plot a property on a given axis and return
        an imshow object
        """

        # Get colormap
        # Case continuous colormap
        if isinstance(cmap, str):
            norm = None
            bounds = None
            colormap = cmap

        # Case discrete colormap
        elif isinstance(cmap, dict):
            colormap = ListedColormap(cmap.values())
            bounds = limits
            norm = mpl.colors.BoundaryNorm(bounds, colormap.N)

        else:
            raise TypeError("Argument cmap needs to be either a string with "
                            "name of the colormap or a dict with values for a "
                            "discrete colormap. Was: {} with value: '{}'"
                            "".format(type(cmap), cmap))

        # Create imshow
        if limits:
            im = ax.imshow(data, cmap=colormap, animated=True,
                        origin='lower', vmin=limits[0], vmax=limits[1])
        else:
            im = ax.imshow(data, cmap=colormap, animated=True,
                        origin='lower')

        # Set title
        if title is not None:
            ax.set_title(title, fontsize=20)

        # Create colorbars
        fig = plt.gcf()
        cbar = fig.colorbar(im, ax=ax, norm=norm, ticks=bounds,
                            fraction=0.05, pad=0.02)

        if bounds is not None:
            # Adjust the ticks for the discrete colormap
            num_colors = len(cmap)
            tick_locs = (  (np.arange(num_colors) + 0.5)
                         * (num_colors-1)/num_colors)
            cbar.set_ticks(tick_locs)
            cbar.ax.set_yticklabels(cmap.keys())

        ax.axis('off')

        return im

    def prepare_data(*, data_2d: dict, prop_name: str, t: int) -> np.ndarray:
        """Prepares the data for plotting"""
        # Get the data from the dict of 2d data
        data = data_2d[prop_name][t]

        # If preprocessing is available for this property, call that function
        if preprocess_funcs and prop_name in preprocess_funcs:
            data = preprocess_funcs[prop_name](data)

        return data


    # Prepare the data ........................................................
    # Get the group that all datasets are in
    grp = uni['data'][model_name]

    data_2d = {p: grp[p] for p in to_plot.keys()}
    shapes = [d.shape for p, d in data_2d.items()]

    if any([shape != shapes[0] for shape in shapes]):
        raise ValueError("Shape mismatch; cannot plot.")

    # Can now be sure they all have the same shape, 
    # so its fine to take the first shape to extract the number of steps
    steps = shapes[0][0]
    
    # Prepare the writer ......................................................
    if mpl.animation.writers.is_available(writer):
        # Create animation writer if the writer is available
        wCls = mpl.animation.writers[writer]
        w = wCls(fps=fps,
                 metadata=dict(title="Grid Animation — {}"
                                     "".format(", ".join(to_plot.keys()),
                               artist="Utopia")))

    elif writer == 'frames':
        # Use the file writer to create individual frames
        w = FileWriter(name_padding=len(str(steps)),
                       **(frames_kwargs if frames_kwargs else {}))

    else:
        raise ValueError("The writer '{}' is not available on your system!"
                         "".format(writer))


    # Set up plotting .........................................................
    # Create figure, not squeezing to always have axs be something iterable
    fig, axs = plt.subplots(1, len(to_plot), squeeze=False,
                            figsize=base_figsize)

    # Adjust figure size in width to accommodate all properties
    figsize = fig.get_size_inches()
    fig.set_size_inches(figsize[0] * len(to_plot), figsize[1])

    # Store the imshow objects such that only the data has to be updated in a
    # following iteration step. Keys will be the property names
    ims = dict()
    
    log.info("Plotting %d frames of %d %s each ...",
             steps, len(to_plot),
             "property" if len(to_plot) == 1 else "properties")

    with w.saving(fig, out_path, dpi=dpi):
        # Loop through time steps, corresponding to rows in the 2d arrays
        for t in range(steps):
            log.debug("Plotting frame %d ...", t)

            # Loop through the subfigures
            for ax, (prop_name, props) in zip(axs.flat, to_plot.items()):
                # Get the data for this time step
                data = prepare_data(data_2d=data_2d, prop_name=prop_name, t=t)
                
                # In the first time step create a new imshow object
                if t == 0:
                    ims[prop_name] = plot_property(data=data, ax=ax, **props)

                    # Important to not continue with the rest
                    continue

                # For t>0, it is better to update imshow data without creating
                # a new object
                ims[prop_name].set_data(data)

                # If no limits are provided, autoscale the new limits
                # in the case of continuous colormaps.
                # A discrete colormap, that is provided as a dict, should never
                # autoscale. 
                if not isinstance(to_plot[prop_name]['cmap'], dict):
                    if not ('limits' in to_plot[prop_name]):
                        ims[prop_name].autoscale()
            
            # Updated all subfigures now and can now tell the writer that the
            # frame is finished and can be grabbed.
            w.grab_frame()

    log.info("Finished plotting of %d frames.", steps)
