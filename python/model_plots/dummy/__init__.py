"""Plots that are specific to the dummy model"""

# Make them all available here to allow easier import
from .state import *

# Register some custom operation with the DAG framework
from utopya.plotting import register_operation

register_operation(name="my_custom_dummy_operation",  # used for testing
                   func=lambda x: x)
