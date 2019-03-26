#include <cassert>
#include <iostream>
#include <random>
#include <cstdio>

#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/random.hpp>
#include <boost/graph/properties.hpp>

#include <utopia/data_io/graph_utils.hh>
#include <utopia/data_io/hdfgroup.hh>
#include <utopia/data_io/hdffile.hh>


using namespace Utopia;


/// Vertex struct containing some properties
struct Vertex {
    int test_int;
    double test_double;
    std::size_t id;

    double get_test_value() {
        return test_double * test_int;
    }
};

/// Edge struct with property
struct Edge {
    float weight;
};


// Create different graph types to be tested.
using Graph_vertvecS_edgevecS_undir =  boost::adjacency_list<
                                            boost::vecS,        // edge container
                                            boost::vecS,        // vertex container
                                            boost::undirectedS,
                                            Vertex,             // vertex struct
                                            Edge>;              // edge struct

using Graph_vertlistS_edgelistS_undir =  boost::adjacency_list<
                                            boost::listS,       // edge container
                                            boost::listS,       // vertex container
                                            boost::undirectedS,
                                            Vertex,             // vertex struct
                                            Edge>;              // edge struct

using Graph_vertsetS_edgesetS_undir =  boost::adjacency_list<
                                            boost::setS,        // edge container
                                            boost::setS,        // vertex container
                                            boost::undirectedS,
                                            Vertex,             // vertex struct
                                            Edge>;              // edge struct

using Graph_vertvecS_edgevecS_dir =  boost::adjacency_list<
                                            boost::vecS,        // edge container
                                            boost::vecS,        // vertex container
                                            boost::directedS,
                                            Vertex,             // vertex struct
                                            Edge>;              // edge struct

using Graph_vertvecS_edgevecS_undir_vdesc = Graph_vertvecS_edgevecS_undir::vertex_descriptor;
using Graph_vertlistS_edgelistS_undir_vdesc = Graph_vertlistS_edgelistS_undir::vertex_descriptor;
using Graph_vertsetS_edgesetS_undir_vdesc = Graph_vertsetS_edgesetS_undir ::vertex_descriptor;
using Graph_vertvecS_edgevecS_dir_vdesc = Graph_vertvecS_edgevecS_dir::vertex_descriptor;

using Graph_vertvecS_edgevecS_undir_edesc = Graph_vertvecS_edgevecS_undir::edge_descriptor;
using Graph_vertlistS_edgelistS_undir_edesc = Graph_vertlistS_edgelistS_undir::edge_descriptor;
using Graph_vertsetS_edgesetS_undir_edesc = Graph_vertsetS_edgesetS_undir ::edge_descriptor;
using Graph_vertvecS_edgevecS_dir_edesc = Graph_vertvecS_edgevecS_dir::edge_descriptor;

/// Creates a small test graph
template<typename GraphType>
GraphType create_and_initialize_test_graph(const int num_vertices, const int num_edges){
    GraphType g;

    std::mt19937 rng(42);

    // Add vertices and initialize
    for (int i = 0; i < num_vertices; i++){
        // Add vertex
        auto v = add_vertex(g);
        
        // Initialize vertex
        g[v].test_int = num_vertices - i;
        g[v].test_double = 2.3;
        g[v].id = i;
    }

    // Randomly add edges
    for (int i = 0; i < num_edges; i++){
        // Add random edge
        auto v1 = random_vertex(g, rng);
        auto v2 = random_vertex(g, rng);
        auto e = add_edge(v1,v2,g);
        
        // Initialize edge
        g[e.first].weight = i;
    }

    // Return the graph
    return g;
}


