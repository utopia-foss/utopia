import numpy as np
from scipy.signal import find_peaks

from utopya.eval import is_operation

# .. Helper functions .........................................................


def apply_along_dim(func):
    """Decorator which allows for applying a function, which acts on an
    array-like, along a dimension of a xarray.DataArray.
    """

    def _apply_along_axis(data, along_dim: str = None, *args, **kwargs):
        if along_dim is not None:
            ax = data.get_axis_num(along_dim)
            return np.apply_along_axis(
                func, axis=ax, arr=data, *args, **kwargs
            )
        else:
            return func(data, *args, **kwargs)

    return _apply_along_axis


# .. Statistics on opinion data ...............................................


@is_operation("Opinionet.op_number_of_peaks")
@apply_along_dim
def op_number_of_peaks(data, *, bins: int, interval, **find_peaks_kwargs):
    hist, _ = np.histogram(data, range=interval, bins=bins)
    peak_number = len(find_peaks(hist, **find_peaks_kwargs)[0])
    return peak_number


@is_operation("Opinionet.op_localization")
@apply_along_dim
def op_localization(data, *, bins: int, interval):
    hist, _ = np.histogram(data, range=interval, bins=bins, density=True)
    hist /= bins

    hist_squared = hist**2
    norm = np.sum(hist_squared**2)
    l = norm / np.sum(hist_squared) ** 2
    return l


@is_operation("Opinionet.op_polarization")
@apply_along_dim
def op_polarization(data, interval):
    p = sum(
        (data[i] - data[j]) ** 2
        for i in range(len(data))
        for j in range(len(data))
    )
    # normalize to data size and interval width
    p *= 2.0 / len(data) ** 2
    p /= (interval[1] - interval[0]) ** 2
    return p
