#define BOOST_TEST_MODULE apply rule on graphs test

#include <boost/test/included/unit_test.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/random.hpp>
#include <boost/graph/copy.hpp>

#include <utopia/core/graph/apply.hh>
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

/// The test graph types
using G_undir_vec = boost::adjacency_list<
                    boost::vecS,         // edge container
                    boost::vecS,         // vertex container
                    boost::undirectedS,
                    Vertex,
                    Edge>;

using G_dir_vec = boost::adjacency_list<
                    boost::vecS,         // edge container
                    boost::vecS,         // vertex container
                    boost::bidirectionalS,
                    Vertex,
                    Edge>;

using G_undir_list = boost::adjacency_list<
                    boost::listS,        // edge container
                    boost::listS,        // vertex container
                    boost::undirectedS,
                    Vertex,
                    Edge>;

using G_dir_list = boost::adjacency_list<
                    boost::listS,        // edge container
                    boost::listS,        // vertex container
                    boost::bidirectionalS,
                    Vertex,
                    Edge>;

// ++ Fixtures ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
template<class G>
struct GraphFixture {
    using VertexDesc = typename boost::graph_traits<G>::vertex_descriptor;

    // Create a random number generator
    Utopia::DefaultRNG rng, rng_ref;

    const unsigned num_vertices;
    const unsigned num_edges;
    const unsigned v_prop_default;
    const unsigned e_prop_default;

    G g, g_ref;

    GraphFixture() :
        num_vertices{10},
        num_edges{20},
        v_prop_default{1},
        e_prop_default{2},
        g{},
        g_ref{}
    {
        // Creatig a test graph
        for (auto v = 0u; v<10; ++v){
            boost::add_vertex(Vertex(VertexState{v_prop_default}), this->g);
        }

        for (auto e = 0u; e < num_edges; ++e){
            boost::add_edge(boost::random_vertex(g, rng), 
                            boost::random_vertex(g, rng), 
                            Edge(EdgeState{e_prop_default}), g);
        }

        // Creating an equal reference graph     
        // NOTE: Using boost::copy_graph() does not compile.
        for (auto v = 0u; v<10; ++v){
            boost::add_vertex(Vertex(VertexState{10}), this->g_ref);
        }

        for (auto e = 0u; e < num_edges; ++e){
            boost::add_edge(boost::random_vertex(g_ref, rng_ref), 
                            boost::random_vertex(g_ref, rng_ref), 
                            Edge(EdgeState{20}), g_ref);
        }
    };
};

using GraphFixtures = boost::mpl::vector<
    GraphFixture<G_dir_vec>,
    GraphFixture<G_dir_list>,
    GraphFixture<G_undir_list>,
    GraphFixture<G_undir_vec>
>;


// ++ Tests +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_manual_rule_noshuffle_async, G, 
    GraphFixtures, G)
{
    {
        // -- Test iteration over vertices ----------------------------------------

        // Set the vertex property to a counter value that increments with each
        // assignment.
        auto counter = 0u;
        apply_rule<IterateOver::vertices, Update::async, Shuffle::off>(
            [&counter](auto v, auto& g){
                auto& state = g[v].state;
                state.v_prop = counter;
                ++counter;
                return state;
            },
            G::g
        );
        
        // Set the properties manually in the reference graph
        counter = 0;
        for (auto [it, it_end] = boost::vertices(G::g_ref); it!=it_end; ++it){
            G::g_ref[*it].state.v_prop = counter;
            ++counter;
        }

        // Test that manually applying the rule leads to the same result
        // as with the apply_rule interface
        for (auto i = 0u; i < boost::num_vertices(G::g); ++i){
            BOOST_TEST(G::g[boost::vertex(i, G::g)].state.v_prop 
                    == G::g_ref[boost::vertex(i, G::g_ref)].state.v_prop);
            BOOST_TEST(G::g[boost::vertex(i, G::g)].state.v_prop == i);
        }
    } 

    // -- Test iteration over neighbors ---------------------------------------
    {
        const auto parent_vertex = boost::vertex(0, G::g);

        // Set the vertex property to a counter value that increments with each
        // assignment.
        auto counter = 0u;
        apply_rule<IterateOver::neighbors, Update::async, Shuffle::off>(
            [&counter](auto v, auto& g){
                auto& state = g[v].state;
                state.v_prop = counter;
                ++counter;
                return state;
            },
            parent_vertex,
            G::g
        );
        
        // Set the properties manually in the reference graph
        counter = 0;
        for (auto [it, it_end] = boost::adjacent_vertices
                                    (parent_vertex, G::g_ref); 
                it!=it_end; ++it){
            G::g_ref[*it].state.v_prop = counter;
            ++counter;
        }

        // Test that manually applying the rule leads to the same result
        // as with the apply_rule interface
        for (auto i = 0u; i < boost::out_degree(parent_vertex, G::g); ++i){
            BOOST_TEST(G::g[boost::vertex(i, G::g)].state.v_prop 
                    == G::g_ref[boost::vertex(i, G::g_ref)].state.v_prop);
            BOOST_TEST(G::g[boost::vertex(i, G::g)].state.v_prop == i);
        }

    // -- Test iteration over other graph entities ----------------------------
    // NOTE that the test whether the correct graph entity is selected to 
    //      iterate over is covered in the `graph_iterator_test.cc`
    //      Therefore, here it is sufficient to test for the two cases from 
    //      above because they have distinct apply_rule signatures.
    }
}


BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_manual_rule_noshuffle_sync, G, 
    GraphFixtures, G)
{
    // Set the vertex property to a counter value that increments with each
    // assignment. Also add the neighbors property which should be constant 
    // if the states are updated synchronously.
    auto counter = 0u;
    apply_rule<IterateOver::vertices, Update::sync, Shuffle::off>(
        [&counter](auto v, auto& g){
            auto state = g[v].state;
            state.v_prop = counter;

            // Add all neighbors v_prop
            for (auto nb : range<IterateOver::neighbors>(v, g)){
                state.v_prop += g[nb].state.v_prop;
            }
            ++counter;
            return state;
        },
        G::g
    );
    
    // Set the properties manually in the reference graph
    // The property consists of a counter variable and the number of neighbors
    // times the default vertex property
    counter = 0;
    for (auto [it, it_end] = boost::vertices(G::g_ref); it!=it_end; ++it){        
        const unsigned expected_v_prop 
            = counter + (boost::out_degree(*it, G::g_ref) * G::v_prop_default);

        G::g_ref[*it].state.v_prop = expected_v_prop;

        ++counter;
    }

    // Test that manually applying the rule leads to the same result
    // as with the apply_rule interface
    // If the rule was applied synchronously all the neighbor's vertex 
    // properties should still have the same constant value.
    for (auto i = 0u; i < boost::num_vertices(G::g); ++i){
        BOOST_TEST(G::g[boost::vertex(i, G::g)].state.v_prop 
                == G::g_ref[boost::vertex(i, G::g_ref)].state.v_prop);
    }
}


BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_manual_rule_shuffle_async, G, 
    GraphFixtures, G)
{
    {
        // -- Test iteration over vertices ----------------------------------------

        // Set the vertex property to a counter value that increments with each
        // assignment.
        auto counter = 0u;
        apply_rule<IterateOver::vertices, Update::async, Shuffle::on>(
            [&counter](auto v, auto& g){
                auto& state = g[v].state;
                state.v_prop = counter;
                ++counter;
                return state;
            },
            G::g,
            G::rng
        );
        
        // Set the properties manually in the reference graph
        auto [it, it_end] = boost::vertices(G::g_ref);
        std::vector<typename G::VertexDesc> it_shuffled(it, it_end);
        std::shuffle(std::begin(it_shuffled), std::end(it_shuffled), G::rng_ref);

        counter = 0;
        for (auto v : it_shuffled){
            G::g_ref[v].state.v_prop = counter;
            ++counter;
        }

        // Test that manually applying the rule leads to the same result
        // as with the apply_rule interface
        for (auto i = 0u; i < boost::num_vertices(G::g); ++i){
            BOOST_TEST(G::g[boost::vertex(i, G::g)].state.v_prop 
                    == G::g_ref[boost::vertex(i, G::g_ref)].state.v_prop);
        }
    } 

    // -- Test iteration over neighbors ---------------------------------------
    {
        // NOTE that here it is only checked whether the function is called. 
        const auto parent_vertex = boost::vertex(0, G::g);

        // Set the vertex property to a counter value that increments with each
        // assignment.
        auto counter = 0u;
        apply_rule<IterateOver::neighbors, Update::async, Shuffle::on>(
            [&counter](auto v, auto& g){
                auto& state = g[v].state;
                state.v_prop = counter;
                ++counter;
                return state;
            },
            parent_vertex,
            G::g,
            G::rng
        );
        
    // -- Test iteration over other graph entities ----------------------------
    // NOTE that the test whether the correct graph entity is selected to 
    //      iterate over is covered in the `graph_iterator_test.cc`
    //      Therefore, here it is sufficient to test for the two cases from 
    //      above because they have distinct apply_rule signatures.
    }
}

BOOST_FIXTURE_TEST_CASE_TEMPLATE(test_manual_rule_shuffle_sync, G, 
    GraphFixtures, G)
{ 
    // Set the vertex property to a counter value that increments with each
    // assignment. Also add the neighbors property which should be constant 
    // if the states are updated synchronously.
    auto counter = 0u;
    apply_rule<IterateOver::vertices, Update::sync, Shuffle::on>(
        [&counter](auto v, auto& g){
            auto state = 
            g[v].state;
            state.v_prop = counter;

            // Add all neighbors v_prop
            for (auto nb : range<IterateOver::neighbors>(v, g)){
                state.v_prop += g[nb].state.v_prop;
            }
            ++counter;
            return state;
        },
        G::g,
        G::rng
    );
    
    // Set the properties manually in the reference graph
    // The property consists of a counter variable and the number of neighbors
    // times the default vertex property

    // Set the properties manually in the reference graph
    auto [it, it_end] = boost::vertices(G::g_ref);
    std::vector<typename G::VertexDesc> it_shuffled(it, it_end);
    std::shuffle(std::begin(it_shuffled), std::end(it_shuffled), G::rng_ref);
    counter = 0;
    for (auto v : it_shuffled){        
        const unsigned expected_v_prop 
            = counter + (boost::out_degree(v, G::g_ref) * G::v_prop_default);

        G::g_ref[v].state.v_prop = expected_v_prop;

        ++counter;
    }

    // Test that manually applying the rule leads to the same result
    // as with the apply_rule interface
    // If the rule was applied synchronously all the neighbor's vertex 
    // properties should still have the same constant value.
    for (auto i = 0u; i < boost::num_vertices(G::g); ++i){
        BOOST_TEST(G::g[boost::vertex(i, G::g)].state.v_prop 
                == G::g_ref[boost::vertex(i, G::g_ref)].state.v_prop);
    }
}


}   // namespace Utopia
