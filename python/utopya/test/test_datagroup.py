"""Test the datagroup module"""

from pkg_resources import resource_filename

import pytest
import networkx as nx

import utopya
from utopya.datagroup import NetworkGroup
from utopya.testtools import ModelTest

# Local constants
NWG_CFG_PATH = resource_filename('test', 'cfg/nwg_cfg.yml')

# ------------------------------------------------------------------------------

def test_networkgroup():
    """Test the dantro.NetworkGroup integration into utopya"""
    mt = ModelTest("Hierarnet", test_file=__file__)
    mv, dm = mt.create_run_load(from_cfg=NWG_CFG_PATH)

    for uni in dm['multiverse'].values():
        nwg = uni['data/Hierarnet/nw']
        cfg = uni['cfg']

        # Check that it was loaded correctly
        assert(isinstance(nwg, NetworkGroup))

        # Test that the graph can be created as desired
        nwg.create_graph()

        # ... also when including node attributes
        g = nwg.create_graph(with_node_attributes=True)

        # Check that the number of vertices matches
        assert g.number_of_nodes() == cfg['Hierarnet']['num_vertices']

        # Check that the node attributes are available    
        assert nx.get_node_attributes(g, 'payoff')
        assert nx.get_node_attributes(g, 'cost')
        assert not nx.get_node_attributes(g, 'some_other_attribute')
