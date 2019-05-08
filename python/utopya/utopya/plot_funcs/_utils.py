"""Utility functions that assist plot functions"""

import itertools
import logging
from typing import Tuple, Dict

import numpy as np
import xarray as xr

# Local Variables
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

def calc_pxmap_rectangles(*, x_coords: np.ndarray, y_coords: np.ndarray,
                          x_scale: str='lin', y_scale: str='lin',
                          default_pos: Tuple[float, float]=(0., 0.),
                          default_distance: float=1.0,
                          size_factor: float=1.0,
                          extend: bool=False
                          ) -> Tuple[xr.Dataset, Dict[str, tuple]]:
    """Calculates the positions and sizes of rectangles centered at the given
    coordinates in such a way that they fully cover the 2D space spanned by
    the coordinates.
    
    The exact values of the coordinates are arbitrary, but they should be
    ordered. The resulting rectangles will not necessarily be centered around
    the coordinate, but the distance between rectangle edges is set such that
    it lies halfway to the next coordinate in that dimension; the "halfway" is
    evaluated according to a certain scale.
    
    Args:
        x_coords (np.ndarray): The x coordinates
        y_coords (np.ndarray): The y coordinates
        x_scale (str, optional): The x-axis scale, used to determine rectangle
            sizes.
        y_scale (str, optional): The y-axis scale, used to determine rectangle
            sizes.
        default_pos (Tuple[float, float], optional): If any of the coordinates
            is not given, this information will be used for creating the
            rectangle specification.
        default_distance (float, optional): If any of the coordinates is not
            given or of size 1, this distance to the next data point is assumed
            in that dimension. In such a case, the scale does not have an
            effect on the resulting rectangle.
        size_factor (float, optional): Scaling factor for the rectangle sizes
        extend (bool, optional): Whether to extend the rectangles of the points
            at the border of the domain such that the coordinate is _not_ at
            the edge of the rectangle but in the bulk.
    
    Returns:
        Tuple[xr.Dataset, Dict[str, tuple]]: The first tuple element is the
            xr.Dataset of rectangle specifications, each available as a data
            variable.
                - ``pos_x``, ``pos_y``: position of the lower-value coordinate
                  of the rectangle. Together, this specifies the bottom left-
                  hand corner of the rectangle (in a right-hand coordinate
                  system)
                - ``len_x``, ``len_y``: lengths of the rectangle sides in the
                  specified dimensions. Adding both these to the position leads
                  to the top-right hand corner of the rectangle
                - ``rect_spec``: A matplotlib-compatible rectangle
                  specification, i.e. (position, width, height)
            The second tuple element are the limits in x and y direction, given
            as dict with keys x and y.
    """

    # Define some allowed linear and logarithmic scales
    LIN_SCALES = ['lin', 'linear']
    LOG_SCALES = ['log', 'symlog']
    CATEGORIAL_SCALES = ['cat', 'categorical', 'cat_at_value']

    # Helper functions ........................................................

    def determine_diffs_and_scale(coords: np.ndarray, *, scale: str,
                                  axis_name: str) -> Tuple[np.ndarray, str]:
        """Given the coordinates and the scale, determine the distances
        between coordinates in one dimension. This is also used to check the
        scale.
        
        Args:
            coords (array-like): Sequence of coordinates
            scale (str): The scale to use
            axis_name (str): The name of the axis; used for error message only
        
        Returns:
            Tuple[np.ndarray, str]: The coordinate distances and the new scale
        
        Raises:
            ValueError: On invalid scale argument
        """
        if scale in LIN_SCALES:
            diffs = np.diff(coords)
        
        elif scale in LOG_SCALES:
            diffs = np.diff(np.log(coords))
        
        elif scale in CATEGORIAL_SCALES:
            # For categorial scale at value, use just some arbitrary integeres
            # for the coordinates; the scale can then be assumed linear.
            if scale != 'cat_at_value':
                coords = list(range(len(coords)))
            diffs = np.diff(coords)
            scale = 'lin'
        
        else:
            raise ValueError("Invalid scale argument for {} axis: '{}'! "
                             "Expected one of: {}"
                             "".format(scale, axis_name,
                                       ", ".join(  LIN_SCALES + LOG_SCALES
                                                 + CATEGORIAL_SCALES)))

        return diffs, scale

    def expand_diffs(diffs, *, extend: bool, default: float) -> np.ndarray:
        """Adds the boundary values to the given diffs"""
        if diffs.size < 1:
            # Need to use the default value
            diffs = np.array([default, default])
        
        elif extend:
            # Add the first item to the beginning and the last to the end of
            # the diffs sequences to account for the borders
            diffs = np.insert(diffs, 0, diffs[0])
            diffs = np.append(diffs, diffs[-1])

        else:
            # No extension of borders desired --> add zeros to diffs
            diffs = np.insert(diffs, 0, 0.)
            diffs = np.append(diffs, 0.)

        return diffs

    def determine_limits(*, coords, sizes, scale: str) -> Tuple[float, float]:
        """Determine the limits of the space that is covered by the rectangles
        at the given coordinates and sizes.
        """
        if scale in LIN_SCALES:
            return (float(coords[0])  - sizes[0],
                    float(coords[-1]) + sizes[-1])
        
        elif scale in LOG_SCALES:
            return (np.exp(np.log(float(coords[0]))  - sizes[0]),
                    np.exp(np.log(float(coords[-1])) + sizes[-1]))

        # NOTE This point is not reached; scale is checked elsewhere


    # The functions below create a rectangle specifier given the x and y
    # positions and the desired sizes.
    # Have 4 explicit functions (one for each scale specification) because it's
    # very tedious to create the rectangle specifier in a general way ... Grml.
    
    def calc_linlin_rect(x: float, y: float,
                         x_sizes: tuple, y_sizes: tuple) -> tuple:
        """
        Create the rectangle specification for a single rectangle embedded in
        a lin-lin plot.

        Args:
            x (float): The x-coordinate
            y (float): The y-coordinate
            x_sizes (tuple): The distance to the rectangle edges in x direction
            y_sizes (tuple): The distance to the rectangle edges in y direction
        
        Returns:
            tuple: (x pos. bottom left-hand corner,
                    y pos. bottom left-hand corner,
                    total x length,
                    total y length)
        """
        return (
            x            - x_sizes[0],
            y            - y_sizes[0],
            x_sizes[0]   + x_sizes[1],
            y_sizes[0]   + y_sizes[1]
        )

    def calc_linlog_rect(x, y, x_sizes, y_sizes) -> tuple:
        """See calc_linlin_rect"""
        return ( 
            x            - x_sizes[0],
            np.exp(np.log(y) - y_sizes[0]),
            x_sizes[0]   + x_sizes[1],
            np.exp(np.log(y) + y_sizes[1]) - np.exp(np.log(y) - y_sizes[0])
        )

    def calc_loglin_rect(x, y, x_sizes, y_sizes) -> tuple:
        """See calc_linlin_rect"""
        return (
            np.exp(np.log(x) - x_sizes[0]),
            y            - y_sizes[0],
            np.exp(np.log(x) + x_sizes[1]) - np.exp(np.log(x) - x_sizes[0]),
            y_sizes[0]   + y_sizes[1]
        )

    def calc_loglog_rect(x, y, x_sizes, y_sizes) -> tuple:
        """See calc_linlin_rect"""
        return (
            np.exp(np.log(x) - x_sizes[0]),
            np.exp(np.log(y) - y_sizes[0]),
            np.exp(np.log(x) + x_sizes[1]) - np.exp(np.log(x) - x_sizes[0]),
            np.exp(np.log(y) + y_sizes[1]) - np.exp(np.log(y) - y_sizes[0])
        )

    # Create a map to select the callable via `in LIN_SCALES` boolean key pair
    FUNC_MAP = {
        #x lin?  y lin?
        (True,   True)  : calc_linlin_rect,
        (True,   False) : calc_linlog_rect,
        (False,  True)  : calc_loglin_rect,
        (False,  False) : calc_loglog_rect
    }

    # .........................................................................

    log.debug("Calculating rectangle positions and sizes ...")

    # Allow None, in which case the rectangle is set to a default position on
    # that axis
    x_coords = x_coords if x_coords is not None else [default_pos[0]]
    y_coords = y_coords if y_coords is not None else [default_pos[1]]

    # Check the lengths
    if len(x_coords) < 1 or len(y_coords) < 1:
        raise ValueError("Coordinate sequences need to be at least of length "
                         "one or None (to use defaults), but were: {} and {}"
                         "".format(x_coords, y_coords))

    # Calculate distances, distinguishing between linear and logarithmic scale
    x_diffs, x_scale = determine_diffs_and_scale(x_coords, scale=x_scale,
                                                 axis_name="x")
    y_diffs, y_scale = determine_diffs_and_scale(y_coords, scale=y_scale,
                                                 axis_name="y")

    # Determine rectangle calculation method
    calc_rect = FUNC_MAP[(x_scale in LIN_SCALES, y_scale in LIN_SCALES)]
    
    # Need two additional borders: the lower border of the first rectangle and
    # the upper border of the last rectangle.
    x_diffs = expand_diffs(x_diffs, extend=extend, default=default_distance)
    y_diffs = expand_diffs(y_diffs, extend=extend, default=default_distance)

    # Divide by two to have the _sizes_ of the rectangle, not the distance
    # between the points, and then apply the size factor.
    x_sizes = x_diffs / 2. * size_factor
    y_sizes = y_diffs / 2. * size_factor

    # Create rects Dataset, which is populated below. It contains the positions
    # and sizes as separate data variables.
    shape = (len(x_coords), len(y_coords))
    rect_spec = np.dtype([('xy', (float, (2,))),
                          ('width', float), ('height', float)])
    rects = xr.Dataset(data_vars=dict(
                            pos_x=(('x', 'y'), np.full(shape, np.nan)),
                            pos_y=(('x', 'y'), np.full(shape, np.nan)),
                            len_x=(('x', 'y'), np.full(shape, np.nan)),
                            len_y=(('x', 'y'), np.full(shape, np.nan)),
                            rect_spec=(('x', 'y'), np.full(shape, np.nan,
                                                           dtype=rect_spec)),
                            ),
                       coords=dict(
                            x=(('x',), x_coords),
                            y=(('y',), y_coords)
                       ))

    # Now, iterate over the coordinate combinations, calculate the rectangle
    # specification, and store it in the data array.
    it = itertools.product(enumerate(rects.coords['x']),
                           enumerate(rects.coords['y']))
    for (x_idx, x_coord), (y_idx, y_coord) in it:
        # Get the relevant sizes calculated from the diffs and calculate the
        # rectangle specification from that
        pos_x, pos_y, len_x, len_y = calc_rect(x_coord, y_coord,
                                               (x_sizes[x_idx],      # left
                                                x_sizes[x_idx + 1]), # right
                                               (y_sizes[y_idx],      # bottom
                                                y_sizes[y_idx+1]))   # top

        # Store in the corresponding data variable in the xr.Dataset
        rects['pos_x'][x_idx, y_idx] = pos_x
        rects['pos_y'][x_idx, y_idx] = pos_y
        rects['len_x'][x_idx, y_idx] = len_x
        rects['len_y'][x_idx, y_idx] = len_y
        rects['rect_spec'][x_idx, y_idx] = (pos_x, pos_y), len_x, len_y

    # Calculate limits of the space that is covered by the rectangles
    x_lims = determine_limits(coords=x_coords, sizes=x_sizes, scale=x_scale)
    y_lims = determine_limits(coords=y_coords, sizes=y_sizes, scale=y_scale)

    # Done.
    return rects, dict(x=x_lims, y=y_lims)
