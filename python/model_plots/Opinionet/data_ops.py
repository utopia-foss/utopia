import numpy as np
from scipy.signal import find_peaks

from utopya.plotting import register_operation

# .. Statistics on opinion data ...............................................

def op_number_of_peaks(data, bins: int, range=None, **find_peaks_kwargs):
    hist, _ = np.histogram(data, range=range, bins=bins)
    peak_number = len(find_peaks(hist, **find_peaks_kwargs)[0])
    return peak_number

def op_localization(data, bins: int, range=None):
    hist, _ = np.histogram(data, range=range, bins=bins, density=True)
    hist /= bins

    hist_squared = hist**2
    norm = np.sum(hist_squared**2)
    l = norm / np.sum(hist_squared)**2
    return l

def op_polarization(data):
    return sum(
        (data[i] - data[j])**2
        for i in range(len(data))
        for j in range(len(data))
    )
    
register_operation(name="op_number_of_peaks", func=op_number_of_peaks)
register_operation(name="op_localization", func=op_localization)
register_operation(name="op_polarization", func=op_polarization)
