"""This module provides matplotlib-related helper constructs"""

import copy
import logging
from typing import Union

import numpy as np

import matplotlib as mpl
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.legend_handler import HandlerPatch

# Get a logger
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

NORMS = {
    'Normalize': mpl.colors.Normalize,
    'BoundaryNorm': mpl.colors.BoundaryNorm,
    'NoNorm': mpl.colors.NoNorm,
    'LogNorm': mpl.colors.LogNorm,
    'PowerNorm': mpl.colors.PowerNorm,
    'SymLogNorm': mpl.colors.SymLogNorm,
    'TwoSlopeNorm': mpl.colors.TwoSlopeNorm
}

# -----------------------------------------------------------------------------

class HandlerEllipse(HandlerPatch):
    """Custom legend handler to turn an ellipse handle into a legend key."""
    def create_artists(self, legend, orig_handle, xdescent, ydescent, width,
                       height, fontsize, trans):
        center = 0.5 * width - 0.5 * xdescent, 0.5 * height - 0.5 * ydescent
        p = mpatches.Ellipse(xy=center, width=height + xdescent,
                            height=height + ydescent)
        self.update_prop(p, orig_handle, legend)
        p.set_transform(trans)
        return [p]

class ColorManager:
    """Custom color manager which provides an interface to the
    matplotlib.colors module.
    """
    def __init__(self, *, cmap: Union[str, dict]=None,
                          norm: Union[str, dict]=None,
                          labels: dict=None,
                          vmin: float=None,
                          vmax: float=None):
        """Initializes the ``ColorManager`` by building the colormap, the norm,
        and the colorbar labels.
        
        Args:
            cmap (Union[str, dict], optional): The colormap. If it is a string,
                it must name a registered colormap. If it is a dict, the
                following arguments are available:

                name (str):
                    Name of a registered colormap.
                from_values (dict, optional):
                    Dict of colors keyed by bin-specifier. If given, ``name``
                    is ignored and a discrete colormap is created from the list
                    of specified colors. The ``norm`` is then set to
                    ``BoundaryNorm``.

                    The bins can be specified either by bin-centers (Scalar) or
                    by bin-intervals (2-tuples). For the former, the deduced
                    bin-edges are assumed halfway between the bin-centers. For
                    the latter, the given intervals must be pairwise connected.
                    In both cases, the bins must monotonically increase.
                under (Union[str, dict], optional):
                    Passed on to
                    `Colormap.set_under <https://matplotlib.org/api/_as_gen/matplotlib.colors.Colormap.html#matplotlib.colors.Colormap.set_under>`_.
                over (Union[str, dict], optional):
                    Passed on to
                    `Colormap.set_over <https://matplotlib.org/api/_as_gen/matplotlib.colors.Colormap.html#matplotlib.colors.Colormap.set_over>`_.
                bad (Union[str, dict], optional):
                    Passed on to
                    `Colormap.set_bad <https://matplotlib.org/api/_as_gen/matplotlib.colors.Colormap.html#matplotlib.colors.Colormap.set_bad>`_.

            norm (Union[str, dict], optional): The norm that is applied for the
                color-mapping. If it is a string, the matching norm in
                `matplotlib.colors <https://matplotlib.org/api/colors_api.html>`_
                is created with default values. If it is a dict, the ``name``
                entry specifies the norm and all further entries are passed to
                its constructor. May be overwritten if a discrete colormap is
                specified in ``cmap``.
            labels (dict, optional): Colorbar tick-labels keyed by tick
                position.
            vmin (float, optional): The lower bound of the color-mapping.
                Ignored if norm is *BoundaryNorm*.
            vmax (float, optional): The upper bound of the color-mapping.
                Ignored if norm is *BoundaryNorm*.
        """
        if isinstance(cmap, str) or cmap is None:
            cmap = dict(name=cmap)

        if isinstance(norm, str) or norm is None:
            norm = dict(name=norm)

        cmap_kwargs = copy.deepcopy(cmap)
        norm_kwargs = copy.deepcopy(norm)

        if 'from_values' in cmap_kwargs:
            mapping = cmap_kwargs.pop('from_values')
            
            cmap_kwargs['name'] = 'ListedColormap'
            cmap_kwargs['colors'] = mapping.values()

            norm_kwargs['name'] = 'BoundaryNorm'
            norm_kwargs['ncolors'] = len(mapping)
            norm_kwargs['boundaries'] = self._parse_boundaries(
                                                list(mapping.keys()))

            log.remark("Configuring a discrete colormap 'from values'. "
                       "Overwriting 'norm' to BoundaryNorm with %d colors.",
                       norm_kwargs['ncolors'])

        if not norm_kwargs['name'] == 'BoundaryNorm':
            norm_kwargs['vmin'] = vmin
            norm_kwargs['vmax'] = vmax

            log.remark("norm.vmin and norm.vmax set to %s and %s.", vmin, vmax)

        self._cmap = self._create_cmap(**cmap_kwargs)
        self._norm = self._create_norm(**norm_kwargs)
        self._labels = labels

    @property
    def cmap(self):
        return self._cmap

    @property
    def norm(self):
        return self._norm
    
    @property
    def labels(self):
        return self._labels

    def _parse_boundaries(self, bins):
        """Parses the boundaries for the BoundaryNorm.
        
        Args:
            bins (Sequence): Either monotonically increasing sequence of bin
                centers or sequence of connected intervals (2-tuples).

        Returns:
            (list): Monotonically increasing boundaries.
        
        Raises:
            ValueError: On disconnected intervals or decreasing boundaries.
        """
        def from_intervals(intervals):
            """Extracts bin edges from sequence of connected intervals"""
            b = [intervals[0][0]]

            for low, up in intervals:
                if up < low:
                    raise ValueError("Received decreasing boundaries: "
                                     f"{up} < {low}.")

                elif b[-1] != low:
                    raise ValueError("Received disconnected intervals: Upper "
                                     f"bound {b[-1]} and lower bound {low} of "
                                     "the proximate interval do not match.")

                b.append(up)

            return b

        def from_centers(centers):
            """Calculates the bin edges as the halfway points between adjacent
            bin centers."""
            centers = np.array(centers)

            if len(centers) < 2:
                raise ValueError("At least 2 bin centers must be given to "
                                 f"create a BoundaryNorm. Got: {centers}")

            halves = 0.5*np.diff(centers)

            b = (  [centers[0]-halves[0]]
                 + [c+h for c,h in zip(centers, halves)]
                 + [centers[-1]+halves[-1]])

            return b

        if isinstance(bins[0], tuple):
            boundaries = from_intervals(bins)
        else:
            boundaries = from_centers(bins)

        return boundaries

    def _create_cmap(self, name: str=None, *, bad: Union[str, dict]=None,
                     under: Union[str, dict]=None, over: Union[str, dict]=None,
                     **cmap_kwargs):
        """Creates a colormap.
        
        Args:
            name (str, optional): The colormap name. Can either be the name of
                a registered colormap or ``ListedColormap``. ``None`` means
                :rc:`image.cmap`.
            bad (Union[str, dict], optional): Set color to be used for masked
                values.
            under (Union[str, dict], optional): Set the color for low
                out-of-range values when ``norm.clip = False``.
            over (Union[str, dict], optional): Set the color for high
                out-of-range values when ``norm.clip = False``.
            **cmap_kwargs: If ``name = ListedColormap``, passed on to the
                constructor of the colormap, else passed to
                matplotlib.cm.get_cmap.
        
        Returns:
            matplotlib.colors.Colormap: The created colormap.
        
        Raises:
            ValueError: On invalid colormap name.
        """
        if name == 'ListedColormap':
            cmap = mpl.colors.ListedColormap(**cmap_kwargs)

        else:
            try:
                cmap = copy.copy(mpl.cm.get_cmap(name, **cmap_kwargs))

            except ValueError as err:
                raise ValueError(f"Received invalid colormap name: '{name}'. "
                                 "Must name a registered colormap."
                                 ) from err

        if isinstance(bad, str):
            bad = dict(color=bad)

        if isinstance(under, str):
            under = dict(color=under)

        if isinstance(over, str):
            over = dict(color=over)

        if bad is not None:
            cmap.set_bad(**bad)
        
        if under is not None:
            cmap.set_under(**under)

        if over is not None:
            cmap.set_over(**over)

        return cmap

    def _create_norm(self, name: str=None, **norm_kwargs):
        """Creates a norm.
        
        Args:
            name (str, optional): The norm name. Must name a
                matplotlib.colors.Normalize instance (see 
                `matplotlib.colors <https://matplotlib.org/api/colors_api.html>`_).
                ``None`` means *Normalize*.
            **norm_kwargs: Passed on to the constructor of the norm.
        
        Returns:
            matplotlib.colors.Normalize: The created norm.
        
        Raises:
            ValueError: On invalid norm specification.
        """
        if name is None:
            name = 'Normalize'

        if name not in NORMS:
            available_norms = ', '.join(NORMS.keys())
            raise ValueError(f"Received invalid norm specifier: '{name}'. "
                             f"Must be one of: {available_norms}")

        return NORMS[name](**norm_kwargs)

    def map_to_color(self, X):
        """Maps the input data to color(s) by applying both norm and colormap.
        
        Args:
            X (Union[scalar, ndarray]): The data value(s) to convert to RGBA.
        
        Returns:
            Tuple of RGBA values if X is scalar, otherwise an array of RGBA
            values with a shape of ``X.shape + (4, )``.
        """
        return self.cmap(self.norm(X))
    
    def create_cbar(self, mappable, *, fig=None, ax=None, **cbar_kwargs):
        """Creates a colorbar.
        
        Args:
            mappable: The matplotlib.cm.ScalarMappable described by the
                colorbar.
            fig (None, optional): The Figure
            ax (None, optional): The axis
            **cbar_kwargs: Passed on to
                `fig.colorbar <https://matplotlib.org/api/_as_gen/matplotlib.pyplot.colorbar.html>`_.
        
        Returns:
            The created colorbar.
        """
        if fig is None:
            fig = plt.gcf()

        if ax is None:
            ax = fig.gca()

        cb = fig.colorbar(mappable, ax=ax, **cbar_kwargs)

        if self.labels is not None:
            cb.set_ticks(list(self.labels.keys()))
            cb.set_ticklabels(list(self.labels.values()))

        return cb