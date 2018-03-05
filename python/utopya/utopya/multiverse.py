"""Implementation of the Multiverse class.

The Multiverse supplies the main user interface of the frontend.
"""

import os
import time
import math
import logging

log = logging.getLogger(__name__)


class Multiverse:
    def __init__(self):
        """ Initialisation of the Multiverse

        The __init__ reads out the yaml cfg files,
        creates folders,
        ...
        """

        # FIXME
        self.cfg = dict()
        # FIXME dummy entry as long as cfg is not read in
        self.cfg['paths'] = dict(out_dir='~/utopia_output', model_name='', model_note='test')
        # read in the dictionary above with #14, base from user specific file #15

        self.dirs = dict()
        # {base : absolute path (default is home directory), config : , eval: }
        # make output directories
        self._create_sim_dir(**self.cfg['paths'])
        
    def _create_sim_dir(self, *, model_name: str, out_dir: str='~/utopia_output', model_note: str=None):
        """ The _create_sim_dir provides the output folder structure.

        The function checks, if the folders are already there,
        if not they are created. If the exact same folder tree (with timestamp)
        already exists a Error is thrown.

        Args:
            model_name: name of your model
            out_dir: location of output directory
            model_note: additional note to time stamp

        Folder Tree from Wiki
        utopia_output/   # all utopia output should go here
            model_a/
            180301125410_my_first_sim/
                config/
                eval/
                universes/
                    uni000/
                    uni001/
                    ...
            model_b/
            180301125412_my_first_sim/
            180301125413_my_second_sim/
        """
        # use recursive makedirs
        log.debug("Expanding user %s", out_dir)
        path_simulation = os.path.expanduser(out_dir)
        path_simulation = os.path.join(path_simulation, model_name, time.strftime("%Y%m%d_%H%M%S"))
        if model_note:
            path_simulation += "_"+model_note
        log.debug("Expanded user and time stamp to %s", path_simulation)

        # recursive folder creation automatic error if already existing
        os.makedirs(path_simulation)
        self.dirs['out_dir'] = path_simulation
        # make Subfolders
        # folder list expandable or configured by cfg yaml ?
        folder_list = ["config", "eval", "universes"]
        for folder in folder_list:
            os.mkdir(os.path.join(path_simulation, folder))
            self.dirs[folder] = os.path.join(path_simulation, folder)

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
