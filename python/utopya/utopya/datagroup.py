"""Implements data group classes specific to the Utopia output data structure.

They are based on the dantro's OrderedDataGroup class.
"""

import logging

from dantro.base import BaseDataGroup
from dantro.group import ParamSpaceGroup, ParamSpaceStateGroup

# Configure and get logger
log = logging.getLogger(__name__)

# Local constants

# -----------------------------------------------------------------------------

class UniverseGroup(ParamSpaceStateGroup):
    """This group represents the data of a single universe"""


class MultiverseGroup(ParamSpaceGroup):
    """This group is meant to manage the `uni` group of the loaded data, i.e.
    the group where output of all universe groups is stored in.

    Its main aim is to provide easy access to universes. By default, universes
    are only identified by their ID, which is a zero-padded _string_. This
    group adds the ability to access them via integer indices.

    Furthermore, via dantro, an easy data selector is available
    """
    _NEW_GROUP_CLS = UniverseGroup
