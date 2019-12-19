#ifndef UTOPIA_DATAIO_TEST_HH
#define UTOPIA_DATAIO_TEST_HH

#include <sstream> // needed for testing throw messages from exceptions
#include <random> 

#include <utopia/data_io/hdfdataset.hh> // for messing around with datasets
#include <utopia/data_io/hdffile.hh>    // for buiding model mock class
#include <utopia/core/types.hh>

// for boost graph stuff
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/adjacency_matrix.hpp>
#include <boost/graph/random.hpp>
#include <boost/graph/properties.hpp>

namespace Utopia {
namespace DataIO {

/**
 * @brief Mock class for cell
 * 
 */
class Cell
{
  public:
    struct State
    {
        int _x;
        int _y;
        double _res;
    };

  private:
    State _state;

  public:
    State& state() { return _state; }

    Cell(int x, int y, double res)
      : _state(State{ x, y, res })
    {}
    Cell() = default;
    Cell(const Cell&) = default;
    Cell(Cell&&) = default;
    ~Cell() = default;
    Cell& operator=(const Cell&) = default;
    Cell& operator=(Cell&&) = default;
};

/**
 * @brief Mock class for agent
 * 
 */
class Agent
{
  public:
    struct State
    {
        int _age;
        double _adaption;
    };

  private:
    State _state;

  public:
    State& state() { return _state; }
    Agent(int age, double adaption)
      : _state(State{ age, adaption })
    {}
    Agent() = default;
    Agent(const Agent&) = default;
    Agent(Agent&&) = default;
    ~Agent() = default;
    Agent& operator=(const Agent&) = default;
    Agent& operator=(Agent&&) = default;
};

/**
 * @brief Mock class for cellmananger
 * 
 */
class Cellmanager
{
    std::vector<std::shared_ptr<Cell>> _cells;

  public:
    auto& cells() { return _cells; }

    Cellmanager(std::size_t size, Cell::State state)
      : _cells(std::vector<std::shared_ptr<Cell>>(
          size,
          std::make_shared<Cell>(state._x, state._y, state._res)))
    {}
    Cellmanager() = default;
    Cellmanager(const Cellmanager&) = default;
    Cellmanager(Cellmanager&&) = default;
    ~Cellmanager() = default;
    Cellmanager& operator=(const Cellmanager&) = default;
    Cellmanager& operator=(Cellmanager&&) = default;
};

/**
 * @brief Mock class for agentmanager
 * 
 */
class Agentmanager
{
    std::vector<std::shared_ptr<Agent>> _agents;

  public:
    auto& agents() { return _agents; }

    Agentmanager(std::size_t size, Agent::State state)
      : _agents(std::vector<std::shared_ptr<Agent>>(
          size,
          std::make_shared<Agent>(state._age, state._adaption)))
    {}
    Agentmanager() = default;
    Agentmanager(const Agentmanager&) = default;
    Agentmanager(Agentmanager&&) = default;
    ~Agentmanager() = default;
    Agentmanager& operator=(const Agentmanager&) = default;
    Agentmanager& operator=(Agentmanager&&) = default;
};

/**
 * @brief Mocking class for tasks
 *
 */
template<typename B, typename W>
struct Task
{
    using Builder = B;
    using Writer = W;
    Builder build_dataset;
    Writer write;
    Utopia::DataIO::HDFGroup group;

    Task(Builder b, Writer w, Utopia::DataIO::HDFGroup g)
      : build_dataset(b)
      , write(w)
      , group(g)
    {}
    Task() = default;
    Task(const Task&) = default;
    Task(Task&&) = default;
    Task& operator=(Task&&) = default;
    Task& operator=(const Task&) = default;
    virtual ~Task() = default;
};

/**
 * @brief Mocking class for tasks ,basic. Needed for testing polymorphism
 *
 */
struct BasicTask
{
    std::string str;

    virtual void write() { str = "base"; }

