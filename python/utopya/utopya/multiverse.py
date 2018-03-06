"""Implementation of the Multiverse class.

The Multiverse supplies the main user interface of the frontend.
"""
from tools import recursive_update, read_yml


class Multiverse:
    def __init__(self, metaconfig: str="metaconfig.yml", userconfig: str=None):
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
        self._defaults = read_yml("default_metaconfig.yml", error_msg="default_metaconfig.yml is not present.")
        self._metaconfig = read_yml(metaconfig, error_msg="{0} was given but metaconfig could not be found.".format(metaconfig))

        if userconfig is not None:
            self._userconfig = read_yml(userconfig, error_msg="{0} was given but userconfig could not be found.".format(userconfig))

        # TODO: typechecks of values should be completed below here.
        # after this point it is assumed that all values are valid

        if userconfig is not None:  # update default with user spec
            self._defaults = recursive_update(self._defaults,
                                              self._userconfig)

        # update default_metaconfig with metaconfig
        self._defaults = recursive_update(self._defaults,
                                          self._metaconfig)
