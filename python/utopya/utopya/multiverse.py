"""Implementation of the Multiverse class.

The Multiverse supplies the main user interface of the frontend.
"""

import os
import time
import math
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

    def _create_uni_dir(self, uni_no, max_dim_no):
        """ The _create_uni_dir generates the folder for a single univers.

        Within the universes directory, create a subdirectory uni<###>
        for the given universe number, preceding zeros are added.
        Thus they are sortable.

        Args:
            uni_no: Number of the universe, created by the workermanager?,
                restoreable from meta_cfg.yaml.
            max_dim:no: Maximal number of universes calculated
                from param sweep from meta_cfg.yaml.
        """
        # calculate path for universe from maximum number, +1 needed because math.ceil >=
        # e.g. math.ceil(math.log(100,10)) gives 2 but 3 places needed
        path_universe = "uni"+str(uni_no).zfill(math.ceil(math.log(max_dim_no+1, 10)))
        # recursive folder creation automatic error if already existing
        pathname = os.path.join(self.dirs['universes'], path_universe)
        os.makedirs(pathname)