    BasicTask() = default;
    BasicTask(const BasicTask&) = default;
    BasicTask(BasicTask&&) = default;
    BasicTask& operator=(const BasicTask&) = default;
    BasicTask& operator=(BasicTask&&) = default;
    virtual ~BasicTask() = default;
};

/**
 * @brief Mocking class for a task which derives from BasicTask
 *
 */
struct DerivedTask : public BasicTask
{
    using Base = BasicTask;

    virtual void write() override { this->str = "derived"; }

    DerivedTask() = default;
    DerivedTask(const DerivedTask&) = default;
    DerivedTask(DerivedTask&&) = default;
    DerivedTask& operator=(DerivedTask&&) = default;
    DerivedTask& operator=(const DerivedTask&) = default;
    virtual ~DerivedTask() = default;
};

// helpers for graphs
/// Vertex struct containing some properties
struct Vertex {
    int test_int;
    double test_double;
    std::size_t id;

    double get_test_value() const {
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

/// Creates a small test graph
template<typename GraphType>
GraphType create_and_initialize_test_graph(int num_vertices, int num_edges){
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


// mocking class for model 
/**
 * @brief Mocking class for the model
 *
 */

struct Model
{
    std::string name;
    Utopia::DataIO::HDFFile file;
    std::shared_ptr<spdlog::logger> logger;
    std::vector<int> x;
    std::size_t time;
    Cellmanager _cellmanager;
    Agentmanager _agentmanager;
    Config _conf;

    auto& get_cfg() { return _conf;}

    auto get_logger() { return logger; }

    std::size_t get_time() { return time; }

    auto get_name() { return name; }

    auto get_hdfgrp() { return file.get_basegroup(); }

    auto& get_agentmanager() { return _agentmanager; }

    auto& get_cellmanager() { return _cellmanager; }


    Model(std::string n,
          std::string cfg_path,
          std::size_t cellnum,
          std::size_t agentnum,
          Cell::State cstate,
          Agent::State astate)
      : name(n)
      , file(n + ".h5", "w")
      , logger(spdlog::stdout_color_mt("logger." + n))
      , x(1000, 5)
      , time(0)
      , _cellmanager(cellnum, cstate)
      , _agentmanager(agentnum, astate)
      , _conf(YAML::LoadFile(cfg_path))
    {}

    Model(std::string n)
      : name(n)
      , file(n + ".h5", "w")
      , logger(spdlog::stdout_color_mt("logger." + n))
      ,
      // mock data
      x([]() {
          std::vector<int> v(100);
          std::iota(v.begin(), v.end(), 1);
          return v;
      }())
      , time(0)
      , _cellmanager()
      , _agentmanager()
    {}

    Model() = default;
    Model(const Model&) = delete;
    Model(Model&&) = default;
    Model& operator=(Model&&) = default;
    Model& operator=(const Model&) = delete;
    ~Model() { file.close(); }
};


// Mock a model class with a graph
template<typename Graph>
struct GraphModel
{
    std::string name;
    Utopia::DataIO::HDFFile file;
    std::shared_ptr<spdlog::logger> logger;
    Graph graph;
    std::size_t time;

    auto get_logger() { return logger; }

    std::size_t get_time() { return time; }

    Graph& get_graph(){return graph;}

    auto get_hdfgrp() { return file.get_basegroup(); }

    GraphModel(std::string n, int num_vertices, int num_edges)
      : name(n)
      , file(n + ".h5", "w")
      , logger(spdlog::stdout_color_mt("logger." + n))
      , graph(create_and_initialize_test_graph<Graph>(num_vertices, num_edges))
      , time(0)
    {}


    GraphModel() = default;
    GraphModel(const GraphModel&) = default;
    GraphModel(GraphModel&&) = default;
    GraphModel& operator=(GraphModel&&) = default;
    GraphModel& operator=(const GraphModel&) = default;
    ~GraphModel() { file.close(); }

};


} // namespace DataIO
} // namespace Utopia
#endif
