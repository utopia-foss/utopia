"""Implementation of the Multiverse class.

The Multiverse supplies the main user interface of the frontend.
"""

import os
import time


class Multiverse:
    def __init__(self):
        """ Initialisation of the Multiverse

        The __init__ reads out the yaml cfg files,
        creates folders,
        ...
        """
        self.path_dict = None
        # read in the dictionary above with #14, base from user specific file #15
        # {base : absolute path (default is home directory), out_dir : , ....}
        self.path_simulation = None
        self._create_sim_dir()

    def _create_sim_dir(self):
        """ The _create_sim_dir provides the output folder structure.

        The function checks, if the folders are already there,
        if not they are created. If the exact same folder tree (with timestamp)
        already exists a Error is thrown.

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
        self.path_dict = dict(base=None, out_dir='utopia_output', model_name='', model_note='test')

        # use recursive makedirs
        if self.path_dict['base'] is None:
            self.path_simulation = os.path.expanduser("~")
        else:
            self.path_simulation = self.path_dict['base']
        self.path_simulation = os.path.join(self.path_simulation, self.path_dict['out_dir'], self.path_dict['model_name'])
        self.path_simulation = os.path.join(self.path_simulation, time.strftime("%Y")+time.strftime("%m")+time.strftime("%d")+"_"+time.strftime("%H")+time.strftime("%M")+time.strftime("%S"))
        if self.path_dict['model_note']:
            self.path_simulation += "_"+self.path_dict['model_note']
        # recursive folder creation automatic error if already existing
        os.makedirs(self.path_simulation)
        # make Subfolders
        # folder list expandable or configured by cfg yaml ?
        folder_list = ["config", "eval", "universes"]
        for folder in folder_list:
            os.mkdir(os.path.join(self.path_simulation, folder))

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
        path_universe = "uni"+str(uni_no).zfill(max_dim_no)
        pathname = os.path.join(self.path_simulation, "universes", path_universe)
        # recursive folder creation automatic error if already existing
        os.makedirs(pathname)
