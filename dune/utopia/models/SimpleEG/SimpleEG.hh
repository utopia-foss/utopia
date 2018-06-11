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

// Alias the neighborhood classes
using NextNeighbor = Utopia::Neighborhoods::NextNeighbor;
using MooreNeighbor = Utopia::Neighborhoods::MooreNeighbor;

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
    std::vector<std::vector<double>> _ia_matrix;
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
        _ia_matrix(config["ia_matrix"].as<std::vector<std::vector<double>>>()),
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
            const auto s1_fraction = _config["s1_fraction"].as<double>();

            // Use a uniform real distribution to determine the state
            auto rand_proposal = std::bind(std::uniform_real_distribution<>(0., 1.),
                                            *_rng);

            auto& cells = _manager.cells();
            auto set_fraction_strategy = [&rand_proposal,&s1_fraction](const auto cell){
                auto state = cell->state();
                if (rand_proposal() < s1_fraction){
                    state.strategy = S1;
                }
                else {
                    state.strategy = S0;
                }

                // NOTE that the payoff is already initialized to zero.

                return state;
            };

            // Apply the rule
            apply_rule(set_fraction_strategy, cells);
        }
        else if (initial_state == "single_s0" || initial_state == "single_s1")
        {
            // Determine which strategy is the common default strategy 
            // and which one is the single strategy in the center of the grid
            Strategy default_strategy, single_strategy;
            if (initial_state == "single_s0"){
                default_strategy = S1;
                single_strategy = S0;
            }
            else{
                default_strategy = S0;
                single_strategy = S1;
            }

            auto& cells = _manager.cells();
            auto grid_size = _config["grid_size"].as<std::pair<std::size_t, std::size_t>>();

            auto set_initial_strategy = [&](const auto cell) {
                // Get the position and state of the cell
                auto position = cell->position();
                auto state = cell->state();

                // Set the initial startegy
                if (position[0] != grid_size.first / 2 
                        && position[1] != grid_size.first / 2){
                    // Case: The cell is in the center of the grid
                    state.strategy = static_cast<Strategy>(default_strategy);
                }
                else{
                    // Case: The cell is not in the center of the grid
                    auto state = cell->state();
                    state.strategy = static_cast<Strategy>(single_strategy);
                }
                
                // NOTE that the payoff is already initialized to zero
                
                return state;
            };
            // Apply the rule
            apply_rule(set_initial_strategy, cells);
        }
        else
        {
            Utopia::ValueError("`initial_state` parameter value '"
                               + initial_state + "' is not supported!");
        }

        std::cout << "Cells initialized." << std::endl;

    }


    // Runtime functions ......................................................

    /** @brief Iterate a single step
     *  @detail In the config, the following interaction matrix is stored:
     *                S0                 S1
     *      S0 ( _ia_matrix[0][0]  _ia_matrix[0][1]  )
     *      S1 ( _ia_matrix[1][0]  _ia_matrix[1][1]  )
     *
     * The interaction payoff is given from the perspective of the left-column-
     * strategy. E.g. if S0 interacts with S1, S0 receives the payoff given by
     * _ia_matrix[0][1] whereas S1 receives the payoff given by _ia_matrix[1][0].
     */
    void perform_step ()
    {
        // Define the interaction rule lambda function
        auto interact = [this](const auto cell){
            // Get the state of the Cell
            auto state = cell->state();

            // First, reset payoff to zero
            state.payoff = 0.;

            // Go through neighboing cells, look at their strategies and add
            // the corresponding payoff only to the current cell's payoff.
            // NOTE: adding the corresponding payoff to the neighboring cell
            // would lead to payoffs being added multiple times!
            for (auto nb : MooreNeighbor::neighbors(cell, this->_manager))
            {
                auto nb_state = nb->state();
                if (state.strategy == S0 && nb_state.strategy == S0) {
                    state.payoff += _ia_matrix[0][0];
                }
                else if (state.strategy == S0 && nb_state.strategy == S1) {
                    state.payoff += _ia_matrix[0][1];
                }
                else if (state.strategy == S1 && nb_state.strategy == S0) {
                    state.payoff += _ia_matrix[1][0];
                }
                else if (state.strategy == S1 && nb_state.strategy == S1) {
                    state.payoff += _ia_matrix[1][1];
                }
            }
            return state;
        };

        // Define the update rule lambda function
        auto update = [this](const auto cell){
            using CellType = typename decltype(_manager)::Cell;

            // Get the state of the cell
            auto state = cell->state();

            // Set highest payoff in the neighborhood to zero
            double highest_payoff = 0.;

            // Store all neighboring cells with the highest payoff in this vector
            std::vector<std::shared_ptr<CellType>> fittest_nbs; 
            
            // Loop through the neighbors and store all neighbors with the highest payoff
            // NOTE that in most cases the vector will contain only one cell.
            // NOTE that this implementation is probably not the most efficient one
            //      but it guarantees to cope with spatial artifacts in certain parameter regimes.
            //      Any suggestions for a better approach?
            for (auto nb : MooreNeighbor::neighbors(cell, this->_manager)){
                if (nb->state().payoff > highest_payoff){
                    highest_payoff = nb->state().payoff;
                    fittest_nbs.clear();
                    fittest_nbs.push_back(nb);
                }
                else if (nb->state().payoff == highest_payoff){
                    highest_payoff = nb->state().payoff;
                    fittest_nbs.push_back(nb);
                }
            }

            if (fittest_nbs.size() == 1){
                // If there is only one fittest neighbor 
                // update the cell's new state with the state of the fittest neighbor
                state.strategy = fittest_nbs[0]->state().strategy;
            }
            else if (fittest_nbs.size() > 1){
                // If there are multiple nbs with the same highest payoff
                // choose randomly one of them to pass on its strategy
                std::uniform_int_distribution<> dist(0, fittest_nbs.size() - 1);
                state.strategy = fittest_nbs[dist(*_rng)]->state().strategy;
            }
            else{
                // There is no fittest neighbor. This case should never occur
                std::runtime_error("In the cell update, there is no fittest neighbor!");
            }

            return state;
        };

        // Apply the rule to all cells
        apply_rule(interact, _manager.cells());
        apply_rule(update, _manager.cells());
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
                                return static_cast<unsigned short int>(cell->state().strategy);
                              },
                              2,              // rank
                              {1, num_cells}, // extend of this entry
                              {},             // max_size of the dataset
                              8               // chunksize, for extension
                              );

        // payoffs
        _dset_payoff->write(  cells.begin(), cells.end(),
                              [](auto& cell) {
                                return cell->state().payoff;
                              },
                              2,              // rank
                              {1, num_cells}, // extend of this entry
                              {},             // max_size of the dataset
                              8               // chunksize, for extension
                              );

        // TODO Find a reasonable chunksize, e.g. like h5py does?
        // https://github.com/h5py/h5py/blob/5b77d54b4d0f47659b4e6f174cc1d001640dabdf/h5py/_hl/filters.py#L295
        // Alternatively: pre-allocate the whole dataset size, knowing the
        // number of columns (number of cells) and rows (num_steps)
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
