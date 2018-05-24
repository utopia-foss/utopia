#ifndef UTOPIA_TEST_MODEL_TEST_HH
#define UTOPIA_TEST_MODEL_TEST_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>

#include <dune/utopia/data_io/config.hh>
#include <dune/utopia/data_io/hdffile.hh>
#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/hdfdataset.hh>

namespace Utopia {
enum Strategy{ S0, S1};

struct State {
    Strategy strategy;
    double payoff;
};

struct Boundary {};

/// Define data types of SimpleEG model
using SimpleEGModelTypes = ModelTypes<State, Boundary>;

/// Simple model of evolutionary games on grids
/** ...
 *  ...
 */
template<class ManagerType>
class SimpleEGModel:
    public Model<SimpleEGModel<ManagerType>, SimpleEGModelTypes>
{
public:
    // convenience type definitions
    using Base = Model<SimpleEGModel<ManagerType>, SimpleEGModelTypes>;
    using Data = typename Base::Data;
    using BCType = typename Base::BCType;

private:
    // Base members
    const std::string _name;
    Utopia::DataIO::Config _config;
    std::shared_ptr<Utopia::DataIO::HDFGroup> _group;
    std::shared_ptr<std::mt19937> _rng;
    // Members of this model
    ManagerType _manager;


public:
    /// Construct the SimpleEG model
    /** \param name Name of this model instance
     *  \param config The corresponding config node
     *  \param group The HDFGroup to write data to
     */
    SimpleEGModel (const std::string name,
                   Utopia::DataIO::Config &config,
                   std::shared_ptr<Utopia::DataIO::HDFGroup> group,
                   std::shared_ptr<std::mt19937> rng,
                   ManagerType&& manager):
        Base(),
        // NOTE the following will need to be passed to the Base constructor
        //      once the base Model class is adjusted
        _name(name),
        _config(config[_name]),
        _group(group->open_group(_name)),
        _rng(rng),
        _manager(manager)
    {   
        // Initialize cells
        this->initialize_cells();

        // Setup the datasets
        setup_datasets();
    }

    /**
     * @brief Initialize the cells
     * 
     */
    void initialize_cells()
    {
        std::string initial_state = _config["initial_state"].as<std::string>();

        std::cout << "Initializing cells... initial_state: "
                  << initial_state << std::endl;

        if (initial_state == "random")
        {
            std::uniform_int_distribution<> dist(0, 1);
            for (auto&& cell : _manager.cells())
            {
                // int init_strategy = dist(_rng);
                
                // cell->state().strategy = init_strategy;
                auto& state = cell->state_new();
                // std::cout << state.payoff << std::endl;
                // cell->state_new().payoff = 0.;
                // std::cout << cell->state().payoff << std::endl;

                // cell.state_new().strategy = init_strategy;
                // cell.state_new().payoff = 0.;
            }
        } 
        else if (initial_state == "fraction")
        {

        }
        else if (initial_state == "single_s0")
        {

        }
        else if (initial_state == "single_s1")
        {

        }
        else
        {
            std::runtime_error("`initial_state` parameter value '"
                               + initial_state + "' is not supported!");
        }
    }

    // Setup functions
    /// Setup datasets
    void setup_datasets()
    {
        // TODO
        // ...
    }

    /// Setup the datasets


    /// Iterate single step
    void perform_step ()
    {
        // TODO
        // ...
    }

    /// Write data
    void write_data ()
    {
        // TODO
        // ...
    }

    // TODO Check what to do with the below methods
    /// Set model boundary condition
    // void set_boundary_condition (const BCType& bc) { _bc = bc; }

    /// Set model initial condition
    // void set_initial_condition (const Data& ic) { _state = ic; }

    /// Return const reference to stored data
    // const Data& data () const { return _state; }
};


/// Setup the grid manager
template<bool periodic=true, class RNGType=std::mt19937>
auto setup_manager(Utopia::DataIO::Config config, std::shared_ptr<RNGType> rng)
{
    // Extract grid size from config and create grid
    auto gsize = config["grid_size"].as<std::array<unsigned int, 2>>();
    auto grid = Utopia::Setup::create_grid<2>(gsize);

    // Create cells on that grid
    auto cells = Utopia::Setup::create_cells_on_grid<true>(grid, 0.0);

    // Create the manager
    auto manager = Utopia::Setup::create_manager_cells<true, periodic>(grid, cells, rng);

    return manager;
}


} // namespace Utopia

#endif // UTOPIA_TEST_MODEL_TEST_HH
