"""Implements data group classes specific to the Utopia output data structure.

They are based on dantro BaseDataGroup-derived implementations. In this module,
they are imported and configured using class variables.
"""

import logging

import numpy as np

import dantro as dtr
import dantro.groups

# Configure and get logger
log = logging.getLogger(__name__)

# Local constants

# -----------------------------------------------------------------------------

class UniverseGroup(dtr.groups.ParamSpaceStateGroup):
    """This group represents the data of a single universe"""

    def get_times_array(self, *, include_t0: bool=True) -> np.ndarray:
        """Constructs a 1D np.array using the information from this universe's
        configuration, i.e. the `num_steps` and `write_every` keys.
        
        Args:
            include_t0 (bool, optional): Whether to include an initial time
                step
        
        Returns:
            np.ndarray: The array of time steps
        """
        # Check if a configuration was loaded
        try:
            cfg = self['cfg']

        except KeyError as err:
            raise ValueError("No configuration associated with {}! Check the "
                             "load configuration of the root DataManager or "
                             "manually add a configuration container.   "
                             "".format(self.logstr)) from err
        
        # Retrieve the necessary parameters from the configuration
        num_steps = cfg['num_steps']
        write_every = cfg['write_every']

        # Calculate the times array, either with or without initial time step
        if include_t0:
            return np.linspace(0, num_steps,
                               num_steps//write_every + 1)
        else:
            return np.linspace(write_every, num_steps,
                               num_steps//write_every)


class MultiverseGroup(dtr.groups.ParamSpaceGroup):
    """This group is meant to manage the `uni` group of the loaded data, i.e.
    the group where output of all universe groups is stored in.

    Its main aim is to provide easy access to universes. By default, universes
    are only identified by their ID, which is a zero-padded _string_. This
    group adds the ability to access them via integer indices.

    Furthermore, via dantro, an easy data selector is available
    """
    _NEW_GROUP_CLS = UniverseGroup


class NetworkGroup(dtr.groups.NetworkGroup):
    """This group is meant to manage network data and create a NetworkX graph
    from it."""

    # Expected names for the containers that hold vertex/edge information
    _NWG_node_container = "_vertices"
    _NWG_edge_container = "_edges"

    # Expected _group_ attribute names determining the type of graph
    _NWG_attr_directed = "is_directed"
    _NWG_attr_parallel = "is_parallel"
