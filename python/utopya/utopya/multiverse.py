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
        self._config = self._configure(metaconfig, userconfig)

    def _configure(self, metaconfig: str, userconfig: str=None) -> dict:
        """Read default configuration file and adjust parameters.

        The default metaconfig file, the user/machine-specific file (if existing) and the regular metaconfig file are read in and the default metaconfig is adjusted accordingly to create a single output file.

        Args:
            metaconfig: path to metaconfig. An empty or invalid path raises
                FileNotFoundError.
            userconfig: optional user/machine-specific configuration file

        Returns:
            dict: returns the updated default metaconfig to be processed further or to be written out.
        """
        # In the following, the final configuration dict is built from three components:
        # The base is the default configuration, which is always present
        # If a userconfig is present, this recursively updates the defaults
        # Then, the given metaconfig recursively updates the created dict
        defaults = read_yml("default_metaconfig.yml", error_msg="default_metaconfig.yml is not present.")

        if userconfig is not None:
            userconfig = read_yml(userconfig, error_msg="{0} was given but userconfig could not be found.".format(userconfig))

        metaconfig = read_yml(metaconfig, error_msg="{0} was given but metaconfig could not be found.".format(metaconfig))

        # TODO: typechecks of values should be completed below here.
        # after this point it is assumed that all values are valid

        # Now perform the recursive update steps
        if userconfig is not None:  # update default with user spec
            defaults = recursive_update(defaults, userconfig)

        # update default_metaconfig with metaconfig
        defaults = recursive_update(defaults, metaconfig)

        return defaults
