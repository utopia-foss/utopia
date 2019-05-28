"""This module holds data processing functionality that is used in the plot
functions.
"""

import operator
import logging
import warnings
from typing import Union, List, Tuple

import numpy as np
import xarray as xr
from scipy.signal import find_peaks

from dantro.base import BaseDataGroup

# Local variables .............................................................
log = logging.getLogger(__name__)


# A map from operator names to binary functions returning booleans
OPERATOR_MAP = {
    '==': operator.eq,  'eq': operator.eq,
    '<': operator.lt,   'lt': operator.lt,
    '<=': operator.le,  'le': operator.le,
    '>': operator.gt,   'gt': operator.gt,
    '>=': operator.ge,  'ge': operator.ge,
    '!=': operator.ne,  'ne': operator.ne,
    '^': operator.xor,  'xor': operator.xor,
    # Expecting an iterable as second argument
    'in':              (lambda x, y: x in y),
    'not in':          (lambda x, y: x not in y),
    # Performing bitwise boolean operations to support numpy logic
    'in interval':     (lambda x, y: x >= y[0] & x <= y[1]),
    'not in interval': (lambda x, y: x < y[0] | x > y[1]),
}

# Data Processing functions ---------------------------------------------------

def create_mask(data: xr.DataArray, *,
                operator_name: str, rhs_value: float) -> xr.DataArray:
    """Given the data, returns a binary mask by applying the following
    comparison: ``data <operator> rhs value``.
    
    Args:
        data (xr.DataArray): The data to apply the comparison to. This is the
            lhs of the comparison.
        operator_name (str): The name of the binary operator function as
            registered in utopya.tools.OPERATOR_MAP
        rhs_value (float): The right-hand-side value
    
    Raises:
        KeyError: On invalid operator name
    
    No Longer Returned:
        xr.DataArray: Boolean mask
    """
    # Get the operator function
    try:
        comp_func = OPERATOR_MAP[operator_name]

    except KeyError as err:
        raise KeyError("No operator with name '{}' available! "
                       "Available operators: {}"
                       "".format(operator_name,
                                 ", ".join(OPERATOR_MAP.keys()))
                       ) from err

    # Apply the comparison
    data = comp_func(data, rhs_value)

    # Create a new name
    name = data.name + " (masked by '{} {}')".format(operator_name, rhs_value)

    # Build a new xr.DataArray from that data, retaining all information
    return xr.DataArray(data=data,
                        name=name,
                        dims=data.dims,
                        coords=data.coords,
                        attrs=dict(**data.attrs))

def where(data: xr.DataArray, *,
          operator_name: str, rhs_value: float) -> xr.DataArray:
    """Filter elements from the given data according to a condition. Only
    those elemens where the condition is fulfilled are not masked.

    NOTE This leads to a dtype change to float.
    """
    # Get the mask and apply it
    return data.where(create_mask(data,
                                  operator_name=operator_name,
                                  rhs_value=rhs_value))


def count_unique(data, **kwargs):
    """Applies np.unique to the given data"""
    unique, counts = np.unique(data, return_counts=True)

    # Construct a new data array and return
    return xr.DataArray(data=counts,
                        name=data.name + " (unique counts)",
                        dims=('unique',),
                        coords=dict(unique=unique),
                        attrs=dict(**data.attrs))


# Transformation interface ----------------------------------------------------

