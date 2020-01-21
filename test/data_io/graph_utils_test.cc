#define BOOST_TEST_MODULE test graph utilities

#include <boost/test/unit_test.hpp>
#include <boost/mpl/vector.hpp>

#include <utopia/data_io/graph_utils.hh>
#include <utopia/data_io/hdfgroup.hh>
#include <utopia/data_io/hdffile.hh>

#include "dataio_test.hh"

using namespace Utopia;
using namespace Utopia::DataIO;

// -- Fixtures ----------------------------------------------------------------

template <typename Graph>
struct SmallGraphFixture{
    using Type = Graph;
    Graph g = create_and_initialize_test_graph<Graph>(10,3);

    SmallGraphFixture() {
        setup_loggers();
    }
};

using SmallGraphsVecSFixtures = boost::mpl::vector<
    SmallGraphFixture<Graph_vertvecS_edgevecS_undir>,
    SmallGraphFixture<Graph_vertvecS_edgevecS_dir>
>;

using SmallGraphsSetSListSFixtures = boost::mpl::vector<
    SmallGraphFixture<Graph_vertlistS_edgelistS_undir>,
    SmallGraphFixture<Graph_vertsetS_edgesetS_undir>
>;

template <typename Graph>
struct LargeGraphFixture{
    using Type = Graph;
    Graph g = create_and_initialize_test_graph<Graph>(100,30);

    LargeGraphFixture() {
        setup_loggers();
    }
};

using LargeGraphsFixtures = boost::mpl::vector<
    LargeGraphFixture<Graph_vertvecS_edgevecS_undir>,
    LargeGraphFixture<Graph_vertvecS_edgevecS_dir>,
    LargeGraphFixture<Graph_vertlistS_edgelistS_undir>,
    LargeGraphFixture<Graph_vertsetS_edgesetS_undir>
>;

// -- Test cases --------------------------------------------------------------

/// Test the save_graph function with internal id's.
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_save_graph_vecS, G,
                                SmallGraphsVecSFixtures, G)
{
    using Utopia::DataIO::create_graph_group;
    using Utopia::DataIO::save_graph;
    // Create a test HDFFile and a HDFGroup
    auto hdf = Utopia::DataIO::HDFFile("graph_testfile.h5","w");
    auto grp = hdf.open_group("testgroup");

    // Open a graph group and save the graph
    auto ggrp = create_graph_group(G::g, grp, "testgraph");
    save_graph(G::g, ggrp);

    // Remove the graph testsfile
    std::remove("graph_testfile.h5");
}

/// Test the save_graph function with custom id's.
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_save_graph_listS_setS, G,
                                SmallGraphsSetSListSFixtures, G)
{
    using Utopia::DataIO::create_graph_group;
    using Utopia::DataIO::save_graph;

    // Create a test HDFFile and a HDFGroup
    auto hdf = Utopia::DataIO::HDFFile("graph_testfile.h5","w");
    auto grp = hdf.open_group("testgroup");
    
    // In the case of boost::listS and boost::setS, the graph does not store a
    // boost::vertex_index internally. Therefore, the save_graph function would
    // not work. To overcome this, the user needs to define an id in the Vertex
    // struct itself. It can be given to the save_data function through a
    // property map, which needs to be defined.
    // NOTE: The user needs to adapt this id, such that it actually represents
    //       a unique id for every agent within the model dynamics!
    using VertexIdMap = typename boost::property_map<typename G::Type,
                                                std::size_t Vertex::*>::type;
    VertexIdMap vertex_id_map = get(&Vertex::id, G::g);

    // Open a graph group and save the graph
    auto ggrp = create_graph_group(G::g, grp, "testgraph");
    save_graph(G::g, ggrp, vertex_id_map);

    // Don't remove the file for the last fixture as this will be used for
    // checking the attributes
    if (not std::is_same_v<typename G::Type, Graph_vertsetS_edgesetS_undir>) {
        std::remove("graph_testfile.h5");
    }
}

