"""Takes care of the YAML setup for Utopya"""

import re
import logging
from typing import Tuple

import numpy as np

import paramspace.yaml_constructors as pspyc

from . import MODELS
from ._yaml import yaml, load_yml, write_yml
from .model_registry import ModelInfoBundle, ModelRegistryEntry
from .stopcond import StopCondition
from .tools import recursive_update
from .model_registry import load_model_cfg

# Local constants
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

def _expr_constructor(loader, node):
    """Custom pyyaml constructor for evaluating strings with simple
    mathematical expressions.

    Supports: +, -, *, **, /, e-X, eX
    """
    # get expression string
    expr_str = loader.construct_scalar(node)

    # Remove spaces
    expr_str = expr_str.replace(" ", "")

    # Parse some special strings
    if expr_str in ['nan', 'NaN']:
        return float("nan")

    # NOTE these will cause errors if emitted file is not read by python!
    elif expr_str in ['np.inf', 'inf', 'INF']:
        return np.inf

    elif expr_str in ['-np.inf', '-inf', '-INF']:
        return -np.inf

    # remove everything that might cause trouble -- only allow digits, dot, +,
    # -, *, /, and eE to allow for writing exponentials
    expr_str = re.sub(r'[^0-9eE\-.+\*\/]', '', expr_str)

    # Try to eval
    return eval(expr_str)

def _model_cfg_constructor(loader, node) -> dict:
    """Custom yaml constructor for loading a model configuration file.

    This extracts the `model_name` key, loads the corresponding model config
    and then recursively updates the loaded config with the remaining keys
    from that part of the configuration.
    """
    # Get a mapping from the node
    d = loader.construct_mapping(node, deep=True)
    # NOTE using the deep flag here to allow nested calls to this constructor

    # Extract the model name and a potentially existing bundle key
    model_name = d.pop('model_name')
    bundle_key = d.pop('bundle_key', None)

    # Load the corresponding model configuration
    mcfg, _ = load_model_cfg(model_name=model_name, bundle_key=bundle_key)

    # Update the loaded config with the remaining keys
    mcfg = recursive_update(mcfg, d)

    # Return the updated dictionary
    return mcfg

# -----------------------------------------------------------------------------
# Attaching representers and constructors

# First register the classes, which directly implemented dumping/loading
yaml.register_class(StopCondition)
yaml.register_class(ModelInfoBundle)
yaml.register_class(ModelRegistryEntry)

# Now, add (additional, potentially overwriting) constructors for certain tags
# For the expression and model config objects.
yaml.constructor.add_constructor(u'!expr',
                                 _expr_constructor)
yaml.constructor.add_constructor(u'!model',
                                 _model_cfg_constructor)

# Add aliases for the (coupled) parameter dimensions
yaml.constructor.add_constructor(u'!sweep',
                                 pspyc.pdim)
yaml.constructor.add_constructor(u'!sweep-default',
                                 pspyc.pdim_default)

yaml.constructor.add_constructor(u'!coupled-sweep',
                                 pspyc.coupled_pdim)
yaml.constructor.add_constructor(u'!coupled-sweep-default',
                                 pspyc.coupled_pdim_default)

# Set the flow style
yaml.default_flow_style = False
