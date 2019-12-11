"""Implements the ModelInfoBundle class, which holds a single bundle of
information about a model.
"""
import os
import copy
import time
import logging
from typing import Sequence, Tuple, Union

from ..tools import pformat, load_selected_keys

# Local constants
log = logging.getLogger(__name__)

TIME_FSTR = "%y%m%d-%H%M%S"

# -----------------------------------------------------------------------------

class ModelInfoBundle:
    """A bundle of model information; behaves like a read-only dict"""

    # Which entries to expect inside the paths property
    PATH_KEYS = (('binary', str, True),
                 ('source_dir', str),  # TODO consider _requiring_ this one
                 ('default_cfg', str, True),
                 ('default_plots', str),
                 ('base_plots', str),
                 ('python_model_tests_dir', str),
                 ('python_model_plots_dir', str),
                 )

    # Path keys that are assumed to be relative to the source directory
    PATH_KEYS_REL_TO_SRC = ('default_cfg', 'default_plots', 'base_plots')

    # Paths to inspect in the source directory
    SRC_DIR_SEARCH_PATHS = (('default_cfg', '{}_cfg.yml'),
                            ('default_plots', '{}_plots.yml'),
                            ('base_plots', '{}_base_plots.yml'))

    # Which entries to expect inside the metadata property
    METADATA_KEYS = (('version', str),
                     ('long_name', str),
                     ('description', str),
                     ('author', str),
                     ('email', str),
                     ('website', str),
                     ('dependencies', list),
                     ('utopya_compatibility', str),
                     ('misc', dict))

    # .........................................................................

    def __init__(self, *, model_name: str,
                 paths: dict, metadata: dict=None,
                 project_name: str=None,
                 registration_time: str=None,
                 missing_path_action: str='log',
                 **additional_kwargs):
        """Initialize a ModelInfoBundle"""
        self._model_name = model_name
        self._reg_time = (registration_time if registration_time is not None
                          else time.strftime(TIME_FSTR))

        # Prepare data dict
        self._d = dict(paths=dict(), metadata=dict(),
                       project_name=project_name, **additional_kwargs)

        # Parse paths before loading them, already expanding the user '~' ...
        paths = self._parse_paths(**{k: os.path.expanduser(p)
                                     for k, p in paths.items()},
                                  missing_path_action=missing_path_action)

        # Populate it, checking some properties
        err_msg_fstr = "Failed loading '{}' info for '{}' model info bundle!"
        
        load_selected_keys(paths,
                           add_to=self.paths, keys=self.PATH_KEYS,
                           err_msg_prefix=err_msg_fstr.format('paths',
                                                              model_name))
        
        load_selected_keys((metadata if metadata is not None else {}),
                           add_to=self.metadata, keys=self.METADATA_KEYS,
                           err_msg_prefix=err_msg_fstr.format('metadata',
                                                              model_name))

        log.debug("Created configuration bundle for model '%s'.",
                  self.model_name)

    # Magic methods ...........................................................

    def __eq__(self, other) -> bool:
        """Compares equality by comparing the stored configuration. Only if
        another ModelInfoBundle is compared against does the model name also
        take part in the comparison.
        """
        if isinstance(other, ModelInfoBundle):
            return (    self._d == other._d
                    and self._model_name == other.model_name)
        return self._d == other

    # Formatting ..............................................................

    def __str__(self) -> str:
        return ("<Info bundle for '{}' model>\n{}\n"
                "".format(self._model_name, pformat(self._d)))

    # General Access ..........................................................
    
    @property
    def model_name(self) -> str:
        return self._model_name
    
    @property
    def registration_time(self) -> str:
        """Registration time string of this bundle"""
        return self._reg_time

    @property
    def as_dict(self) -> dict:
        """Returns a deep copy of all bundle data. This does NOT include the
        model name and the registration time."""
        return copy.deepcopy(self._d)

    def __getitem__(self, key: str):
        """Direct access to the full bundle data"""
        return self._d[key]
    
    @property
    def paths(self) -> dict:
        """Access to the paths information of the bundle"""
        return self._d['paths']
    
    @property
    def metadata(self) -> dict:
        """Access to the metadata information of the bundle"""
        return self._d['metadata']

    @property
    def project_name(self) -> str:
        """Access to the Utopia project name information of the bundle"""
        return self._d['project_name']

    @property
    def missing_paths(self) -> dict:
        """Returns those paths where os.path.exists did not evaluate to True"""
        return {k: p for k, p in self.paths.items() if not os.path.exists(p)}

    # Helpers .................................................................

    def _parse_paths(self, *, missing_path_action: str,
                     binary: str, base_bin_dir: str=None,
                     src_dir: str=None, base_src_dir: str=None,
                     **more_paths) -> dict:
        """Given path arguments, parse them into actual paths, e.g. by joining
        some together ...

        NOTE This assumes that all paths already got the os.path.expanduser
             treament.
        """
        # Make sure the base directories are not none (os.path.join easier)
        base_bin_dir = base_bin_dir if base_bin_dir is not None else ""
        base_src_dir = base_src_dir if base_src_dir is not None else ""

        # Create paths dict, basing it on those paths that will not be parsed
        paths = dict(**more_paths)

        # Now, populate that dict.
        # Easiest: binary path
        paths['binary'] = os.path.join(base_bin_dir, binary)

        # Prepare an absolute version of the source path
        abs_src_path = None
        if src_dir:
            abs_src_path = os.path.join(base_src_dir, src_dir)

        # If a source directory is given, store it, then auto-detect some files
        if abs_src_path:
            paths['source_dir'] = abs_src_path

            for key, fname_fstr in self.SRC_DIR_SEARCH_PATHS:
                # Build the full file path and see if a file exists there
                fname = fname_fstr.format(self.model_name)
                fpath = os.path.join(abs_src_path, fname)
                
                if os.path.exists(fpath):
                    paths[key] = fpath

        # Carry over configuration files, potentially overwriting existing and
        # resolving their potentially relative paths
        for key, path in more_paths.items():
            if key in self.PATH_KEYS_REL_TO_SRC and not os.path.isabs(path):
                # Is relative. Need a source directory to join it to ...
                if not abs_src_path:
                    raise ValueError("Given '{}' path ({}) was relative, but "
                                     "no source directory was specified!"
                                     "".format(key, path))

                path = os.path.join(abs_src_path, path)

            # All good, can now store it. If it was a path that is not to be
            # seen as relative to the source directory, that will lead to an
            # error being thrown below ...
            paths[key] = path

        # Done populating.
        # Make sure paths are all absolute and exist.
        for key, path in paths.items():
            if not os.path.isabs(path):
                raise ValueError("The given '{}' path ({}) for config bundle "
                                 "of model '{}' was not absolute! Please "
                                 "provide only absolute paths (may include ~)."
                                 "".format(key, path, self.model_name))
            
            if not os.path.exists(path):
                msg = ("Given '{}' path for model '{}' does not exist: {}"
                       "".format(key, self.model_name, path))
                
                if missing_path_action == 'warn':
                    log.warning(msg)
                
                elif missing_path_action in ('log', 'ignore'):
                    log.debug(msg)

                elif missing_path_action == 'raise':
                    raise ValueError(msg + "\nEither adjust the corresponding "
                                     "configuration bundle or place the "
                                     "expected file at that path. To ignore "
                                     "this error, pass `missing_path_action` "
                                     "argument to ModelInfoBundle.")

                else:
                    raise ValueError("Invalid missing_path_action '{}'! "
                                     "Choose from: ignore, log, warn, raise."
                                     "".format(missing_path_action))

        return paths

    # YAML representation .....................................................

    @classmethod
    def to_yaml(cls, representer, node):
        """Creates a YAML representation of the data stored in this bundle.

        Args:
            representer (ruamel.yaml.representer): The representer module
            node (type(self)): The node to represent, i.e. an instance of this
                class.
        
        Returns:
            A YAML representation of the given instance of this class.
        """
        return representer.represent_data(node._d)
