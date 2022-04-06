"""Plotting tools that can be used from all model-specific plot functions"""

from typing import Union

import matplotlib.pyplot as plt
import numpy as np
from matplotlib.collections import LineCollection
from matplotlib.colors import BoundaryNorm, Colormap, ListedColormap, Normalize

# -----------------------------------------------------------------------------


def save_and_close(out_path: str, *, save_kwargs: dict = None) -> None:
    """Save and close the figure, passing the kwargs

    Args:
        out_path (str): The output path to save the figure to
        save_kwargs (dict, optional): The additional save_kwargs
    """
    plt.savefig(out_path, **(save_kwargs if save_kwargs else {}))
    plt.close()


def colorline(
    x,
    y,
    *,
    ax=None,
    z=None,
    cmap: Union[str, Colormap] = "viridis",
    norm: Union[None, Normalize] = None,
    **lc_kwargs,
):
    """Plot a (gradient) colorline with the coordinates x and y.

    Args:
        x (array-like): The x data
        y (array-like): The y data
        ax: The axis to which to add the collection; if none is given, will
            use the current axis.
        z (array-like): Specifies the colors values; can also be a scalar.
        cmap (Union[str, Colormap], optional): Name of a matplotlib colormap
            or the actual colormap object.
        norm (Union[None, Normalize], optional): A colormap normalization.
            If None, will use plt.Normalize(0., 1.)
        **lc_kwargs: Keyword arguments passed to LineCollection.__init__

    Returns:
        matplotlib.LineCollection: The line collection

    Raises:
        ValueError: For invalid x, y or z
    """

    # -- Resolve defaults
    ax = ax if ax is not None else plt.gca()
    norm = norm if norm is not None else plt.Normalize(0.0, 1.0)
    cmap = plt.get_cmap(cmap) if isinstance(cmap, str) else cmap

    # Convert y and y into np.arrays
    x = np.array(x)
    y = np.array(y)

    if x.ndim != 1:
        raise ValueError(
            "Need `x` to have ndim == 1, but had shape {}!" "".format(x.shape)
        )

    elif y.ndim != 1:
        raise ValueError(
            "Need `y` to have ndim == 1, but had shape {}!" "".format(y.shape)
        )

    if z is None:
        z = np.linspace(0.0, 1.0, x.size)

    elif not hasattr(z, "__iter__"):
        # Presumably a single number; allow it being handled in the same way
        z = np.array([z])

    else:
        # Make sure it is an array and of correct size
        z = np.array(z)

        if z.ndim != 1:
            raise ValueError(
                "Need `z` to be of either None, an iterable, or "
                "an array of ndim == 1, but had shape {}!"
                "".format(z.shape)
            )

    # -- Done with argument checks now.
    # Calculate the segments
    points = np.array([x, y]).T.reshape(-1, 1, 2)
    segments = np.concatenate([points[:-1], points[1:]], axis=1)

    # Create the line collection
    lc = LineCollection(segments, array=z, cmap=cmap, norm=norm, **lc_kwargs)

    # Register it with the axis and trigger autoscaling
    ax.add_collection(lc)
    ax.autoscale()

    # Pass back the line collection if other things need to be done with it
    return lc