/// Test that the attributes are written correctly in the save_graph function
BOOST_AUTO_TEST_CASE(test_attribute_writing_save_graph) {
    // NOTE This test is only done for the last fixture of the test above.

    // Read the HDF5 file
    auto hdf = Utopia::DataIO::HDFFile("graph_testfile.h5","r");
    auto grp = hdf.open_group("testgroup");
    auto ggrp = grp->open_group("testgraph");
    auto dset_vertices = ggrp->open_dataset("_vertices");
    auto dset_edges = ggrp->open_dataset("_edges");

    // Test that the group attributes are set correctly
    HDFAttribute ggrp_attr(*ggrp, "content");
    auto attr_content = std::get<1>(ggrp_attr.read<std::string>());
    BOOST_TEST(attr_content == "network");
    ggrp_attr.close();

    ggrp_attr.open(*ggrp, "is_directed");
    auto attr_is_dir = std::get<1>(ggrp_attr.read<int>());
    BOOST_TEST(attr_is_dir == 0);
    ggrp_attr.close();

    ggrp_attr.open(*ggrp, "allows_parallel");
    auto attr_parallel = std::get<1>(ggrp_attr.read<int>());
    BOOST_TEST(attr_parallel == 0);
    ggrp_attr.close();

    ggrp_attr.open(*ggrp, "num_vertices");
    auto attr_num_vertices = std::get<1>(ggrp_attr.read<size_t>());
    BOOST_TEST(attr_num_vertices == 10);
    ggrp_attr.close();

    ggrp_attr.open(*ggrp, "num_edges");
    auto attr_num_edges = std::get<1>(ggrp_attr.read<size_t>());
    BOOST_TEST(attr_num_edges == 3);
    ggrp_attr.close();

    // Test that the edge dataset attributes are set correctly

    HDFAttribute dset_edges_attr(*dset_edges, "dim_name__0");
    auto attr_dim0_name = std::get<1>(dset_edges_attr.read<std::string>());
    BOOST_TEST(attr_dim0_name == "label");
    dset_edges_attr.close();

    dset_edges_attr.open(*dset_edges, "coords_mode__label");
    auto attr_cmode_label = std::get<1>(dset_edges_attr.read<std::string>());
    BOOST_TEST(attr_cmode_label == "values");
    dset_edges_attr.close();

    dset_edges_attr.open(*dset_edges, "coords__label");
    auto attr_c_label = std::get<1>(dset_edges_attr.read<
                                                std::vector<std::string>>());
    std::vector<std::string> c_label{"source", "target"};
    BOOST_TEST(attr_c_label == c_label);
    dset_edges_attr.close();

    dset_edges_attr.open(*dset_edges, "dim_name__1");
    auto attr_dim1_name = std::get<1>(dset_edges_attr.read<std::string>());
    BOOST_TEST(attr_dim1_name == "edge_idx");
    dset_edges_attr.close();

    dset_edges_attr.open(*dset_edges, "coords_mode__edge_idx");
    auto attr_cmode_eidx = std::get<1>(dset_edges_attr.read<std::string>());
    BOOST_TEST(attr_cmode_eidx == "trivial");
    dset_edges_attr.close();

    // Test that the vertex dataset attributes are set correctly

    HDFAttribute dset_vertices_attr(*dset_vertices, "dim_name__0");
    auto v_attr_dim0_name = std::get<1>(dset_vertices_attr.read<std::string>());
    BOOST_TEST(v_attr_dim0_name == "vertex_idx");
    dset_vertices_attr.close();

    dset_vertices_attr.open(*dset_vertices, "coords_mode__vertex_idx");
    auto v_attr_cmode_label = std::get<1>(dset_vertices_attr.read<std::string>());
    BOOST_TEST(v_attr_cmode_label == "trivial");
    dset_vertices_attr.close();

    // Remove the graph testsfile
    std::remove("graph_testfile.h5");
}

