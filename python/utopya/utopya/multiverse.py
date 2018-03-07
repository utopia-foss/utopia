"""Implementation of the Multiverse class.

The Multiverse supplies the main user interface of the frontend.
"""

import os
import time
import logging

from utopya.tools import recursive_update, read_yml

log = logging.getLogger(__name__)


class Multiverse:

    def __init__(self, run_cfg_path: str="run_config.yml", user_cfg_path: str=None):
        """Initialize the setup.

        Load base_configuration file and adjust parameters given
        by user_config and run_config.
        """
        # Initialise empty dict for config
        self._meta_config = dict()
        # Create Meta Config
        self._meta_config = self._create_meta_config(run_cfg_path=run_cfg_path, user_cfg_path=user_cfg_path)

        # Initialise empty dict for keeping track of directory paths
        self.dirs = dict()

        # Now create the simulation directory and its internal subdirectories
        #self._create_sim_dir(** model_name="test", self._meta_config['paths'])

    # Properties ..............................................................
    @property
    def meta_config(self) -> dict:
        """The meta_config."""
        return self._meta_config

    # Public API ..............................................................

    def prepare_universe(self):  # uni_id? max_uni_id?, other stuff
        # called from worker manager or inside here?
        # depending on whom is managing parameter sweeps
        # self._create_uni_dir()
        # self._create_uni_config()
        log.debug("Multiverse.prepare_universe called, but not implemented")

    # Non-public API ..........................................................

    def _create_meta_config(self, *, run_cfg_path: str, user_cfg_path: str=None) -> dict:
        """Read base configuration file and adjust parameters.

        The base_config file, the user_config file (if existing) and the run_config file are read in.
        The base_config is adjusted accordingly to create the meta_config.

        Args:
            run_cfg_path: path to run_config. An empty or invalid path raises
                FileNotFoundError.
            user_cfg_path: optional user_config file An invalid path raises
                FileNotFoundError.

        Returns:
            dict: returns the updated default metaconfig to be processed further or to be written out.
        """
        # In the following, the final configuration dict is built from three components:
        # The base configuration, which is always present
        # If a userconfig is present, this recursively updates the base
        # Then, the given run_config recursively updates the created dict
        defaults = read_yml("./utopya/base_config.yml", error_msg="base_config.yml is not present.")

        if user_cfg_path is not None:
            user_config = read_yml(user_cfg_path, error_msg="{0} was given but user_config.yaml could not be found.".format(user_cfg_path))

        run_config = read_yml(run_cfg_path, error_msg="{0} was given but run_config could not be found.".format(run_cfg_path))

        # TODO: typechecks of values should be completed below here.
        # after this point it is assumed that all values are valid

        # Now perform the recursive update steps
        if user_cfg_path is not None:  # update default with user spec
            defaults = recursive_update(defaults, user_config)

        # update default_metaconfig with metaconfig
        defaults = recursive_update(defaults, run_config)

        return defaults

    def _create_uni_config(self) -> dict: #other arguments, dummy function
        # what to really do here ?
        # return .yml file return dict ? save .yaml file at right position?
        log.debug("Multiverse._create_uni_config called, but not implemented")
       
    def _create_sim_dir(self, *, model_name: str, out_dir: str, model_note: str=None) -> None:
        """Create the folder structure for the simulation output.
    
        The following folder tree will be created
        utopia_output/   # all utopia output should go here
            model_a/
                180301-125410_my_model_note/
                    config/
                    eval/
                    universes/
                        uni000/
                        uni001/
                        ...
            model_b/
                180301-125412_my_first_sim/
                180301-125413_my_second_sim/


        Args:
            model_name (str): Description
            out_dir (str): Description
            model_note (str, optional): Description

        Raises:
            RuntimeError: If the simulation directory already existed. This
                should not occur, as the timestamp is unique. If it occurs,
                something is seriously wrong. Or you are in a strange time
                zone.
        """
        # NOTE could check if the model name is valid

        # Create the folder path to the simulation directory
        log.debug("Expanding user %s", out_dir)
        out_dir = os.path.expanduser(out_dir)
        sim_dir = os.path.join(out_dir,
                               model_name,
                               time.strftime("%Y%m%d-%H%M%S"))

        # Append a model note, if needed
        if model_note:
            sim_dir += "_" + model_note

        # Inform and store to directory dict
        log.debug("Expanded user and time stamp to %s", sim_dir)
        self.dirs['sim_dir'] = sim_dir

        # Recursively create the whole path to the simulation directory
        try:
            os.makedirs(sim_dir)
        except OSError as err:
            raise RuntimeError("Simulation directory already exists. This "
                               "should not have happened. Try to start the "
                               "simulation again.") from err

        # Make subfolders
        for subdir in ('config', 'eval', 'universes'):
            subdir_path = os.path.join(sim_dir, subdir)
            os.mkdir(subdir_path)
            self.dirs[subdir] = subdir_path

        log.debug("Finished creating simulation directory. Now registered: %s",
                  self.dirs)

    def _create_uni_dir(self, uni_id: int, max_uni_id: int) -> None:
        """The _create_uni_dir generates the folder for a single universe

        Within the universes directory, create a subdirectory uni### for the
        given universe number, zero-padded such that they are sortable.

        Args:
            uni_id (int): ID of the universe whose folder should be created. 
                Needs to be positive or zero.
            max_uni_id (int): highest ID, needed for correct zero-padding.
                Needs to be larger or equal to uni_id.
        """
        # Check if uni_id and max_uni_id are positive
        if uni_id < 0 or uni_id > max_uni_id:
            raise RuntimeError("Input variables don't match prerequisites: "
                               "uni_id >= 0, max_uni_id >= uni_id. Given arguments: "
                               "uni_id: {}, max_uni_id: {}".format(uni_id, max_uni_id))
                               
        # Use a format string for creating the uni_path
        fstr = "uni{id:>0{digits:}d}"
        uni_path = os.path.join(self.dirs['universes'],
                                fstr.format(id=uni_id, digits=len(str(max_uni_id))))

        # Now create the folder
        os.mkdir(uni_path)
        log.debug("Created universe path: %s", uni_path)
