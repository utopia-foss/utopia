"""Test the plot_funcs._utils module"""

import itertools
from pkg_resources import resource_filename

import pytest

import numpy as np
import xarray as xr

import utopya
import utopya.plot_funcs._utils

# Fixtures --------------------------------------------------------------------


# Tests -----------------------------------------------------------------------

def test_calc_pxmap_rectangles():
    """Test the calc_pxmap_rectangles function"""

    cpr = utopya.plot_funcs._utils.calc_pxmap_rectangles

    def create_2D_data(*shape, dims=None,
                       coords_func=np.linspace,
                       rg_args=(0., 1.)):
        data = xr.DataArray(np.random.random(shape), dims=dims)
        data.coords[data.dims[0]] = coords_func(*rg_args, shape[0])
        data.coords[data.dims[1]] = coords_func(*rg_args, shape[1])
        return data

    # Basics . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
    shape = (6, 11)
    data = create_2D_data(*shape, dims=('foo', 'bar'))
    ds, lims = cpr(x_coords=data.coords['foo'],
                   y_coords=data.coords['bar'])

    # Limits should be [0., 1.] for both dimensions
    assert lims['x'] == (0., 1.)
    assert lims['y'] == (0., 1.)

    # The dataset should contain rectangle information for each point
    assert ds['pos_x'].shape == shape
    assert ds['pos_y'].shape == shape
    assert ds['len_x'].shape == shape
    assert ds['len_y'].shape == shape

    # Check the shape of some exemplary rectangles
    pt = ds.sel(x=0., y=0.)
    assert pt['pos_x'] == 0.
    assert pt['pos_y'] == 0.
    assert pt['len_x'] == 0.1
    assert pt['len_y'] == 0.05

    # With extended rectangles, the limits are adjusted . . . . . . . . . . . .
    shape = (4, 2)
    data = create_2D_data(*shape)
    ds, lims = cpr(x_coords=data.coords['dim_0'],
                   y_coords=data.coords['dim_1'],
                   extend=True)

    ext_x, ext_y = .5/(shape[0]-1), .5/(shape[1]-1)
    assert lims['x'] == (0. - ext_x, 1. + ext_x)
    assert lims['y'] == (0. - ext_y, 1. + ext_y)

    # The corresponding point should be extended beyond the coordinate space
    pt = ds.sel(x=0., y=0.)
    print(pt)
    assert pt['pos_x'] == 0. - ext_x
    assert pt['pos_y'] == 0. - ext_y
    assert pt['len_x'] == 1. / (shape[0] - 1)
    assert pt['len_y'] == 1. / (shape[1] - 1)
    assert (pt['rect_spec'].values['xy'] == [pt['pos_x'], pt['pos_y']]).all()
    assert pt['rect_spec'].values['width'] == pt['len_x']
    assert pt['rect_spec'].values['height'] == pt['len_y']

    # Unpacking also works; need .item call though
    (lambda *args: args)(*pt['rect_spec'].item())

    # Should be possible to only give one coordinate . . . . . . . . . . . . .
    ds, lims = cpr(x_coords=data.coords['dim_0'], y_coords=None,
                   default_distance=3.)

    assert lims['y'] == (-3/2, +3/2)


    ds, lims = cpr(x_coords=data.coords['dim_0'], y_coords=None, extend=True)
    assert lims['y'] == (-.5, +.5)


    # There is full coverage of the space . . . . . . . . . . . . . . . . . . .
    shapes = [(5, 11), (2, 3), (1, 2), (2, 1), (1, 1)]

    for ax, shape, ext in itertools.product(('x', 'y'), shapes, (True, False)):
        print("\n --- Checking coverage in {}-direction for shape {}, {} ---"
              "".format(ax, shape, "extended" if ext else "not extended"))

        # Create data
        data = create_2D_data(*shape)
        ds, lims = cpr(x_coords=data.coords['dim_0'],
                       y_coords=data.coords['dim_1'],
                       extend=ext)
        print("Limits:", lims[ax])

        # Determine lower borders
        b_lower = ds['pos_'+ax]
        print("Lower borders:", b_lower)

        # Determine upper borders
        b_upper = ds['pos_'+ax] + ds['len_'+ax]
        print("Upper borders:", b_upper)

        # Upper border of one rectangle should coincide with lower of the next
        if b_lower.sizes[ax] > 1 and b_upper.sizes[ax] > 1:
            assert np.isclose(b_upper[{ax: slice(None, -1)}].values,
                              b_lower[{ax: slice(1, None)}].values).all()
        # else: only a single rectangle in this direction

        # First rectangle's lower border is the lower limit
        assert np.isclose(b_lower[{ax: 0}].values, lims[ax][0]).all()

        # Last rectangle's upper border is the upper limit
        assert np.isclose(b_upper[{ax: -1}].values, lims[ax][1]).all()

    # Change scales . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
    # The ones below are only coverage tests

    shape = (5, 9)
    data = create_2D_data(*shape, coords_func=np.logspace, rg_args=(2, 6))

    # Logarithmic
    _, _ = cpr(x_coords=data.coords['dim_0'],
               y_coords=data.coords['dim_1'],
               x_scale='lin', y_scale='log')
    
    _, _ = cpr(x_coords=data.coords['dim_0'],
               y_coords=data.coords['dim_1'],
               x_scale='log', y_scale='lin')

    _, _ = cpr(x_coords=data.coords['dim_0'],
               y_coords=data.coords['dim_1'],
               x_scale='log', y_scale='log')

    # Categorical
    _, _ = cpr(x_coords=data.coords['dim_0'],
               y_coords=data.coords['dim_1'],
               x_scale='cat_at_value', y_scale='categorical')

    # Make sure the values are as expected
    ds, lims = cpr(x_coords=data.coords['dim_0'],
                   y_coords=data.coords['dim_1'],
                   x_scale='log', y_scale='log', extend=True)
    
    assert np.isclose(lims['x'], [10**1.5,  10**6.5], rtol=1.e-6).all()
    assert np.isclose(lims['y'], [10**1.75, 10**6.25], rtol=1.e-6).all()

    pt = ds.sel(x=1.e2, y=1.e2)
    assert np.isclose(pt['pos_x'], 10**1.5)
    assert np.isclose(pt['pos_y'], 10**1.75)
    assert np.isclose(pt['len_x'], 10**2.5 - 10**1.5)
    assert np.isclose(pt['len_y'], 10**2.25 - 10**1.75)

    
    # Errors . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
    with pytest.raises(ValueError, match="Invalid scale argument"):
        cpr(x_coords=data.coords['dim_0'], y_coords=data.coords['dim_1'],
            x_scale='foo')

    with pytest.raises(ValueError, match="need to be at least of length one"):
        cpr(x_coords=[], y_coords=None)
