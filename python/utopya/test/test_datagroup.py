"""Test the datagroup module"""

from pkg_resources import resource_filename

import pytest

import utopya
from utopya.datagroup import NetworkGroup, UniverseGroup
from utopya.testtools import ModelTest

# Local constants
NWG_CFG_PATH = resource_filename('test', 'cfg/nwg_cfg.yml')

# -----------------------------------------------------------------------------

@pytest.mark.skip(reason="No model to test this with is currently available")
def test_networkgroup():
    """Test the dantro.NetworkGroup integration into utopya"""
    mt = ModelTest("Hierarnet", test_file=__file__)
    mv, dm = mt.create_run_load(from_cfg=NWG_CFG_PATH)

    for uni in dm['multiverse'].values():
        nwg = uni['data/Hierarnet/nw']
        cfg = uni['cfg']

        # Check that it was loaded correctly. This requires that, on C++ side,
        # the correct attributes were set and the model was built.
        assert isinstance(nwg, NetworkGroup)

        # Test that the graph can be created as desired
        g = nwg.create_graph()

        # Check that the number of vertices matches
        assert g.number_of_nodes() == cfg['Hierarnet']['create_network']['num_vertices']

        # TODO Extend tests to test that dantro interface works as desired. The
        #      full functionality is tested on dantro side, but we would need a
        #      model that uses the save_graph_properties functionality such
        #      that it could be tested here...