/// Test the save_graph_entity_properties functionality for 1d data using
/// vertex descriptors.
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_save_properties_vertices_1d, G,
                                LargeGraphsFixtures, G)
{
    using Utopia::DataIO::save_graph_entity_properties;

    // Create a test HDFFile and a HDFGroup
    auto hdf = Utopia::DataIO::HDFFile("graph_testfile.h5","w");
    auto grp = hdf.open_group("testgroup");

    const auto vertex_desc_vals = std::make_tuple(
            std::make_tuple("id",
                    [](auto& vd, auto& g){return g[vd].id;}),
            std::make_tuple("test_double",
                    [](auto& vd, auto& g){return g[vd].test_double;}),
            std::make_tuple("test_value",
                    [](auto& vd, auto& g){return g[vd].get_test_value();}),
            std::make_tuple("test_int",
                    [](auto& vd, auto& g){return g[vd].test_int;}),
            std::make_tuple("id2",
                    [](auto& vd, auto& g){return g[vd].id;})
    );

    save_vertex_properties(G::g, grp, "dset", vertex_desc_vals);

    // Get the datasets
    auto dset_id = grp->open_group("id")->open_dataset("dset");
    auto dset_id2 = grp->open_group("id2")->open_dataset("dset");

    // Read the data
    auto data_id = dset_id->template read<std::vector<std::size_t>>();
    auto data_id2 = dset_id2->template read<std::vector<std::size_t>>();

    // Assert the vertex data consistency
    BOOST_TEST(std::get<1>(data_id) == std::get<1>(data_id2));

    if (not std::is_same_v<typename G::Type, Graph_vertsetS_edgesetS_undir>) {
        // Do not test data for setS as the iterator range might differ

        std::vector<std::size_t> g_ids;
        std::vector<int> g_ints;
        std::vector<long long unsigned int> shape = {100};

        // get original data with the same iterator range
        for (const auto v : range<IterateOver::vertices>(G::g)) {
            g_ids.push_back(G::g[v].id);
            g_ints.push_back(G::g[v].test_int);
        }

        // Get additional test data
        auto dset_int = grp->open_group("test_int")->open_dataset("dset");
        auto data_int = dset_int->template read<std::vector<int>>();

        // Check for matching data
        BOOST_TEST(g_ids == std::get<1>(data_id));
        BOOST_TEST(g_ints == std::get<1>(data_int));
        BOOST_TEST(shape == std::get<0>(data_int));
    }

    // Remove the graph testsfile
    std::remove("graph_testfile.h5");
}

/// Test the save_graph_entity_properties functionality for 2d data using
/// vertex descriptors.
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_save_properties_vertices_2d, G,
                                LargeGraphsFixtures, G)
{
    using Utopia::DataIO::save_graph_entity_properties;

    // Create a test HDFFile and a HDFGroup
    auto hdf = Utopia::DataIO::HDFFile("graph_testfile.h5","w");
    auto grp = hdf.open_group("testgroup");

    const auto vertex_desc_vals = std::make_tuple(
            std::make_tuple("id_test_int", "dim0_name",
                std::make_tuple("id",
                        [](auto& vd, auto& g){return g[vd].id;}),
                std::make_tuple("test_int",
                        [](auto& vd, auto& g){return g[vd].test_int;})
            )
    );

    save_vertex_properties(G::g, grp, "dset", vertex_desc_vals);

    if (not std::is_same_v<typename G::Type, Graph_vertsetS_edgesetS_undir>) {
        // Do not test data for setS as the iterator range might differ
        
        std::vector<std::size_t> g_id;
        std::vector<std::size_t> g_test_int;

        // Get the original data with the same iterator range
        for (const auto v : range<IterateOver::vertices>(G::g)) {
            g_id.push_back(G::g[v].id);
            g_test_int.push_back(G::g[v].test_int);
        }

        // Get the dataset
        auto dset_id_test_int = grp->open_group("id_test_int")
                                ->open_dataset("dset");
        
        // Read the data
        auto data_id_test_int = std::get<1>(dset_id_test_int
                                ->template read<std::vector<std::size_t>>());
        auto data_id = std::vector<std::size_t>(
                                            data_id_test_int.begin(),
                                            data_id_test_int.begin() + 100);
        auto data_test_int = std::vector<std::size_t>(
                                            data_id_test_int.begin() + 100,
                                            data_id_test_int.end());

        // Check for matching data
        BOOST_TEST(g_id == data_id);
        BOOST_TEST(g_test_int == data_test_int);
    }

    // Remove the graph testsfile
    std::remove("graph_testfile.h5");
}

