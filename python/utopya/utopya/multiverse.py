"""Implementation of the Multiverse class.

The Multiverse supplies the main user interface of the frontend.
"""

import os
import time


class Multiverse:
    def __init__(self):
        self.path_dict = None
        # read in the dictionary above with #14, base from user specific file #15
        self.simulation_path = None
        self._create_sim_dir()

    def _create_sim_dir(self):
        """ The _create_sim_dir provides the output folder structure.

        The function checks, if the folders are already there,
        if not they are created. If the exact same folder tree (with timestamp)
        already exists a Error is thrown.
        """
        self.path_dict = dict(base='./', out_dir='utopia_output', model_name='', model_note='test')

        # use recursive makedirs
        self.simulation_path = self.path_dict['base']+"/"+self.path_dict['out_dir']+"/"+self.path_dict['model_name']
        self.simulation_path += "./"+time.strftime("%Y")+time.strftime("%m")+time.strftime("%d")+"_"+time.strftime("%H")+time.strftime("%M")+time.strftime("%S")
        self.simulation_path += self.path_dict['model_note']
        # recursive folder creation automatic error if already existing
        os.makedirs(self.simulation_path)
        # make Subfolders
        # folder list expandable or configured by cfg yaml ?
        folder_list = ["config", "eval", "universes"]  
        for folder in folder_list:
            os.mkdir(self.simulation_path+"/"+folder)

    def _create_uni_dir(self, uni_no, max_dim_no):
        """ The _create_uni_dir generates the folder for a single univers.

        Within the universes directory, create a subdirectory uni<###>
        for the given universe number, preceding zeros are added.
        Thus they are sortable.
        """
        universe_path = "uni"+str(uni_no).zfill(max_dim_no)
        pathname = self.simulation_path+"universes"+universe_path
        # recursive folder creation automatic error if already existing
        os.makedirs(pathname)

'''
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
       180301125411_my_second_sim/
          ...
    model_b/
       180301125412_my_first_sim/
       180301125413_my_second_sim/
#'''