# A mapping of transformation operations to callables.
# All callables expect xr.DataArray as their first argument and additional
# arguments as their second argument
TRANSFORMATIONS = {
    # Select data by value
    'sel':      lambda d, kws: d.sel(**(kws if kws else {})),

    # Select data by index
    'isel':     lambda d, kws: d.isel(**(kws if kws else {})),

    # Calculate the sum
    'sum':      lambda d, kws: d.sum(**(kws if isinstance(kws, dict)
                                        else dict(dim=kws))),

    # Calculate the mean
    'mean':     lambda d, kws: d.mean(**(kws if isinstance(kws, dict)
                                         else dict(dim=kws))),

    # Calculate the median
    'median':   lambda d, kws: d.median(**(kws if isinstance(kws, dict)
                                           else dict(dim=kws))),

    # Calculate the standard deviation
    'std':      lambda d, kws: d.std(**(kws if isinstance(kws, dict)
                                        else dict(dim=kws))),

    # Calculate the minimum
    'min':      lambda d, kws: d.min(**(kws if isinstance(kws, dict)
                                        else dict(dim=kws))),

    # Calculate the maximum
    'max':      lambda d, kws: d.max(**(kws if isinstance(kws, dict)
                                        else dict(dim=kws))),

    # Perform a squeeze operation
    'squeeze':  lambda d, kws: d.squeeze(**(kws if kws else {})),

    # Count unique values
    'count_unique': lambda d, _: count_unique(d),
    
    # Create a mask from the given data
    'create_mask':  
        lambda d, kws: create_mask(d, **(kws if isinstance(kws, dict)
                                         else dict(operator_name=kws[0],
                                                   rhs_value=kws[1]))),

    # Filter values that do not match a condition, i.e.: make them NaN
    'where':    lambda d, kws: where(d, **(kws if isinstance(kws, dict)
                                           else dict(operator_name=kws[0],
                                                     rhs_value=kws[1]))),

    # Logarithms
    'log':      lambda d, _: np.log(d),
    'log10':    lambda d, _: np.log10(d),
    'log2':     lambda d, _: np.log2(d),
    'log1p':    lambda d, _: np.log1p(d),

    # Powers
    'power':    lambda d, e: np.power(d, e),
    'squared':  lambda d, _: np.square(d),
    'sqrt':     lambda d, _: np.sqrt(d),
    'cubed':    lambda d, _: np.power(d, 3),
    'sqrt3':    lambda d, _: np.power(d, 1./.3),

    # Normalizations
    'normalize_to_sum':         lambda d, _: d / np.sum(d),
    'normalize_to_max':         lambda d, _: d / np.max(d),

    # Cumulation
    'cumulate':                 lambda d, _: np.cumsum(d),
    'cumulate_complementary':   lambda d, _: np.cumsum(d[::-1])[::-1],

    # Elementwise operations
    'add':      lambda d, v: operator.add(d, v),
    'concat':   lambda d, v: operator.concat(d, v),
    'div':      lambda d, v: operator.truediv(d, v),
    'truediv':  lambda d, v: operator.truediv(d, v),
    'floordiv': lambda d, v: operator.floordiv(d, v),
    'lshift':   lambda d, v: operator.lshift(d, v),
    'mod':      lambda d, v: operator.mod(d, v),
    'mul':      lambda d, v: operator.mul(d, v),
    'matmul':   lambda d, v: operator.matmul(d, v),
    'rshift':   lambda d, v: operator.rshift(d, v),
    'sub':      lambda d, v: operator.sub(d, v)
}

# End of TRANSFORMATIONS dict population
# .............................................................................


