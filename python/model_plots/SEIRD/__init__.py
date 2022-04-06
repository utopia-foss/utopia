"""Plots that are specific to the SEIRD Model"""

# Register custom operations
from utopya.plotting import register_operation

from .operations import compute_age_distribution as _compute_age_distribution

register_operation(
    name="SEIRD.compute_age_distribution", func=_compute_age_distribution
)


# Make them available here to allow easier import
from .state import phase_diagram