/// Test the save_graph_entity_properties functionality for 1d data using
/// edge descriptors.
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_save_properties_edges_1d, G,
                                LargeGraphsFixtures, G)
{
    using Utopia::DataIO::save_graph_entity_properties;

    // Create a test HDFFile and a HDFGroup
    auto hdf = Utopia::DataIO::HDFFile("graph_testfile.h5","w");
    auto grp = hdf.open_group("testgroup");

    const auto edge_desc_vals = std::make_tuple(
            std::make_tuple("weights",
                    [](auto& ed, auto& g){return g[ed].weight;}),
            std::make_tuple("sources",
                    [](auto& ed, auto& g){return g[source(ed, g)].id;}),
            std::make_tuple("targets",
                    [](auto& ed, auto& g){return g[target(ed, g)].id;}),
            std::make_tuple("sources2",
                    [](auto& ed, auto& g){return g[source(ed, g)].id;}),
            std::make_tuple("targets2",
                    [](auto& ed, auto& g){return g[target(ed, g)].id;})
    );

    save_edge_properties(G::g, grp, "dset", edge_desc_vals);

    // Get the datasets
    auto dset_sources = grp->open_group("sources")->open_dataset("dset");
    auto dset_targets = grp->open_group("targets")->open_dataset("dset");
    auto dset_sources2 = grp->open_group("sources2")->open_dataset("dset");
    auto dset_targets2 = grp->open_group("targets2")->open_dataset("dset");

    // Read the data
    auto data_sources = std::get<1>(dset_sources
                                ->template read<std::vector<std::size_t>>());
    auto data_targets = std::get<1>(dset_targets
                                ->template read<std::vector<std::size_t>>());
    auto data_sources2 = std::get<1>(dset_sources2
                                ->template read<std::vector<std::size_t>>());
    auto data_targets2 = std::get<1>(dset_targets2
                                ->template read<std::vector<std::size_t>>());

    // Assert the edge data consistency
    BOOST_TEST( data_sources == data_sources2);
    BOOST_TEST( data_targets == data_targets2);

    if (not std::is_same_v<typename G::Type, Graph_vertsetS_edgesetS_undir>) {
        // Do not test data for setS as the iterator range might differ

        std::vector<float> g_weights;
        std::vector<std::size_t> g_sources;
        std::vector<std::size_t> g_targets;

        // Get the original data with the same iterator range
        for (const auto e : range<IterateOver::edges>(G::g)) {
            g_weights.push_back(G::g[e].weight);
            g_sources.push_back(G::g[source(e, G::g)].id);
            g_targets.push_back(G::g[target(e, G::g)].id);
        }

        // Get additional test data
        auto dset_weights = grp->open_group("weights")->open_dataset("dset");
        auto data_weights = std::get<1>(dset_weights
                                    ->template read<std::vector<float>>());

        // Check for matching data
        BOOST_TEST(g_weights == data_weights);
        BOOST_TEST(g_sources == data_sources);
        BOOST_TEST(g_targets == data_targets);
    }

    // Remove the graph testsfile
    std::remove("graph_testfile.h5");
}

/// Test the save_graph_entity_properties functionality for 2d data using
/// edge descriptors.
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_save_properties_edges_2d, G,
                                LargeGraphsFixtures, G)
{
    using Utopia::DataIO::save_graph_entity_properties;

    // Create a test HDFFile and a HDFGroup
    auto hdf = Utopia::DataIO::HDFFile("graph_testfile.h5","w");
    auto grp = hdf.open_group("testgroup");

    const auto edge_desc_vals = std::make_tuple(
            std::make_tuple("edges1", "label",
                std::make_tuple("source",
                    [](auto& ed, auto& g){return g[source(ed, g)].id;}),
                std::make_tuple("target",
                    [](auto& ed, auto& g){return g[target(ed, g)].id;})
            ),
            std::make_tuple("edges2", "label",
                std::make_tuple(123,
                    [](auto& ed, auto& g){ return g[source(ed, g)].id; }),
                std::make_tuple(456,
                    [](auto& ed, auto& g){ return g[target(ed, g)].id; })
            ),
            std::make_tuple("edges3", "label",
                std::make_tuple(1.23,
                    [](auto& ed, auto& g){ return g[source(ed, g)].id; }),
                std::make_tuple(4.56,
                    [](auto& ed, auto& g){ return g[target(ed, g)].id; })
            )
    );

    save_edge_properties(G::g, grp, "dset", edge_desc_vals);

    if (not std::is_same_v<typename G::Type, Graph_vertsetS_edgesetS_undir>) {
        // Do not test data for setS as the iterator range might differ
        
        std::vector<std::size_t> g_sources;
        std::vector<std::size_t> g_targets;

        // Get the original data with the same iterator range
        for (const auto e : range<IterateOver::edges>(G::g)) {
            g_sources.push_back(G::g[source(e, G::g)].id);
            g_targets.push_back(G::g[target(e, G::g)].id);
        }

        // Get the edge data
        auto dset_edges = grp->open_group("edges1")->open_dataset("dset");
        auto data_edges = std::get<1>(dset_edges
                                ->template read<std::vector<std::size_t>>());
        auto edge_sources = std::vector<std::size_t>(data_edges.begin(),
                                                     data_edges.begin() + 30);
        auto edge_targets = std::vector<std::size_t>(data_edges.begin() + 30,
                                                     data_edges.end());

        // Check for matching data
        BOOST_TEST(g_sources == edge_sources);
        BOOST_TEST(g_targets == edge_targets);
    }

    // Don't remove the file for the last fixture as this will be used for
    // checking the attributes
    if (not std::is_same_v<typename G::Type, Graph_vertsetS_edgesetS_undir>) {
        std::remove("graph_testfile.h5");
    }
}

