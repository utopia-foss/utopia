"""Plots and operations specific to the SimpleFlocking model"""

from utopya.plotting import register_operation

from .operations import *
from .spatial import agents_in_domain

# Register custom operations
register_operation(name="set_nan_at_discontinuities",
                   func=set_nan_at_discontinuities)
register_operation(name="insert_nan_at_discontinuities",
                   func=insert_nan_at_discontinuities)
