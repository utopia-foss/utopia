"""Test the datagroup module"""

from pkg_resources import resource_filename

import pytest
import networkx as nx

import utopya
from utopya.datagroup import GraphGroup, UniverseGroup
from utopya.testtools import ModelTest

# Local constants
GG_CFG_PATH = resource_filename('test', 'cfg/graphgroup_cfg.yml')

# -----------------------------------------------------------------------------

def test_graphgroup():
    """Test the dantro.GraphGroup integration into utopya. The full
    functionality is tested on dantro side."""
    mt = ModelTest("CopyMeGraph", test_file=__file__)
    mv, dm = mt.create_run_load(from_cfg=GG_CFG_PATH)

    for uni in dm['multiverse'].values():
        gg_static = uni['data/CopyMeGraph/g_static']
        gg_dynamic = uni['data/CopyMeGraph/g_dynamic']
        cfg = uni['cfg']

        for gg in [gg_static, gg_dynamic]:

            # Check that it was loaded correctly. This requires that, on C++
            # side, the correct attributes were set and the model was built.
            assert isinstance(gg, GraphGroup)
            assert gg._GG_node_container == '_vertices'
            assert gg._GG_edge_container == '_edges'
            assert gg._GG_attr_directed == 'is_directed'
            assert gg._GG_attr_parallel == 'allows_parallel'
            assert (gg._GG_attr_edge_container_is_transposed
                    == 'edge_container_is_transposed')
            assert gg._GG_WARN_UPON_BAD_ALIGN == True

            # Test that the graph can be created and properties can be added.
            g = gg.create_graph(at_time=0)

            if gg == gg_dynamic:
                g = gg.create_graph(at_time_idx=-1,
                                    node_props=['some_state', 'some_trait'],
                                    edge_props=['weights'])
            else:
                g = gg.create_graph(at_time_idx=-1,
                                    node_props=['some_state', 'some_trait'])

            # Check that the number of vertices matches
            assert (g.number_of_nodes()
                    == cfg['CopyMeGraph']['create_graph']['num_vertices'])

            # Get the properties from the graph
            some_state = nx.get_node_attributes(g, 'some_state')
            some_trait = nx.get_node_attributes(g, 'some_trait')

            # Check that the node properties were correctly added
            for n in g.nodes():
                assert (some_state[n]
                        == gg['some_state'].isel(time=-1).sel(vertex_idx=n))
                assert (some_trait[n]
                        == gg['some_trait'].isel(time=-1).sel(vertex_idx=n))

            # Check that edge properties were added for the dynamic graph
            if gg == gg_dynamic:
                assert nx.get_edge_attributes(g, 'weights')