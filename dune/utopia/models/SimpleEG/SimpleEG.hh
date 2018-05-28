#ifndef UTOPIA_TEST_MODEL_TEST_HH
#define UTOPIA_TEST_MODEL_TEST_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>

#include <dune/utopia/data_io/config.hh>
#include <dune/utopia/data_io/hdffile.hh>
#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/hdfdataset.hh>

namespace Utopia {

namespace SimpleEG {

// TODO check the namespace the following are implemented in
// Define the strategy enum
enum Strategy : unsigned short int { S0=0, S1=1 };

// Define the state struct, consisting of strategy and payoff
struct State {
    Strategy strategy;
    double payoff;
};

// Define the boundary type
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
    // Datasets
    std::shared_ptr<Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>> _dset_strategy;
    std::shared_ptr<Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>> _dset_payoff;


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
        _manager(manager),
        // datasets
        _dset_strategy(_group->open_dataset("strategy")),
        _dset_payoff(_group->open_dataset("payoff"))
    {   
        // Initialize cells
        this->initialize_cells();

        // Write initial state
        this->write_data();
    }

    // Setup functions ........................................................
    /// Initialize the cells according to `initial_state` config parameter
    void initialize_cells()
    {
        // Extract the mode that determines the initial strategy
        std::string initial_state = _config["initial_state"].as<std::string>();

        std::cout << "Initializing cells in '" << initial_state << "' mode ..."
                  << std::endl;

        // Distinguish according to the mode, which strategy to choose
        if (initial_state == "random")
        {
            // Use a uniform int distribution for determining the state
            auto rand_strat = std::bind(std::uniform_int_distribution<>(0, 1),
                                        *_rng);

            auto& cells = _manager.cells();
            auto set_random_strategy = [&rand_strat](const auto cell) {
                auto state = cell->state();
                state.strategy = static_cast<Strategy>(rand_strat());
                return state;

                // NOTE that setting the payoff to zero is not needed as that
                // is already done during initialization
            };
            // Apply the rule
            apply_rule(set_random_strategy, cells);
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
            Utopia::ValueError("`initial_state` parameter value '"
                               + initial_state + "' is not supported!");
        }

        std::cout << "Cells initialized." << std::endl;

    }


    // Runtime functions ......................................................

    /// Iterate single step
    void perform_step ()
    {
        // TODO
        // ...
    }

    /// Write data
    void write_data ()
    {
        std::cout << "Writing data for time step " << this->time << std::endl;
        
        // For the grid data, get the cells in order to iterate over them
        auto cells = _manager.cells();
        unsigned int num_cells = std::distance(cells.begin(), cells.end());

        // strategy
        _dset_strategy->write(cells.begin(), cells.end(),
                              [](auto& cell) {
                                return static_cast<int>(cell->state().strategy);
                              },
                              2,              // rank
                              {1, num_cells}, // extend of this entry
                              {},             // max_size of the dataset
                              1               // chunksize, for extension
                              );

        // payoffs
        _dset_payoff->write(  cells.begin(), cells.end(),
                              [](auto& cell) {
                                return cell->state().payoff;
                              },
                              2,              // rank
                              {1, num_cells}, // extend of this entry
                              {},             // max_size of the dataset
                              50              // chunksize, for extension
                              );
    }

    // TODO Check what to do with the below methods
    /// Set model boundary condition
    // void set_boundary_condition (const BCType& bc) { _bc = bc; }

    /// Set model initial condition
    // void set_initial_condition (const Data& ic) { _state = ic; }

    /// Return const reference to stored data
    // const Data& data () const { return _state; }
};


// TODO could be made a bit more general and then used elsewhere? This would
// predominantly require to find a convenient way to define the initial state?

/// Setup the grid manager with an initial state
/** \param config The top-level config node; 'grid_size' extracted from there
  * \param rng The top-level RNG
  */ 
template<bool periodic=true, unsigned int dim=2, class RNGType=std::mt19937>
auto setup_manager(Utopia::DataIO::Config config, std::shared_ptr<RNGType> rng)
{
    std::cout << "Setting up grid manager ..." << std::endl;

    // Extract grid size from config
    auto gsize = config["grid_size"].as<std::array<unsigned int, dim>>();

    // Inform about the size
    std::cout << "Creating " << dim << "-dimensional grid of size: ";
    for (const auto& dim_size : gsize) {
        std::cout << dim_size << " ";
    }
    std::cout << std::endl;

    // Create grid of that size
    auto grid = Utopia::Setup::create_grid<dim>(gsize);

    // Create the SimpleEG initial state: S0 and payoff 0.0
    State state_0 = {static_cast<Strategy>(0), 0.0};

    // Create cells on that grid, passing the initial state
    auto cells = Utopia::Setup::create_cells_on_grid<true>(grid, state_0);

    // Create the manager
    auto manager = Utopia::Setup::create_manager_cells<true, periodic>(grid, cells, rng);

    std::cout << "Grid manager initialized." << std::endl;

    return manager;
}

} // namespace SimpleEG

} // namespace Utopia

#endif // UTOPIA_TEST_MODEL_TEST_HH
