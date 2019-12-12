#define BOOST_TEST_MODULE graph entity construction test

#include <boost/test/included/unit_test.hpp>

#include <utopia/core/state.hh>
#include <utopia/core/graph/entity.hh>

namespace Utopia
{

// ++ Types +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

struct VertexState {
    unsigned v_prop = 0;
};

struct EdgeState {
    unsigned e_prop = 0;
};


/// The traits of a vertex are just the traits of a graph entity
using VertexTraits = Utopia::GraphEntityTraits<VertexState>;

/// The traits of an edge are just the traits of a graph entity
using EdgeTraits = Utopia::GraphEntityTraits<EdgeState>;

/// A vertex is a graph entity with vertex traits
using Vertex = GraphEntity<VertexTraits>;

/// An edge is a graph entity with edge traits
using Edge = GraphEntity<EdgeTraits>;


// ++ Tests +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// -- Entities ----------------------------------------------------------------
BOOST_AUTO_TEST_CASE(test_graph_entity_construction){
    // test using the Vertex which is a graph entity
    auto v0 = Vertex(VertexState{1});
    auto v1 = Vertex(v0);
    auto v2 = Vertex();
    auto v3 = v0;

    // Test that the internal counter increased and is the same for all vertices
    BOOST_TEST(v3.get_id_counter() == 4);
    BOOST_TEST(v2.get_id_counter() == 4);
    BOOST_TEST(v1.get_id_counter() == 4);
    BOOST_TEST(v0.get_id_counter() == 4);

    // Test that the id's are correctly
    BOOST_TEST(v0.id() == 0);
    BOOST_TEST(v1.id() == 1);
    BOOST_TEST(v2.id() == 2);
    BOOST_TEST(v3.id() == 3);
}

}   // namespace Utopia