/// Test the save graph functionality
void test_save_graph()
{
    using Utopia::DataIO::save_graph;

    // Create a test HDFFile and a HDFGroup
    auto hdf = Utopia::DataIO::HDFFile("graph_testfile.h5","w");
    auto grp = hdf.open_group("testgroup");

    // Test case 1:
    //     vertex container: boost::vecS
    //     list container:   boost::vecS

    // Set up graph
    auto g_vvu = create_and_initialize_test_graph<Graph_vertvecS_edgevecS_undir>(10,3);

    // Save the graph
    save_graph(g_vvu, grp, "g");

    // Test case 2:
    //     vertex container: boost::listS
    //     list container:   boost::listS

    // Set up graph
    auto g_llu = create_and_initialize_test_graph<Graph_vertlistS_edgelistS_undir>(10,3);

    // In the case of boost::listS, the graph does not store a boost::vertex_index internally.
    // Therefore, the save_graph function would not work.
    // To overcome this, the user needs to define an id in the Vertex struct itself.
    // It can be given to the save_data function through a property map, which
    // needs to be defined.
    // NOTE: The user needs to adapt this id, such that it actually represents a unique id
    //       for every agent within the model dynamics!
    using VertexIdMapLLU = boost::property_map<Graph_vertlistS_edgelistS_undir,
            std::size_t Vertex::*
    >::type;
    VertexIdMapLLU vertex_id_map_llu = get(&Vertex::id, g_llu);

    // Save the graph
    save_graph(g_llu, grp, "g_llu", vertex_id_map_llu);


    // Test case 3:
    //     vertex container: boost::setS
    //     list container:   boost::setS

    // Set up graph
    auto g_ssu = create_and_initialize_test_graph<Graph_vertsetS_edgesetS_undir>(10,3);

    // In the case of boost::setS, the graph does not store a boost::vertex_index internally.
    // Therefore, the save_graph function would not work.
    // To overcome this, the user needs to define an id in the Vertex struct itself.
    // It can be given to the save_data function through a property map, which
    // needs to be defined.
    // NOTE: The user needs to adapt this id, such that it actually represents a unique id
    //       for every agent within the model dynamics!
    using VertexIdMapSSU = boost::property_map<Graph_vertsetS_edgesetS_undir,
            std::size_t Vertex::*
    >::type;
    VertexIdMapSSU vertex_id_map_ssu = get(&Vertex::id, g_ssu);

    // Save the graph
    save_graph(g_ssu, grp, "g_ssu", vertex_id_map_ssu);


    // Test case 4:
    //     vertex container: boost::vecS
    //     list container:   boost::vecS
    //     undirected graph

    // Set up graph
    auto g_vvd = create_and_initialize_test_graph<Graph_vertvecS_edgevecS_dir>(10,3);

    // Save the graph
    save_graph(g_vvd, grp, "g_vvd");


    // Remove the graph testsfile
    std::remove("graph_testfile.h5");
}


/// Test if the data written to hdf5 matches the original data
/// Also test consistency of boost iterator range
template <typename GraphType, typename GRP_PTR>
void test_vertex_data(GraphType g, GRP_PTR grp, std::string name){

    std::vector<std::size_t> g_ids;
    std::vector<int> g_ints;
    std::vector<long long unsigned int> shape = {100};

    // get original data with the same iterator range
    for (auto v : boost::make_iterator_range(vertices(g))) {
        g_ids.push_back(g[v].id);
        g_ints.push_back(g[v].test_int);
    }

    // get the datasets
    auto dset_id = grp->open_group("id")->open_dataset(name);
    auto dset_int = grp->open_group("test_int")->open_dataset(name);

    // read data
    auto data_id = dset_id->template read<std::vector<std::size_t> >();
    auto data_int = dset_int->template read<std::vector<int> >();



    if ( g_ids != std::get<1>(data_id)) {

        throw std::runtime_error("Data \"id\" written by "
                           "\"save_graph_properties\" does not match "
                           "original data");
    }


    if ( g_ints != std::get<1>(data_int)) {
        throw std::runtime_error("Data \"test_int\" written by "
                           "\"save_graph_properties\" does not match "
                           "original data");
    }

    if (shape != std::get<0>(data_int)) {
        throw std::runtime_error("Data \"test_int\" written by "
                                 "\"save_graph_properties\" does not match "
                                 "desired shape");

    }

}

