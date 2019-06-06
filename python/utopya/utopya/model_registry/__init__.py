"""This submodule implements a registry of Utopia models, which is used to
provide the required model information to the frontend and use it throughout.
"""
# Make names of class definitions available
from .info_bundle import ModelInfoBundle
from .entry import ModelRegistryEntry
from ._exceptions import ModelRegistryError

# Import the registry class only as "private"; should only be instantiated once
from .registry import ModelRegistry as _ModelRegistry
MODELS = _ModelRegistry()

# Make utility functions available which work on the created model registry
from .utils import get_info_bundle, load_model_cfg
