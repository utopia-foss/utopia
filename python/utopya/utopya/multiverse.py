"""Implementation of the Multiverse class.

The Multiverse supplies the main user interface of the frontend.
"""
import yaml
# from .tools import recursive_update, read_yaml


class Multiverse:
    def __init__(self, metaconfig="metaconfig.yml", userconfig=None):
        """Initialize the setup.

        Load default configuration file and adjust parameters given
        by metaconfig and userconfig.
        """
        self._configure(metaconfig, userconfig)

    def _configure(self, metaconfig: str, userconfig: str=None):
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

        except FileNotFoundError as err:
            raise FileNotFoundError("default_metaconfig.yml is not present.") from err

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
