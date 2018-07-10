"""SimpleEG-model specific plot functions for spatial figures"""
from typing import Union

import numpy as np
import matplotlib.pyplot as plt
from matplotlib import animation as anim
from matplotlib import rcParams

from utopya import DataManager

# -----------------------------------------------------------------------------

class FileWriterContextManager():
    def __init__(self, *, fig, out_path, dpi):
        self.fig = fig
        self.out_path = out_path
        self.dpi = dpi

    def __enter__(self):
        pass

    def __exit__(self, *args):
        plt.close(self.fig)

class FileWriter():
    """The FileWriter class yields functionality to save individual frames."""
    def __init__(self, *, frame_name_padding: int=5, frame_format: str='png', **savefig_kwargs):
        self.index = 0
        self.frame_name_padding = frame_name_padding
        self.frame_format = frame_format

    def saving(self, fig, out_path, *,dpi):
        self.cm = FileWriterContextManager(fig=fig, out_path=out_path, dpi=dpi)
        return self.cm

    def grab_frame(self, **savefig_kwargs):
        out_path = self.cm.out_path[:-4] + str(self.index).rjust(self.frame_name_padding, '0') + '.' + self.frame_format

        self.cm.fig.savefig(out_path, format=self.frame_format, dpi=self.cm.dpi, **savefig_kwargs)
        self.index = self.index + 1

# -----------------------------------------------------------------------------

def grid_animation( dm: DataManager, 
                    *, 
                    out_path: str, 
                    uni: int, 
                    to_plot: dict,
                    writer: str,
                    frames: dict,
                    fps: int=1, 
                    step_size: int=1,
                    dpi: int=100):
    """Create a grid animation of a two dimensional cellular automaton
    
    Arguments:
        dm {DataManager} -- The DataManager object containing the data
        out_path {str} -- The output path
        uni {int} -- The universum
        to_plot {dict} -- The plotting configuration
        writer {str} -- The writer that should be used. Additional to the external writers such as 'ffmpeg', 
                        it is possible to create an dsave the individual frames with 'frames'
        frames {dict} -- The frames configuration that is used if writer='frames'
    
    Keyword Arguments:
        fps {int} -- The frames per second (default: {1})
        step_size {int} -- The step size (default: {1})
        dpi {int} -- The dpi setting (default: {100})
    """

    def plot_property(name, *, initial_data, ax, cmap, limits, title=None):
        """Plot a property on a given axis"""
        # Create imshow
        im = ax.imshow(initial_data, cmap=cmap, animated=True, origin='lower', vmin=limits[0], vmax=limits[1])

        if title is not None:
            ax.set_title(title)

        # Create colorbars
        fig = plt.gcf()
        fig.colorbar(im ,ax=ax, fraction=0.046, pad=0.04)
        ax.axis('off')

        return im

    # Get the group that all datasets are in
    grp = dm['uni'][str(uni)]['data/SimpleEG']

    # Get the shape of the 2D grid to later reshape the data
    cfg = dm['uni'][str(uni)]['cfg']
    grid_size = cfg['SimpleEG']['grid_size']
    steps = cfg['num_steps']
    new_shape = (steps+1, grid_size[0], grid_size[1])

    # Extract the data of the strategies in the CA    
    data_1d = {p: grp[p] for p in to_plot.keys()}
    data = {k: np.reshape(v, new_shape) for k,v in data_1d.items()}
        
    # create animation writer if the writer is available
    if anim.writers.is_available(writer):
        w = anim.writers[writer](fps=4, metadata={'title' : 'Grid Animation for {}'.format("_".join(to_plot.keys())), 
                                                        'artist' : 'Utopia'})
    # If the writer is set to 'frames' just create the frames and save them later
    elif writer == 'frames':
        w = FileWriter(frame_name_padding=frames['name_padding'], frame_format=frames['format'])
    # Case: no valid writer
    else:
        print("The writer {} is not available on your system!".format(writer))
        return

    # Set plot parameters
    rcParams.update({'font.size': 20})
    rcParams['figure.figsize'] = (6.0*len(to_plot), 5.0)

    # Create figure
    fig, axs = plt.subplots(1,len(to_plot))

    # Assert that the axes are stored in a list even if it is only one axis.
    if len(to_plot) == 1 :
         axs = [axs] 
    
    # store imshow objects such that only the data has to be updated in a following iteration step
    ims = []
    
    with w.saving(fig, out_path, dpi=dpi):
        
        # Loop through time steps
        for t in range(steps):

            # Loop through the subfigures
            for i, (ax, (key, props)) in enumerate(zip(axs, to_plot.items())):

                    # In the first time step create a new imshow object
                    if t == 0:
                        ims.append(plot_property(key, initial_data=data[key][t], ax=ax, **props))
                        continue

                    # In later steps just update imshow data without creating a new object
                    ims[i].set_data(data[key][t])                  
            
            w.grab_frame()