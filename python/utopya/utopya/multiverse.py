"""Implementation of the Multiverse class.

The Multiverse supplies the main user interface of the frontend.
"""

import os
import time
import logging

log = logging.getLogger(__name__)


class Multiverse:
    def __init__(self, cfg: dict):
        """ Initialisation of the Multiverse

        The __init__ reads out the yaml cfg files,
        creates folders,
        ...
        """

        # Carry over config dictionary
        self.cfg = cfg
        # FIXME this should read in a yaml file rather than a dict
        # read in the dictionary above with #14, base from user specific file #15

        # Initialise empty dict for keeping track of directory paths
        self.dirs = dict()
        # make output directories

        # Now create the simulation directory and its internal subdirectories
        self._create_sim_dir(**self.cfg['paths'])
       
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
