"""Implements data group classes specific to the Utopia output data structure.

They are based on dantro BaseDataGroup-derived implementations. In this module,
they are imported and configured to suit the needs of the Utopia data structes.
"""

import logging

import dantro as dtr
import dantro.groups

from .datacontainer import XarrayDC
from .dataprocessing import transform

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

    Furthermore, via dantro, an easy data selector method is available, see
    `dantro.groups.ParamSpaceGroup.select`.
    """
    _NEW_GROUP_CLS = UniverseGroup

    # Make the transformation interface available to the select method
    _PSPGRP_TRANSFORMATOR = lambda _, d, *ops, **kws: transform(d, *ops, **kws)
    # NOTE first argument (self) not needed here


class TimeSeriesGroup(dtr.groups.TimeSeriesGroup):
    """This group is meant to manage time series data, with the container names
    being interpreted as the time coordinate.
    """
    _NEW_CONTAINER_CLS = XarrayDC


class HeterogeneousTimeSeriesGroup(dtr.groups.HeterogeneousTimeSeriesGroup):
    """This group is meant to manage time series data, with the container names
    being interpreted as the time coordinate.
    """
    _NEW_CONTAINER_CLS = XarrayDC


class GraphGroup(dtr.groups.GraphGroup):
    """This group is meant to manage network data and create a NetworkX graph
    from it.
    """
    # Let new groups contain only time series
    _NEW_GROUP_CLS = TimeSeriesGroup

    # Define allowed member container types
    _ALLOWED_CONT_TYPES = (TimeSeriesGroup, XarrayDC)

    # Expected names for the containers that hold vertex/edge information
    _NWG_node_container = "_vertices"
    _NWG_edge_container = "_edges"

    # Expected _group_ attribute names determining the type of graph
    _NWG_attr_directed = "is_directed"
    _NWG_attr_parallel = "allows_parallel"
    _NWG_attr_node_prop = "is_vertex_property"
    _NWG_attr_edge_prop = "is_edge_property"