/// Test that the attributes are written correctly in 'save_graph_entity_properties'
BOOST_AUTO_TEST_CASE(test_attribute_writing_save_graph_entity_properties) {
    // NOTE This test is only done for the last fixture of the test above.

    // Read the HDF5 file
    auto hdf = Utopia::DataIO::HDFFile("graph_testfile.h5","r");
    auto grp = hdf.open_group("testgroup");
    auto grp_edges = grp->open_group("edges1");
    auto dset_edges = grp_edges->open_dataset("dset");

    // Test that the group attribute is set correctly
    HDFAttribute grp_edges_attr(*grp_edges, "is_edge_property");
    auto attr_is_edge_prop = std::get<1>(grp_edges_attr.read<int>());
    BOOST_TEST(attr_is_edge_prop == 1);
    grp_edges_attr.close();

    // Test that the dataset attributes are set correctly

    HDFAttribute dset_edges_attr(*dset_edges, "dim_name__0");
    auto attr_dim0_name = std::get<1>(dset_edges_attr.read<std::string>());
    BOOST_TEST(attr_dim0_name == "label");
    dset_edges_attr.close();

    dset_edges_attr.open(*dset_edges, "coords_mode__label");
    auto attr_cmode_label = std::get<1>(dset_edges_attr.read<std::string>());
    BOOST_TEST(attr_cmode_label == "values");
    dset_edges_attr.close();

    dset_edges_attr.open(*dset_edges, "coords__label");
    auto attr_c_label = std::get<1>(dset_edges_attr.read<
                                                std::vector<std::string>>());
    std::vector<std::string> c_label{"source", "target"};
    BOOST_TEST(attr_c_label == c_label);
    dset_edges_attr.close();

    dset_edges_attr.open(*dset_edges, "dim_name__1");
    auto attr_dim1_name = std::get<1>(dset_edges_attr.read<std::string>());
    BOOST_TEST(attr_dim1_name == "edge_idx");
    dset_edges_attr.close();

    dset_edges_attr.open(*dset_edges, "coords_mode__edge_idx");
    auto attr_cmode_eidx = std::get<1>(dset_edges_attr.read<std::string>());
    BOOST_TEST(attr_cmode_eidx == "trivial");
    dset_edges_attr.close();

    // Test that integer coordinates are written correctly

    auto grp_int_coords = grp->open_group("edges2");
    auto dset_int_coords = grp_int_coords->open_dataset("dset");

    HDFAttribute grp_int_coords_attr(*dset_int_coords, "coords__label");
    auto attr_c_label_int = std::get<1>(grp_int_coords_attr.read<
                                                std::vector<int>>());
    std::vector<int> c_label_int{123, 456};
    BOOST_TEST(attr_c_label_int == c_label_int);
    grp_int_coords_attr.close();

    // Test that double coordinates are written correctly

    auto grp_double_coords = grp->open_group("edges3");
    auto dset_double_coords = grp_double_coords->open_dataset("dset");

    HDFAttribute grp_double_coords_attr(*dset_double_coords, "coords__label");
    auto attr_c_label_double = std::get<1>(grp_double_coords_attr.read<
                                                std::vector<double>>());
    std::vector<double> c_label_double{1.23, 4.56};
    BOOST_TEST(attr_c_label_double == c_label_double);
    grp_double_coords_attr.close();

    // Remove the graph testsfile
    std::remove("graph_testfile.h5");
}