/// Test consistency of data order
template <typename GRP_PTR>
void test_vertex_data_consistency(GRP_PTR grp, std::string name){

    // get the datasets
    auto dset_id = grp->open_group("id")->open_dataset(name);
    auto dset_id2 = grp->open_group("id2")->open_dataset(name);

    // read data
    auto data_id = dset_id->template read<std::vector<std::size_t> >();
    auto data_id2 = dset_id2->template read<std::vector<std::size_t> >();



    if ( std::get<1>(data_id) != std::get<1>(data_id2)) {

        throw std::runtime_error("Data \"id\" and \"id2\" written by "
                                 "\"save_graph_properties\" do not match");
    }
}


/// Test the save graph vertex value functionality
void test_save_graph_vertex_value()
{
    using namespace Utopia::DataIO;

    // Create a test HDFFile and a HDFGroup
    auto hdf = Utopia::DataIO::HDFFile("graph_testfile.h5","w");
    auto grp = hdf.open_group("testgroup");

    auto vertex_vals = std::make_tuple(
            std::make_tuple(
                    "id",
                    [](auto& v){return v.id;}),
            std::make_tuple(
                    "test_double",
                    [](auto& v){return v.test_double;}),
            std::make_tuple(
                    "test_value",
                    [](auto& v){return v.get_test_value();}),
            std::make_tuple(
                    "test_int",
                    [](auto& v){return v.test_int;}),
            std::make_tuple(
                    "id2",
                    [](auto& v){return v.id;})
                           );

    auto vertex_desc_vals = std::make_tuple(
            std::make_tuple(
                    "id",
                    [](auto& g, auto& v){return g[v].id;}),
            std::make_tuple(
                    "test_double",
                    [](auto& g, auto& v){return g[v].test_double;}),
            std::make_tuple(
                    "test_value",
                    [](auto& g, auto& v){return g[v].get_test_value();}),
            std::make_tuple(
                    "test_int",
                    [](auto& g, auto& v){return g[v].test_int;}),
            std::make_tuple(
                    "id2",
                    [](auto& g, auto& v){return g[v].id;})
    );

    // Test case 1:
    //     vertex container: boost::vecS
    //     list container:   boost::vecS

    // Set up graph
    auto g_vvu = create_and_initialize_test_graph<Graph_vertvecS_edgevecS_undir>(100,30);

    save_graph_properties<Vertex>(g_vvu, grp, "g_vvu", vertex_vals);
    save_graph_properties<Graph_vertvecS_edgevecS_undir_vdesc>(g_vvu, grp, "g_vvu_desc", vertex_desc_vals);

    test_vertex_data(g_vvu, grp, "g_vvu");
    test_vertex_data_consistency(grp, "g_vvu");
    test_vertex_data(g_vvu, grp, "g_vvu_desc");
    test_vertex_data_consistency(grp, "g_vvu_desc");

    // Test case 2:
    //     vertex container: boost::listS
    //     list container:   boost::listS

    // Set up graph
    auto g_llu = create_and_initialize_test_graph<Graph_vertlistS_edgelistS_undir>(100,30);

    save_graph_properties<Vertex>(g_llu, grp, "g_llu", vertex_vals);
    save_graph_properties<Graph_vertlistS_edgelistS_undir_vdesc>(g_llu, grp, "g_llu_desc", vertex_desc_vals);

    test_vertex_data(g_llu, grp, "g_llu");
    test_vertex_data_consistency(grp, "g_llu");
    test_vertex_data(g_llu, grp, "g_llu_desc");
    test_vertex_data_consistency(grp, "g_llu_desc");


    // Test case 3:
    //     vertex container: boost::setS
    //     list container:   boost::setS

    // Set up graph
    auto g_ssu = create_and_initialize_test_graph<Graph_vertsetS_edgesetS_undir>(100,30);

    save_graph_properties<Vertex>(g_ssu, grp, "g_ssu", vertex_vals);
    save_graph_properties<Graph_vertsetS_edgesetS_undir_vdesc>(g_ssu, grp, "g_ssu_desc", vertex_desc_vals);

    // Do not test data here as the iterator range of setS might differ

    // However, the consistency in one call of `save_graph_properties` is given
    test_vertex_data_consistency(grp, "g_ssu");
    test_vertex_data_consistency(grp, "g_ssu_desc");


    // Test case 4:
    //     vertex container: boost::vecS
    //     list container:   boost::vecS
    //     undirected graph

    // Set up graph
    auto g_vvd = create_and_initialize_test_graph<Graph_vertvecS_edgevecS_dir>(100,30);

    save_graph_properties<Vertex>(g_vvd, grp, "g_vvd", vertex_vals);
    save_graph_properties<Graph_vertvecS_edgevecS_dir_vdesc >(g_vvd, grp, "g_vvd_desc", vertex_desc_vals);

    test_vertex_data(g_vvd, grp, "g_vvd");
    test_vertex_data_consistency(grp, "g_vvd");
    test_vertex_data(g_vvd, grp, "g_vvd_desc");
    test_vertex_data_consistency(grp, "g_vvd_desc");

    // Remove the graph testsfile
    std::remove("graph_testfile.h5");
}

