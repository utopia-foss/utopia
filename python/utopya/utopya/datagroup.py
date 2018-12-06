"""Implements data group classes specific to the Utopia output data structure.

They are based on dantro BaseDataGroup-derived implementations. In this module,
they are imported and configured using class variables.
"""

import logging

import dantro as dtr
import dantro.groups

# Configure and get logger
log = logging.getLogger(__name__)

# Local constants

# -----------------------------------------------------------------------------

class UniverseGroup(dtr.groups.ParamSpaceStateGroup):
    """This group represents the data of a single universe"""


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
