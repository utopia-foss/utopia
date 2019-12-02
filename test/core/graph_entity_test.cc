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

    // Test that the internal counter increased and that the id's are correctly
    // set
    BOOST_TEST(Vertex::id_counter == 3);
    BOOST_TEST(v0.id() == 0);
    BOOST_TEST(v1.id() == 1);
    BOOST_TEST(v2.id() == 2);
}

}   // namespace Utopia