/// Test if the data written to hdf5 matches the original data
/// Also test consistency of boost iterator range
template <typename GraphType, typename GRP_PTR>
void test_edge_data(GraphType g, GRP_PTR grp, std::string name){

    std::vector<float> g_weights;
    std::vector<std::size_t > g_sources;
    std::vector<std::size_t > g_targets;

    // get original data with the same iterator range
    for (auto e : boost::make_iterator_range(edges(g))) {
        g_weights.push_back(g[e].weight);
        g_sources.push_back(g[source(e, g)].id);
        g_targets.push_back(g[target(e, g)].id);
    }

    // get the datasets
    auto dset_weight = grp->open_group("weight")->open_dataset(name);
    auto dset_sources = grp->open_group("_sources")->open_dataset(name);
    auto dset_targets = grp->open_group("_targets")->open_dataset(name);

    // read data
    auto data_weight = dset_weight->template read<std::vector<float> >();
    auto data_sources = dset_sources->template read<std::vector<std::size_t> >();
    auto data_targets = dset_targets->template read<std::vector<std::size_t> >();

    if ( g_weights != std::get<1>(data_weight)) {

        throw std::runtime_error("Data \"weight\" written by "
                                 "\"save_graph_properties\" does not match "
                                 "original data");
    }

    if ( g_sources != std::get<1>(data_sources)) {

        throw std::runtime_error("Data \"_sources\" written by "
                                 "\"save_graph_properties\" does not match "
                                 "original data");
    }
    if ( g_targets != std::get<1>(data_targets)) {

        throw std::runtime_error("Data \"_targets\" written by "
                                 "\"save_graph_properties\" does not match "
                                 "original data");
    }

}

/// Test consistency of data order
template <typename GRP_PTR>
void test_edge_data_consistency(GRP_PTR grp, std::string name){

    // get the datasets
    auto dset_sources = grp->open_group("_sources")->open_dataset(name);
    auto dset_targets = grp->open_group("_targets")->open_dataset(name);
    auto dset_sources2 = grp->open_group("_sources2")->open_dataset(name);
    auto dset_targets2 = grp->open_group("_targets2")->open_dataset(name);

    // read data
    auto data_sources = dset_sources->template read<std::vector<std::size_t> >();
    auto data_targets = dset_targets->template read<std::vector<std::size_t> >();
    auto data_sources2 = dset_sources2->template read<std::vector<std::size_t> >();
    auto data_targets2 = dset_targets2->template read<std::vector<std::size_t> >();



    if ( std::get<1>(data_sources) != std::get<1>(data_sources2)) {

        throw std::runtime_error("Data \"_sources\" and \"_sources2\" written "
                                 "by \"save_graph_properties\" do not match");
    }
    if ( std::get<1>(data_sources) != std::get<1>(data_sources2)) {

        throw std::runtime_error("Data \"_targets\" and \"_targets2\" written "
                                 "by \"save_graph_properties\" do not match");
    }
}


