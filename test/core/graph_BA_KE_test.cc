#define BOOST_TEST_MODULE graph Barabasi-Albert Klemm-Eguiluz test

#include <boost/test/included/unit_test.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <spdlog/spdlog.h>

#include <utopia/core/testtools.hh>
#include <utopia/core/types.hh>
#include <utopia/core/graph.hh>

using namespace Utopia::Graph;
using namespace Utopia::TestTools;


// -- Types -------------------------------------------------------------------

// Initialise the core logger
auto logger = Utopia::init_logger("core", spdlog::level::debug);

struct Infrastructure : public BaseInfrastructure<> {
    Infrastructure() : BaseInfrastructure<>("graph_BA_KE_test.yml") {};
};

struct Vertex {};
struct Edge {};

/// The test graph types
struct Test_Graph : Infrastructure {
  // undirected
  using G_vec_u = boost::adjacency_list<
                      boost::vecS,         // edge container
                      boost::vecS,         // vertex container
                      boost::undirectedS,
                      Vertex,              // vertex struct
                      Edge>;               // edge struct

  using G_list_u = boost::adjacency_list<
                      boost::listS,
                      boost::listS,
                      boost::undirectedS,
                      Vertex,
                      Edge>;

  // directed
  using G_vec_d = boost::adjacency_list<
                      boost::vecS,
                      boost::vecS,
                      boost::bidirectionalS,
                      Vertex,
                      Edge>;

  using G_list_d = boost::adjacency_list<
                      boost::listS,
                      boost::listS,
                      boost::bidirectionalS,
                      Vertex,
                      Edge>;
};

// -- Helper functions --------------------------------------------------------
// Calculate expected number of edges in Klemm-Eguiluz graph
std::size_t expected_num_edges(const std::size_t num_vertices,
                               const std::size_t mean_degree,
                               const bool is_undirected)
{
    if (is_undirected) {
        if (mean_degree >= num_vertices-1) {
            return 1.0 * num_vertices * (num_vertices - 1)/2;
        }
        else {
            double a = sqrt(4.0*pow(num_vertices, 2) - 4.0*num_vertices*(mean_degree+1.0) +1.0);
            a *= -0.5;
            a += num_vertices - 0.5;
            a = std::round(a);
            return ((a*(a-1))+2*a*(num_vertices-a))/2;
        }
    }
    else {
          if (mean_degree >= num_vertices-1) {
              return 1.0 * num_vertices * (num_vertices - 1);
          }
          else {
              size_t a = std::round(1.0* num_vertices *
                  mean_degree/(2*(num_vertices-1)));
              return ((a*(a-1))+a*(num_vertices-a));
          }
    }
}

// Test against parallel or self edges
template<typename Graph>
void assert_no_parallel_self_edges(Graph& G) {
    size_t num_parallel = 0;
    for (auto [v, v_end] = boost::vertices(G); v!=v_end; ++v) {
        for (auto [e, e_end] = boost::out_edges(*v, G); e!=e_end; ++e) {
            int counter = 0;
            for (auto [g, g_end] = boost::out_edges(*v, G); g!=g_end; ++g) {
                if (target(*g, G) == target(*e, G)) {
                    counter += 1;
                }
            }
            if (counter > 1) {
                num_parallel += 1;
            }
            // Check against self-edges
            BOOST_TEST(target(*e, G) != *v);
        }
    }
    BOOST_TEST(num_parallel == 0);
}

// -- Tests -------------------------------------------------------------------

// Undirected graphs
BOOST_FIXTURE_TEST_CASE(create_KE_graph, Test_Graph)
{
    test_config_callable (

      [&](auto test_cfg){

        const auto g0 = create_graph<G_vec_u>(test_cfg, *rng);
        const auto g1 = create_graph<G_list_u>(test_cfg, *rng);
        const auto g2 = create_graph<G_vec_d>(test_cfg, *rng);
        const auto g3 = create_graph<G_list_d>(test_cfg, *rng);
        const auto num_vertices = get_as<std::size_t>("num_vertices", test_cfg);
        const auto mean_degree = get_as<std::size_t>("mean_degree", test_cfg);

        // Expected number of edges for undirected graphs
        const auto num_edges_u = expected_num_edges(num_vertices,
                                                    mean_degree,
                                                    boost::is_undirected(g0));
        // Expected number of edges for directed graphs
        const auto num_edges_d = expected_num_edges(num_vertices,
                                                    mean_degree,
                                                    boost::is_undirected(g2));

        BOOST_TEST(boost::num_vertices(g0) == num_vertices);
        BOOST_TEST(boost::num_edges(g0) == num_edges_u);

        assert_no_parallel_self_edges(g0);

        BOOST_TEST(boost::num_vertices(g1) == num_vertices);
        BOOST_TEST(boost::num_edges(g1) == num_edges_u);

        BOOST_TEST(boost::num_vertices(g2) == num_vertices);
        BOOST_TEST(boost::num_edges(g2) == num_edges_d);

        assert_no_parallel_self_edges(g2);

        BOOST_TEST(boost::num_vertices(g3) == num_vertices);
        BOOST_TEST(boost::num_edges(g3) == num_edges_d);

      },
      cfg
    );
}
