"""Implementation of the Multiverse class.

The Multiverse supplies the main user interface of the frontend.
"""
import yaml


class Multiverse:
    def __init__(self, metaconfig="metaconfig.yml", userconfig=None):
        """Initialize the setup.

        Load default configuration file and adjust parameters given
        by metaconfig and userconfig.
        """
        self._configure(metaconfig, userconfig)

    # TODO: move this function to tools?
    def _recursive_update(self, d: dict, u: dict):
        """Update Mapping d with values from Mapping u."""
        for key, val in u.items():
            if isinstance(val, dict):
                # Already a Mapping, continue recursion
                d[key] = self._recursive_update(d.get(key, {}), val)
            else:
                # Not a mapping -> at leaf -> update value
                d[key] = val 	# ... which is just u[k]
        return d

    def _configure(self, metaconfig: str, userconfig=None):
        """Read default configuration file and adjust parameters.

        The default metaconfig file, the user/machine-specific file (if
        existing) and the regular metaconfig file are read in and the default
        metaconfig is adjusted accordingly to create a single output file.
        """
        # read in all configuration files
        # TODO: outsource the statements below into a function in tools?
        try:
            with open("default_metaconfig.yml", 'r') as ymlfile:
                self._defaults = yaml.load(ymlfile)

        except FileNotFoundError:
            raise FileNotFoundError("default_metaconfig.yml is not present.")

        try:
            with open(metaconfig, 'r') as ymlfile:
                self._metaconfig = yaml.load(ymlfile)

        except FileNotFoundError:
            raise FileNotFoundError("Path to metaconfig was given but\
                                    metaconfig could not be found.")

        if(userconfig is not None):
            try:
                with open(userconfig, 'r') as ymlfile:
                    self._userconfig = yaml.load(ymlfile)
            except FileNotFoundError:
                raise FileNotFoundError("Path to userconfig was given but\
                                        userconfig could not be found.")

        # TODO: typechecks of values should be completed below here.
        # after this point it is assumed that all values are valid

        if userconfig is not None:
            self._defaults = self._recursive_update(self._defaults,
                                                    self._userconfig)

        self._defaults = self._recursive_update(self._defaults,
                                                self._metaconfig)
