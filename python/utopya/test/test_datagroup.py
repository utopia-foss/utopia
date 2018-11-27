"""Test the datagroup module"""

from pkg_resources import resource_filename

import pytest

import utopya
from utopya.datagroup import NetworkGroup
from utopya.testtools import ModelTest

# Local constants
NWG_CFG_PATH = resource_filename('test', 'cfg/nwg_cfg.yml')

# ------------------------------------------------------------------------------

def test_networkgroup():
    """test the NetworkGroup"""
    mt = ModelTest("Hierarnet", test_file=__file__)

    mv, dm = mt.create_run_load(from_cfg=NWG_CFG_PATH)

    for uni in dm['multiverse'].values():
        nwg = uni['data/Hierarnet/nw']

        assert(isinstance(nwg, NetworkGroup))