def transform(data: xr.DataArray, *operations: Union[dict, str],
              aux_data: Union[xr.Dataset, BaseDataGroup]=None,
              log_level: int=None) -> xr.DataArray:
    """Applies transformations to the given data, e.g.: reducing dimensionality
    or calculating 
    
    Args:
        data (xr.DataArray): The data that is to be reduced in dimensionality
        *operations (Union[dict, str]): Which operations to apply and with
            which parameters. These should be operation names or dicts. For
            dicts, they should only have a single key, which is the name of
            the operation to perform.
            The available operations are defined in the TRANSFORMATIONS dict.
        aux_data (Union[xr.Dataset, dantro.BaseDataGroup], optional): The
            auxiliary data for binary operations.
            NOTE Needed in those cases.
        log_level (int, optional): Which level to log the progress of the
            operations on. If not given, will be 10.
    
    Returns:
        xr.DataArray: A new object with dimensionality-reduced data.
    
    Raises:
        TypeError: On bad ``operations`` specification
        ValueError: On bad operation name
    """
    # Set default log level
    log_level = log_level if log_level is not None else 10

    log.log(log_level, "Performing %d transformation%s on input data:\n%s\n",
            len(operations), "s" if len(operations) != 1 else "", data)

    # Apply operations in the given order
    for i, op_spec in enumerate(operations):
        if isinstance(op_spec, str):
            op_name = op_spec
            op_spec = dict()
            may_fail = False

        elif not isinstance(op_spec, dict):
            raise TypeError("Entries of `operations` argument need to be dict"
                            "-like, but entry {} was {}!"
                            "".format(i, repr(op_spec)))

        else:
            # Get the names that don't start with _
            names = [k for k in op_spec.keys() if not k.startswith("_")]

            if len(names) != 1:
                raise ValueError("Could not extract an operation name from "
                                 "`operations` argument at index {}! For dict-"
                                 "like arguments, the dict need to contain "
                                 "one and only one key that does not start "
                                 "with '_'. Got keys: {}. Full dict: {}"
                                 "".format(i, ", ".join(names), op_spec))
            
            # Get the single name
            op_name = names[0]
            may_fail = op_spec.get('_may_fail', False)
            op_spec = op_spec[op_name]

        # Get the transform operation
        try:
            op_func = TRANSFORMATIONS[op_name]

        except KeyError as err:
            raise ValueError("No transform operation with name '{}' is "
                             "available! Choose from: {}"
                             "".format(op_name,
                                       ", ".join(TRANSFORMATIONS.keys()))
                             ) from err

        # Resolve auxilary data for binary operations. This changes op_spec
        # such that it is no longer the dict but the object that is to be used
        # as the right-hand side of a binary operation.
        if isinstance(op_spec, dict) and 'rhs_from' in op_spec:
            try:
                op_spec = aux_data[op_spec['rhs_from']]

            except KeyError as err:
                raise ValueError("The requested data path '{}' for the "
                                 "binary operation '{}' was not provided as "
                                 "part of the auxilary data. Available "
                                 "aux_data: {}"
                                 "".format(op_spec['rhs_from'], op_name,
                                           aux_data)) from err

        log.log(log_level, "Applying operation %d/%d:  %s  ...",
                i+1, len(operations), op_name)
        log.log(log_level, "  … with arguments:  %s", op_spec)

        # Catch and record warnings instead of displaying
        with warnings.catch_warnings(record=True) as caught_warnings:
            # Apply the transformation
            try:
                data = op_func(data, op_spec)

            except Exception as exc:
                if may_fail:
                    log.warning("Operation '%s' failed with %s: %s",
                                op_name, exc.__class__.__name__, exc)
                    continue
                raise

        # Emit any warnings that might have been generated
        if caught_warnings:
            log.log(log_level,
                    "Operation '%s' generated the following warnings:\n{}\n",
                    op_name,
                    "\n".join([warnings.formatwarning(w.message, w.category,
                                                      w.filename, w.lineno)
                               for w in caught_warnings]))

        log.log(log_level, "Result:\n%s\n", data)

    return data


# Data Analysis ---------------------------------------------------------------

def find_endpoint(data: xr.DataArray, *, time: int=-1,
                  **kwargs) -> Tuple[bool, xr.DataArray]:
    """Find the endpoint of a dataset wrt. ``time`` coordinate.
    
    This function is compatible with the
    :py:func:`utopya.plot_funcs.attractor.bifurcation_diagram`.
    
    Arguments:
        data (xr.DataArray): The data
        time (int, optional): The time index to select
        **kwargs: Passed on to ``data.isel`` call
    
    Returns:
        Tuple[bool, xr.DataArray]: (endpoint found, endpoint) 
    """
    return True, data.isel(time=time, **kwargs)

def find_fixpoint(data: xr.Dataset, *, spin_up_time: int=0, 
                  abs_std: float=None, rel_std: float=None, mean_kwargs=None, 
                  std_kwargs=None, isclose_kwargs=None, 
                  squeeze: bool=True) -> Tuple[bool, float]:
    """Find the fixpoint(s) of a dataset and confirm it by standard deviation.
    For dimensions that are not 'time' the fixpoints are compared and 
    duplicates removed.
    
    This function is compatible with the
    :py:func:`utopya.plot_funcs.attractor.bifurcation_diagram`.
    
    Arguments:
        data (xr.Dataset): The data
        spin_up_time (int, optional): The first timestep included
        abs_std (float, optional): The maximal allowed absolute standard 
            deviation
        rel_std (float, optional): The maximal allowed relative standard 
            deviation
        mean_kwargs (dict, optional): Additional keyword arguments passed on
            to the appropriate array function for calculating mean on data.
        std_kwargs (dict, optional): Additional keyword arguments passed on to 
            the appropriate array function for calculating std on data.
        isclose_kwargs (dict, optional): Additional keyword arguments passed
            on to the appropriate array function for calculating np.isclose for
            fixpoint-duplicates across dimensions other than 'time'.
        squeeze (bool, optional): Use the data.squeeze method to remove
            dimensions of length one. Default is True.
    
    Returns:
        tuple: (fixpoint found, mean) 
    """
    if squeeze:
        data = data.squeeze()
    if len(data.dims) > 2:
        raise ValueError("Method 'find_fixpoint' cannot handle data with more"
                         " than 2 dimensions. Data has dims {}"
                         "".format(data.dims))
    if spin_up_time > data.time[-1]:
        raise ValueError("Spin up time was chosen larger than actual simulation"
                         " time in module find_fixpoint. Was {}, but"
                         " simulation time was {}. .".format(spin_up_time,
                                                             data.time.data[-1]))
    
    # Get the data
    data = data.where(data.time >= spin_up_time, drop=True)

    # Calculate mean and std
    mean = data.mean(dim='time', **(mean_kwargs if mean_kwargs else {}))
    std = data.std(dim='time', **(std_kwargs if std_kwargs else {}))

    # Apply some masking, if parameters are given
    if abs_std is not None:
        mean = mean.where(std < abs_std)

    if rel_std is not None:
        mean = mean.where(std/mean < rel_std)

    conclusive = True
    for data_var_name, data_var in mean.data_vars.items():
        if data_var.shape:
            for i, val in enumerate(data_var[:-1]):
                mask = np.isclose(val, data_var[i+1:],
                                  **(isclose_kwargs if isclose_kwargs else {}))
                data_var[i+1:][mask] = np.nan

        conclusive = conclusive and (np.count_nonzero(~np.isnan(data_var)) > 0)
    
    return conclusive, mean

