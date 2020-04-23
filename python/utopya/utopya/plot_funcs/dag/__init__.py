"""This subpackge contains generic plot function implementations, which rely on
the DAG framework to select and transform the data.

The plot functions then take care to visualize the data. They should NOT be
specialized to work only for one kind of creator, but focus on the creating a
visualization from the given data.
"""

from .time_series import time_series
from .graph import draw_graph