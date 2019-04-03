"""This module provides plotting functions to visualize cellular automata.

NOTE This module is deprecated and will be removed!
"""

import os
import logging

import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.animation


# Get a logger
log = logging.getLogger(__name__)


# -----------------------------------------------------------------------------

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
