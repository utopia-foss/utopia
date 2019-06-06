"""This module implements the ModelRegistry, which combines ModelRegistryEntry
objects and makes it possible to register new models.
"""

import os
import copy
import logging
from itertools import chain
from typing import Dict

import dantro.utils

from .._yaml import yaml, write_yml, load_yml
from ..cfg import UTOPIA_CFG_DIR
from ..tools import pformat, recursive_update

from .entry import ModelRegistryEntry

# Local constants
log = logging.getLogger(__name__)

# -----------------------------------------------------------------------------

class KeyOrderedDict(dantro.utils.KeyOrderedDict):
    """A key-ordered dict that expects string keys and sorts by the lower-case
    representation of keys.
    """
    DEFAULT_KEY_COMPARATOR = lambda _, k: k.lower()

# -----------------------------------------------------------------------------

class ModelRegistry:
    """The ModelRegistry class takes care of providing model information to the
    rest of the utopya package and, at the same time, maintains the model
    registry it is associated with.

    It provides a dict-like interface to access the stored registry entries
    under their model name. Via ``reigster_model_info``, a model can be
    registered and information can be added to it.

    Additionally, there are some functions that provide an overview over the
    registered models and with which information they 

    # TODO Consider making this class a singleton?
    """

    def __init__(self, utopia_cfg_dir_path: str=UTOPIA_CFG_DIR):
        """Loads the utopia model registry from the configuration at the given
        path.
        
        Args:
            utopia_cfg_dir_path (str, optional): The path to store the model
                registry folder in.
        """
        # Store paths
        self._paths = dict()
        self._paths['utopia_cfg'] = utopia_cfg_dir_path
        self._paths['registry'] = os.path.join(self._paths['utopia_cfg'],
                                               'models')

        # If the directories at these paths do not exist, create them
        if not os.path.exists(self.registry_dir):
            # Suffices to create registry dir
            os.makedirs(self.registry_dir)

        # Create the model registry dict and populate it
        #   Keys:    model names
        #   Values:  ModelRegistryEntry objects
        self._registry = KeyOrderedDict()
        self._load_from_registry_dir()

        log.info("Ready. Have %d model%s registered.",
                 len(self), "s" if len(self) != 1 else "")
    
    @property
    def registry_dir(self) -> str:
        """The model registry directory path"""
        return self._paths['registry']

    def __len__(self) -> int:
        return len(self._registry)

    # Information .............................................................

    def __str__(self) -> str:
        return ("<Utopia Model Registry; {} model{} registered>"
                "".format(len(self), "s" if len(self) != 1 else ""))

    @property
    def info_str(self) -> str:
        lines = []
        lines.append("Utopia Model Registry ({} model{} registered)"
                     "".format(len(self), "s" if len(self) != 1 else ""))

        for model_name, _ in self.items():
            lines.append("  - {}".format(model_name))

        return "\n".join(lines)

    @property
    def info_str_detailed(self) -> str:
        lines = []
        lines.append("Utopia Model Registry ({} model{} registered)"
                     "".format(len(self), "s" if len(self) != 1 else ""))

        for model_name, entry in self.items():
            lines.append("  - {:19s} {}".format(model_name, entry))

        return "\n".join(lines)

    # TODO Improve output formats and amount of information

    # Dict interface ..........................................................

    def keys(self):
        return self._registry.keys()
    
    def values(self):
        return self._registry.values()
    
    def items(self):
        return self._registry.items()

    def __contains__(self, model_name: str) -> bool:
        """Whether an entry for the given model name exists in the registry"""
        return model_name in self._registry

    # Working on entries ......................................................

    def __getitem__(self, model_name: str) -> ModelRegistryEntry:
        """Retrieve a deep copy of a model registration entry for the given
        model name.
        """
        try:
            return self._registry[model_name]

        except KeyError as err:
            raise KeyError("No model with name '{}' found! Did you forget "
                           "to register it? Available models: {}"
                           "".format(model_name, ", ".join(self.keys()))
                           ) from err

    def register_model_info(self, model_name: str, *,
                            skip_existing: bool=False,
                            clear_existing: bool=False,
                            extend_existing: bool=False,
                            **bundle_kwargs) -> ModelRegistryEntry:
        """Register information for a single model. This method also allows to
        create a new entry if a model does not exist.
        
        However, it will raise an error if the model was already registered and
        neither the skip nor the remove options were explicitly specified.
        
        Args:
            model_name (str): The name of the model to register
            skip_existing (bool, optional): Whether to skip registration if a
                model of that name already exists. In that case returns the
                already registered entry.
            clear_existing (bool, optional): Whether to clear an existing
                entry for the given model name before adding. This removes all
                existing config bundles!
            extend_existing (bool, optional): Whether to extend an existing
                model entry. May raise an error if the same bundle is already
                present.
            **bundle_kwargs: Passed on to ModelRegistryEntry.add_bundle
        
        Returns:
            ModelRegistryEntry: The registry entry for this model.
        
        Raises:
            ValueError: On already existing model if neither skip nor remove
                options were set.
        """
        # Register the model, if not already done
        if model_name not in self:
            self._add_entry(model_name)

        elif skip_existing:
            log.debug("Model '%s' already registered. Skipping ...",
                      model_name)
            return self[model_name]

        elif clear_existing:
            log.debug("Removing existing configuration bundles for model "
                      "'%s' ...", model_name)
            self[model_name].clear()

        elif not extend_existing:
            raise ValueError("A registry entry for model '{}' already exists! "
                             "To add a configuration bundle to it, use its "
                             "add_bundle method or pass the extend_existing "
                             "or clear_existing arguments to this method."
                             "".format(model_name))

        # If this point is reached, a bundle is also to be added.
        if bundle_kwargs:
            self[model_name].add_bundle(**bundle_kwargs)

        # To be consistent, return the entry, not the bundle
        return self[model_name]

    def remove_entry(self, model_name: str):
        """Removes a registry entry and deletes the associated registry file"""
        try:
            entry = self._registry.pop(model_name)
        
        except KeyError as err:
            raise KeyError("Could not remove entry for model '{}', because "
                           "no such model is registered. Available models: "
                           "{}".format(model_name, ", ".join(self.keys()))
                           ) from err
        
        os.remove(entry.registry_file_path)
        log.debug("Removed entry for model '%s' from Utopia Model Registry "
                  "and removed associated registry file at %s.",
                  model_name, entry.registry_file_path)
        # Entry goes out of scope now and is then be garbage-collected if it
        # does not exist anywhere else... Only if some action is taken on that
        # entry does it lead to file being created again.

    # Helpers .................................................................

    def _add_entry(self, model_name: str) -> ModelRegistryEntry:
        """Create a ModelRegistryEntry for the given model, which loads the
        associated data from the registry directory, and store it in the
        registry.
        
        Args:
            model_name (str): Model name for which to add an ModelRegistryEntry
                object.
        
        Raises:
            ValueError: If the model already exists.        
        
        Returns:
            ModelRegistryEntry: The newly created entry
        """
        if model_name in self:
            raise ValueError("There already is a model registered under the "
                             "name of '{}'! Use the add_bundle method to add "
                             "information to it.".format(model_name))

        entry = ModelRegistryEntry(model_name, registry_dir=self.registry_dir)
        self._registry[entry.model_name] = entry

        log.debug("Added registry entry for model '%s'.", entry.model_name)
        return entry

    def _load_from_registry_dir(self):
        """Load all available entries from the registry directory.

        If called multiple times, will only load entries that are not already
        registered.
        """
        log.info("Loading entries from model registry directory:\n  %s ...",
                 self.registry_dir)

        # Go over files in registery directory
        new_entries = []
        for fname in os.listdir(self.registry_dir):
            # Get the model name from the file name
            model_name, ext = os.path.splitext(fname)
            
            # Continue only if it is a YAML file
            if not ext.lower() == '.yml' or model_name in self:
                continue

            self._add_entry(model_name)
            new_entries.append(model_name)

        log.debug("Loaded %s new entr%s: %s",
                  len(new_entries), "ies" if len(new_entries) != 1 else "y",
                  ", ".join(new_entries))
