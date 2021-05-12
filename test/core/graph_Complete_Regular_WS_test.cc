#define BOOST_TEST_MODULE graph Watts-Strogatz test

#include <boost/test/included/unit_test.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <utopia/core/testtools.hh>

#include <utopia/core/types.hh>
#include <utopia/core/graph.hh>

using namespace Utopia::Graph;


// -- Types -------------------------------------------------------------------

struct Infrastructure : public Utopia::TestTools::BaseInfrastructure<> {
    Infrastructure() : BaseInfrastructure<>("graph_Complete_Regular_WS_test.yml") {};
};

struct Vertex {};

/// The test graph types
struct Graph_Undir : Infrastructure {
  using G_vec = boost::adjacency_list<
                      boost::vecS,         // edge container
                      boost::vecS,         // vertex container
                      boost::undirectedS,
                      Vertex>;             // vertex struct

  using G_list = boost::adjacency_list<
                      boost::listS,        // edge container
                      boost::listS,        // vertex container
                      boost::undirectedS,
                      Vertex>;             // vertex struct
};

struct Graph_Dir : Infrastructure {
  using G_vec = boost::adjacency_list<
                      boost::vecS,         // edge container
                      boost::vecS,         // vertex container
                      boost::bidirectionalS,
                      Vertex>;             // vertex struct

  using G_list = boost::adjacency_list<
                      boost::listS,        // edge container
                      boost::listS,        // vertex container
                      boost::bidirectionalS,
                      Vertex>;             // vertex struct
};

// -- Tests -------------------------------------------------------------------

// Undirected graphs
BOOST_FIXTURE_TEST_CASE(create_undirected_graph, Graph_Undir)
{
    const auto cfg_undir = cfg["Passing"]["Undirected"];

    for (const auto& model_map : cfg_undir){
      const auto& model_cfg = model_map.second;
      const auto model = model_map.second["model"].as<std::string>();
      const auto& num_vertices = model_map.second["num_vertices"].as<size_t>();
      const auto& mean_degree = model_map.second["mean_degree"].as<size_t>();
      const auto num_edges = model == "complete"
                             ? num_vertices*(num_vertices-1)/2
                             :  std::min((num_vertices * mean_degree / 2),
                                         num_vertices * (num_vertices-1)/2);

      const auto g0 = Utopia::Graph::create_graph<G_vec>(model_cfg, *rng);

      BOOST_TEST(boost::num_vertices(g0) == num_vertices);
      BOOST_TEST(boost::num_edges(g0) == num_edges);

      const auto g1 = Utopia::Graph::create_graph<G_list>(model_cfg, *rng);

      BOOST_TEST(boost::num_vertices(g1) == num_vertices);
      BOOST_TEST(boost::num_edges(g1) == num_edges);

      // Check for parallel edges
      size_t num_parallel = 0;
      for (auto [v, v_end] = boost::vertices(g0); v!=v_end; ++v) {
          for (auto [e, e_end] = boost::out_edges(*v, g0); e!=e_end; ++e) {
              size_t counter = 0;
              for (auto [g, g_end] = boost::out_edges(*v, g0); g!=g_end; ++g) {
                  if (target(*g, g0) == target(*e, g0)) {
                      counter += 1;
                  }
              }
              if (counter > 1) {
                  num_parallel += 1;
              }
              // Check against self-edges
              BOOST_TEST(target(*e, g0) != *v);
          }
      }

      BOOST_TEST(num_parallel == 0);
    }
}

// Directed graphs
BOOST_FIXTURE_TEST_CASE(create_directed_graph, Graph_Dir)
{
    const auto cfg_dir = cfg["Passing"]["Directed"];

    for (const auto& model_map : cfg_dir){
      const auto& model_cfg = model_map.second;
      const auto model = model_map.second["model"].as<std::string>();
      const auto& num_vertices = model_map.second["num_vertices"].as<size_t>();
      const auto& mean_degree = model_map.second["mean_degree"].as<size_t>();
      const auto num_edges = model == "complete"
                             ? num_vertices*(num_vertices-1)
                             :  std::min((num_vertices * mean_degree),
                                         num_vertices * (num_vertices-1));

      const auto g0 = Utopia::Graph::create_graph<G_vec>(model_cfg, *rng);

      BOOST_TEST(boost::num_vertices(g0) == num_vertices);
      BOOST_TEST(boost::num_edges(g0) == num_edges);

      const auto g1 = Utopia::Graph::create_graph<G_list>(model_cfg, *rng);

      BOOST_TEST(boost::num_vertices(g1) == num_vertices);
      BOOST_TEST(boost::num_edges(g1) == num_edges);

      // Check for parallel edges
      size_t num_parallel = 0;
      for (auto [v, v_end] = boost::vertices(g0); v!=v_end; ++v) {
          for (auto [e, e_end] = boost::out_edges(*v, g0); e!=e_end; ++e) {
              size_t counter = 0;
              for (auto [g, g_end] = boost::out_edges(*v, g0); g!=g_end; ++g) {
                  if (target(*g, g0) == target(*e, g0)) {
                      counter += 1;
                  }
              }
              if (counter > 1) {
                  num_parallel += 1;
              }
              // Check against self-edges
              BOOST_TEST(target(*e, g0) != *v);
          }
      }

      BOOST_TEST(num_parallel == 0);
    }
}

// Failing cases: undirected
BOOST_FIXTURE_TEST_CASE(failing_graphs_undir, Graph_Undir)
{
    const auto cfg_undir = cfg["Failing"]["Undirected"];

    for (const auto& model_map : cfg_undir){
      const auto& model_cfg = model_map.second;

      BOOST_CHECK_THROW(Utopia::Graph::create_graph<G_vec>(model_cfg, *rng),
                        std::invalid_argument);

      BOOST_CHECK_THROW(Utopia::Graph::create_graph<G_list>(model_cfg, *rng),
                        std::invalid_argument);
    }
}

// Failing cases: directed
BOOST_FIXTURE_TEST_CASE(failing_graphs_dir, Graph_Dir)
{
    const auto cfg_undir = cfg["Failing"]["Directed"];

    for (const auto& model_map : cfg_undir){
      const auto& model_cfg = model_map.second;

      BOOST_CHECK_THROW(Utopia::Graph::create_graph<G_vec>(model_cfg, *rng),
                        std::invalid_argument);

      BOOST_CHECK_THROW(Utopia::Graph::create_graph<G_list>(model_cfg, *rng),
                        std::invalid_argument);
    }
}
