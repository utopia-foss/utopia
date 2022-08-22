"""Plots that are specific to the dummy model"""

# Make them all available here to allow easier import
# Register some custom operation with the DAG framework
from utopya.eval import register_operation

from .state import *

register_operation(
    name="my_custom_dummy_operation", func=lambda x: x  # used for testing
)
