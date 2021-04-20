#define BOOST_TEST_MODULE test graph utilities doc examples

#include <boost/test/unit_test.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/random.hpp>

#include <utopia/core/state.hh>
#include <utopia/core/graph/entity.hh>
#include <utopia/data_io/graph_utils.hh>
#include <utopia/data_io/hdfgroup.hh>
#include <utopia/data_io/hdffile.hh>


// -- Types -------------------------------------------------------------------

/// The vertex state 
struct VertexState {
    /// A vertex property
    size_t some_prop;
};

/// The traits of a vertex are just the traits of a graph entity
using VertexTraits = Utopia::GraphEntityTraits<VertexState>;
/// A vertex is a graph entity with vertex traits
using Vertex = Utopia::GraphEntity<VertexTraits>;

/// The graph type
using Graph = boost::adjacency_list<boost::vecS,         // edge container
                                    boost::vecS,         // vertex container
                                    boost::undirectedS,
                                    Vertex>;

// -- Fixtures ----------------------------------------------------------------

struct GraphFixture {
    // Create a random number generator
    Utopia::DefaultRNG rng;

    const size_t num_vertices;
    const size_t num_edges;
    const size_t v_prop_default;

    Graph g;

    GraphFixture() :
        num_vertices{10},
        num_edges{20},
        v_prop_default{1},
        g{}
    {
        // Create a test graph
        for (size_t v = 0; v < num_vertices; ++v){
            boost::add_vertex(Vertex(VertexState{v_prop_default}), g);
        }

        for (size_t e = 0; e < num_edges; ++e){
            boost::add_edge(
                boost::random_vertex(g, rng), boost::random_vertex(g, rng), g
            );
        }
    };
};

// -- Test cases --------------------------------------------------------------

/// Test the save_graph function
BOOST_FIXTURE_TEST_CASE(test_save_graph_doc_example, GraphFixture)
{
    // Create a test HDFFile and a HDFGroup
    auto hdf = Utopia::DataIO::HDFFile("graph_testfile.h5", "w");
    auto grp = hdf.open_group("testgroup");

    // DOC REFERENCE START: save_graph example
    using Utopia::DataIO::create_graph_group;
    using Utopia::DataIO::save_graph;

    // Open a graph group and save the graph
    const auto ggrp = create_graph_group(g, grp, "graph_grp");
    save_graph(g, ggrp);

    // This will result in the following data structure
    // (assuming the graph has 10 vertices and 20 edges):
    // 
    //  └┬ graph_grp
    //     └┬ _vertices     < ... shape(10,)
    //      └ _edges        < ... shape(2, 20)
    // DOC REFERENCE END: save_graph example

    // Remove the graph testfile
    std::remove("graph_testfile.h5");
}

/// Test the dynamic property data saving
BOOST_FIXTURE_TEST_CASE(test_save_properties_doc_example, GraphFixture)
{
    // Create a test HDFFile and a HDFGroup
    auto hdf = Utopia::DataIO::HDFFile("graph_testfile.h5", "w");
    auto grp = hdf.open_group("testgroup");

    // DOC REFERENCE START: setup_adaptor_tuples example
    const auto vertex_adaptors = std::make_tuple(
        // 1D case: Pass a (name, adaptor) tuple
        std::make_tuple("_vertices",
            [](auto vd, auto& g){
                return boost::get(boost::vertex_index_t(), g, vd);
            }
        ),
        std::make_tuple("some_prop",
            [](auto vd, auto& g){ return g[vd].state.some_prop; }
        )
    );

    const auto edge_adaptors = std::make_tuple(
        // 2D case: Pass a (name, dim0_name,
        //                      (coord1, adaptor1),
        //                      (coord2, adaptor2), ...) tuple
        std::make_tuple("_edges", "label",
            std::make_tuple("source",
                [](auto ed, auto& g){
                    return boost::get(
                        boost::vertex_index_t(), g, boost::source(ed, g)
                    );
                }
            ),
            std::make_tuple("target",
                [](auto ed, auto& g){
                    return boost::get(
                        boost::vertex_index_t(), g, boost::target(ed, g)
                    );
                }
            )
        )
    );
    // DOC REFERENCE END: setup_adaptor_tuples example

    // DOC REFERENCE START: save_properties example
    using Utopia::DataIO::create_graph_group;
    using Utopia::DataIO::save_vertex_properties;
    using Utopia::DataIO::save_edge_properties;

    const auto ggrp = create_graph_group(g, grp, "graph_grp");

    // Common usage within a Utopia model: Replace "0" by
    // std::to_string(this->get_time()) to set the current time as dset name.
    save_vertex_properties(g, ggrp, "0", vertex_adaptors);
    save_edge_properties(g, ggrp, "0", edge_adaptors);

    // ... the first model update step, assuming one vertex and one edge
    //     are added to the graph ...

    // Write the data for the next time step
    save_vertex_properties(g, ggrp, "1", vertex_adaptors);
    save_edge_properties(g, ggrp, "1", edge_adaptors);

    // This would result in the following data structure
    // (assuming the graph initially has 10 vertices and 20 edges):
    // 
    //  └┬ graph_grp
    //     └┬ _vertices
    //         ┬─ 0         < ... shape(10,)
    //         └─ 1         < ... shape(11,)
    //      ├ some_prop
    //         ┬─ 0         < ... shape(10,)
    //         └─ 1         < ... shape(11,)
    //      ├ _edges
    //         ┬─ 0         < ... shape(2, 20)
    //         └─ 1         < ... shape(2, 21)
    // DOC REFERENCE END: save_properties example

    // Remove the graph testfile
    std::remove("graph_testfile.h5");
}