/// Test the save graph edge value functionality
void test_save_graph_edge_value()
{
    using namespace Utopia::DataIO;

    // Create a test HDFFile and a HDFGroup
    auto hdf = Utopia::DataIO::HDFFile("graph_testfile.h5","w");
    auto grp = hdf.open_group("testgroup");

    auto edge_vals = std::make_tuple(
            std::make_tuple("weight", [](auto& e){return e.weight;})
    );

    auto edge_desc_vals = std::make_tuple(
            std::make_tuple("_sources",
                    [](auto& g, auto& e){return g[source(e, g)].id;}),
            std::make_tuple("_targets",
                    [](auto& g, auto& e){return g[target(e, g)].id;}),
            std::make_tuple("_sources2",
                            [](auto& g, auto& e){return g[source(e, g)].id;}),
            std::make_tuple("_targets2",
                            [](auto& g, auto& e){return g[target(e, g)].id;})
    );

    // Test case 1:
    //     vertex container: boost::vecS
    //     list container:   boost::vecS

    // Set up graph
    auto g_vvu = create_and_initialize_test_graph<Graph_vertvecS_edgevecS_undir>(100,30);

    save_graph_properties<Edge>(g_vvu, grp, "g_vvu", edge_vals);
    save_graph_properties<Graph_vertvecS_edgevecS_undir_edesc>(g_vvu, grp, "g_vvu", edge_desc_vals);

    test_edge_data(g_vvu, grp, "g_vvu");
    test_edge_data_consistency(grp, "g_vvu");

    // Test case 2:
    //     vertex container: boost::listS
    //     list container:   boost::listS

    // Set up graph
    auto g_llu = create_and_initialize_test_graph<Graph_vertlistS_edgelistS_undir>(100,30);

    save_graph_properties<Edge>(g_llu, grp, "g_llu", edge_vals);
    save_graph_properties<Graph_vertlistS_edgelistS_undir_edesc>(g_llu, grp, "g_llu", edge_desc_vals);

    test_edge_data(g_llu, grp, "g_llu");
    test_edge_data_consistency(grp, "g_llu");



    // Test case 3:
    //     vertex container: boost::setS
    //     list container:   boost::setS

    // Set up graph
    auto g_ssu = create_and_initialize_test_graph<Graph_vertsetS_edgesetS_undir>(100,30);


    save_graph_properties<Edge>(g_ssu, grp, "g_ssu", edge_vals);
    save_graph_properties<Graph_vertsetS_edgesetS_undir_edesc>(g_ssu, grp, "g_ssu", edge_desc_vals);

    test_edge_data_consistency(grp, "g_ssu");

    // Test case 4:
    //     vertex container: boost::vecS
    //     list container:   boost::vecS
    //     undirected graph

    // Set up graph
    auto g_vvd = create_and_initialize_test_graph<Graph_vertvecS_edgevecS_dir>(100,30);

    save_graph_properties<Edge>(g_vvd, grp, "g_vvd", edge_vals);
    save_graph_properties<Graph_vertvecS_edgevecS_dir_edesc >(g_vvd, grp, "g_vvd", edge_desc_vals);

    test_edge_data(g_vvd, grp, "g_vvd");
    test_edge_data_consistency(grp, "g_vvd");


    // Remove the graph testsfile
    std::remove("graph_testfile.h5");
}

int main() {
    try {
        // Setup
        Utopia::setup_loggers();

        // Run the test
        test_save_graph();
        test_save_graph_vertex_value();
        test_save_graph_edge_value();

        std::cout << "Tests successful." << std::endl;
        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cout << "Exception occurred!" << std::endl;
        return 1;
    }
}