def find_multistability(*args, **kwargs) -> Tuple[bool, float]:
    """Find the multistabilities of a dataset.

    Performs find_fixpoint. Method conclusive if find_fixpoint conclusive with
    multiple entries.


    Arguments:
        *args, **kwargs: passed on to find_fixpoint

    Returns
        tuple: (multistability found, mean) 
    """
    conclusive, mean = find_fixpoint(*args, **kwargs)

    if not conclusive:
        return conclusive, mean
    
    for data_var_name, data_var in mean.data_vars.items():
        # Conclusive only if there are at least two non-nan values.
        # Count the non-zero entries of the inverse of boolian array np.isnan.
        # Need negation operator for that.
        if np.count_nonzero(~np.isnan(data_var)) > 1:
            return True, mean

    return False, mean

def find_oscillation(data: xr.Dataset, *, spin_up_time: int=0, 
                     squeeze: bool=True,
                     **find_peak_kwargs) -> Tuple[bool, list]:
    """Find oscillations in a dataset.
    
    This function is compatible with the
    :py:func:`utopya.plot_funcs.attractor.bifurcation_diagram`.
    
    Arguments:
        data (xr.Dataset): The data
        spin_up_time (int, optional): The first timestep included
        squeeze (bool, optional): Use the data.squeeze method to remove
            dimensions of length one. Default is True.
        **find_peak_kwargs: Passed on to ``scipy.signal.find_peaks``. Default
            for kwarg 'height' is -1e15.
    
    Returns:
        Tuple[bool, list]: (oscillation found, [min, max])
    """
    if squeeze:
        data = data.squeeze()
    if len(data.dims) > 1:
        raise ValueError("Method 'find_oscillation' cannot handle data with more"
                         " than 1 dimension. Data has dims {}"
                         "".format(data.dims))
    if spin_up_time > data.time[-1]:
        raise ValueError("Spin up time was chosen larger than actual simulation"
                         " time in module find_oscillation. Was {}, but"
                         " simulation time was {}. .".format(spin_up_time,
                                                             data.time.data[-1]))
    
    # Only use the data after spin up time
    data = data.where(data.time >= spin_up_time, drop=True)

    coords = {k: v for k, v in data.coords.items()}
    coords.pop('time', None)
    coords['osc'] = ['min', 'max']
    attractor = xr.Dataset(coords=coords, attrs={'conclusive': False})

    if not find_peak_kwargs['height']:
        find_peak_kwargs['height'] = -1e15

    for data_var_name, data_var in data.items():
        maxima, max_props = find_peaks(data_var, **find_peak_kwargs)
        amax = np.amax(data_var)
        minima, min_props = find_peaks(amax - data_var, **find_peak_kwargs)
        
        if not maxima.size or not minima.size:
            mean = data_var.mean(dim='time')
            attractor[data_var_name] = ('osc', [mean, mean])

        else:
            # Build (min, max) pair
            min_max = [amax - min_props['peak_heights'][-1],
                    max_props['peak_heights'][-1]]
            attractor[data_var_name] = ('osc', min_max)

            # at least one data_var performs oscillations
            attractor.attrs['conclusive'] = True

    if attractor.attrs['conclusive']:
        return True, attractor
    return False, attractor
