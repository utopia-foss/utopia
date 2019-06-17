"""This module implements the ModelRegistryEntry class, which contains a set
of ModelInfoBundle objects. These can be combined in a labelled and unlabelled
fashion.
"""

import os
import time
import copy
import logging
from itertools import chain
from typing import Tuple, Union, Iterator, Generator

from .._yaml import write_yml, load_yml
from ..cfg import UTOPIA_CFG_DIR
from ..tools import pformat

from ._exceptions import ModelRegistryError, BundleExistsError
from .info_bundle import TIME_FSTR, ModelInfoBundle

# Local constants
log = logging.getLogger(__name__)


# -----------------------------------------------------------------------------

class ModelRegistryEntry:
    """Holds model config bundles for a single model.

    Instances of this class are associated with a file in the model registry
    directory. Upon change, they update that file.
    """
    
    def __init__(self, model_name: str, *, registry_dir: str):
        """Initialize a model registry entry from a registry directory.

        Expects a file named <model_name>.yml in the registry directory. If it
        does not exist, creates an empty one.
        """
        self._model_name = model_name
        self._registry_dir = registry_dir

        self._labelled = dict()
        self._unlabelled = list()

        # If no file exists for this entry, create empty one. Otherwise: load.
        if not os.path.exists(self.registry_file_path):
            log.debug("Creating empty registry file for registry entry of "
                      "model '%s' ...", self.model_name)
            self._update_registry_file()  # creates file if path doesn't exist

        else:
            self._load_from_registry()
        
        log.debug("Initialized registry entry for model '%s' with a total "
                  "of %d bundles.", self.model_name, len(self))

    @property
    def model_name(self) -> str:
        return self._model_name

    @property
    def registry_dir(self) -> str:
        return self._registry_dir

    @property
    def registry_file_path(self) -> str:
        return os.path.join(self.registry_dir, self.model_name + ".yml")

    # Magic methods ...........................................................

    def __len__(self) -> int:
        """Returns total number of bundles, labelled and unlabelled."""
        return len(self._labelled) + len(self._unlabelled)

    def __contains__(self, obj) -> bool:
        """Checks if the given _object_ is part of this registry entry.

        Note that this does not check for keys!
        """
        return obj in self.values()

    def __str__(self) -> str:
        return ("<{} for '{}' model; {} bundle{} ({} labelled)>"
                "".format(type(self).__name__, self.model_name,
                          len(self), "s" if len(self) != 1 else "",
                          len(self._labelled)))

    def __eq__(self, other) -> bool:
        """Check for equality by inspecting stored bundles and model name"""
        if not isinstance(other, ModelRegistryEntry):
            return False
        return self._as_dict(self) == other._as_dict(other)

    # Access ..................................................................

    def __getitem__(self, key: Union[str, int]) -> ModelInfoBundle:
        """Return a bundle for the given label or index. If None, returns the
        tries to return the item.
        """
        if isinstance(key, int):
            return self._unlabelled[key]

        elif key is None:
            return self.item()

        else:
            return self._labelled[key]

    def item(self) -> ModelInfoBundle:
        """Retrieve the single bundle for this model, if not ambiguous."""
        if len(self) != 1:
            raise ModelRegistryError("Could not select single bundle from {}, "
                                     "because there are zero or more than one "
                                     "bundles stored in it. Use __getitem__ "
                                     "and the .available property instead."
                                     "".format(self))

        return self[list(self.keys())[0]]

    def keys(self) -> Iterator[Union[str, int]]:
        """Returns legitimate keys for item access, i.e.: all registered keys.
        Starts with labelled bundle names, continues with indices of unlabelled
        bundles.
        """
        return chain(sorted(self._labelled.keys()),
                     range(len(self._unlabelled)))

    def values(self) -> Generator[ModelInfoBundle, None, None]:
        """Returns stored model config bundles, starting with labelled ones"""
        for k in self.keys():
            yield self[k]
    
    def items(self) -> Generator[Tuple[Union[str, int], ModelInfoBundle],
                                 None, None]:
        """Returns keys and registered bundles, starting with labelled ones"""
        for k in self.keys():
            yield k, self[k]
    
    # Manipulation ............................................................

    def add_bundle(self, *, label: str=None, overwrite_label: bool=False,
                   update_registry_file: bool=True, **bundle_kwargs
                   ) -> ModelInfoBundle:
        """Add a new configuration bundle to this registry entry.
        
        This makes sure that the added bundle does not compare equal to an
        already existing bundle.
        
        Args:
            label (str, optional): The label under which to add it. If None,
                will be added as unlabelled.
            overwrite_label (bool, optional): If True, overwrites an existing
                bundle under the same label. This option does not affect
                unlabelled bundles or registry file updates.
            update_registry_file (bool, optional): Whether to write changes
                directly to the registry file.
            **bundle_kwargs: Passed on to construct the ModelInfoBundle that
                is to be stored.
        
        Raises:
            ModelRegistryError: If exist_ok is False and such a bundle already
                exists in this registry entry.
        """
        bundle = ModelInfoBundle(model_name=self.model_name, **bundle_kwargs)

        if bundle in self:
            raise BundleExistsError("A bundle that compared equal to the to-"
                                    "be-added bundle already exists in {}! "
                                    "Not adding it again.\n{}"
                                    "".format(self, bundle))
            # TODO should this warn instead of raising?!

        # Depending on whether a label was specified, decide on the container
        # to add the new bundle to.
        if label is None:
            log.debug("Adding unlabelled config bundle for model '%s' ...",
                      self.model_name)
            self._unlabelled.append(bundle)

        else:
            if isinstance(label, int):
                raise TypeError("Argument label for model registry entry '{}' "
                                "may not be an int!"
                                "".format(self.model_name))

            if not overwrite_label and label in self._labelled:
                raise ModelRegistryError("A bundle with label '{}' already "
                                         "exists in {}! Remove it first."
                                         "".format(label, self))

            log.debug("Adding labelled config bundle '%s' for model '%s' ...",
                      label, self.model_name)
            self._labelled[label] = bundle

        if update_registry_file:
            self._update_registry_file()

        return bundle

    def pop(self, key: Union[str, int], *,
            update_registry_file: bool=True) -> ModelInfoBundle:
        """Pop a configuration bundle from this entry."""
        if isinstance(key, int):
            log.debug("Popping unlabelled config bundle from index %d of "
                      "registry entry for model '%s' ...",
                      key, self.model_name)
            bundle = self._unlabelled.pop(key)
        
        else:
            log.debug("Removing labelled config bundle '%s' from registry "
                      "entry for model '%s' ...", key, self.model_name)
            bundle = self._labelled.pop(key)

        if update_registry_file:
            self._update_registry_file()

        return bundle

    def clear(self, *, update_registry_file: bool=True):
        """Removes all configuration bundles from this entry."""
        self._labelled = dict()
        self._unlabelled = list()
        
        if update_registry_file:
            self._update_registry_file()

    # Loading and Storing .....................................................

    def _load_from_registry(self):
        """Load the YAML registry entry for this model from the associated
        registry file path.
        """        
        try:
            obj = load_yml(self.registry_file_path)

        except Exception as exc:
            raise type(exc)("Failed loading model registry file from {}!"
                            "".format(self.registry_file_path)) from exc

        # Loaded successfully
        # If a model name is given, make sure it matches the file name
        if self.model_name != obj.get('model_name', self.model_name):
            raise ValueError("Mismatch between expected model name '{}' "
                             "and the model name '{}' specified in the "
                             "registry file at {}! Check the file name and "
                             "the registry file and make sure the model name "
                             "matches exactly."
                             "".format(self.model_name, obj['model_name'],
                                       self.registry_file_path))

        # Populate self. Need not update because content is freshly loaded.
        bundles = obj.get('cfg_bundles', {})
        for kwargs in bundles.get('unlabelled', []):
            self.add_bundle(**kwargs, update_registry_file=False)

        for label, kwargs in bundles.get('labelled', {}).items():
            self.add_bundle(label=label, **kwargs, update_registry_file=False)

    def _update_registry_file(self, *, overwrite_existing: bool=True) -> str:
        """Stores a YAML representation of this bundle in a file in the given
        directory and returns the full path.

        The file is saved under ``<model_name>.yml``, preserving the case of
        the model name.
        Before saving, this makes sure that no file exists in that directory
        whose lower-case version would compare equal to the lower-case version
        of this model.
        """
        if not os.path.exists(self.registry_dir):
            os.makedirs(self.registry_dir)

        fpath = self.registry_file_path
        fname = os.path.basename(fpath)

        # Check for duplicates, including lower case
        lc_duplicates = [fn for fn in os.listdir(self.registry_dir)
                         if fn.lower() == fname.lower()]
        if lc_duplicates and not overwrite_existing:
            raise FileExistsError("At least one file with a file name "
                                  "conflicting with '{}' was found in {}: {}. "
                                  "Manually delete or rename the conflicting "
                                  "file(s), taking care to not mix cases."
                                  "".format(fname, self.registry_dir,
                                            ", ".join(lc_duplicates)))

        # Write to separate location; move only after write was successful.
        write_yml(self, path=fpath + ".tmp")
        os.replace(fpath + ".tmp", fpath)
        # NOTE fpath need not exist for os.replace to work

        log.debug("Successfully stored %s at %s.", self, fpath)

    # YAML representation .....................................................

    @classmethod
    def _as_dict(cls, obj) -> dict:
        """Return a copy of the dict representation of this object"""
        d = dict(model_name=obj.model_name,
                 cfg_bundles=dict(labelled=obj._labelled,
                                  unlabelled=obj._unlabelled))
        return copy.deepcopy(d)

    @classmethod
    def to_yaml(cls, representer, node):
        """Creates a YAML representation of the data stored in this entry.
        As the data is a combination of a dict and a sequence, instances of
        this class are also represented as such.
        
        Args:
            representer (ruamel.yaml.representer): The representer module
            node (type(self)): The node to represent, i.e. an instance of this
                class.
        
        Returns:
            A YAML representation of the given instance of this class.
        """
        d = cls._as_dict(node)

        # Add some more information to it
        d['time_of_last_change'] = time.strftime(TIME_FSTR)

        # Return a YAML representation of the data
        return representer.represent_data(d)
