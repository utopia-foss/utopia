#ifndef UTOPIA_MODELS_SIMPLEEG_HH
#define UTOPIA_MODELS_SIMPLEEG_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>

namespace Utopia {

namespace Models { // TODO check if additional namespace might be good!

/// Strategy enum
enum Strategy : unsigned short int { S0=0, S1=1 };


/// State struct for SimpleEG model, consisting of strategy and payoff
struct State {
    Strategy strategy;
    double payoff;
};


/// Boundary condition type
struct Boundary {};
// TODO do we need this?


// Alias the neighborhood classes
using NextNeighbor = Utopia::Neighborhoods::NextNeighbor;
using MooreNeighbor = Utopia::Neighborhoods::MooreNeighbor;


/// Typehelper to define data types of SimpleEG model 
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
    /// The base model
    using Base = Model<SimpleEGModel<ManagerType>, SimpleEGModelTypes>;
    
    /// Data type of the state
    using Data = typename Base::Data;
    
    /// Data type of the boundary condition
    using BCType = typename Base::BCType;
    
    /// Data type that holds the configuration
    using Config = typename Base::Config;
    
    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename Base::DataGroup;
    
    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Data type of the shared RNG
    using RNG = typename Base::RNG;

private:
    // Base members: time, name, cfg, hdfgrp, rng
    // Members of this model
    ManagerType _manager;
    std::vector<std::vector<double>> _ia_matrix;
    // Pointers to datasets
    std::shared_ptr<DataSet> _dset_strategy;
    std::shared_ptr<DataSet> _dset_payoff;

public:
    /// Construct the SimpleEG model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template<class ParentModel>
    SimpleEGModel (const std::string name,
                   ParentModel &parent,
                   ManagerType&& manager)
    :
        // Initialize first via base model
        Base(name, parent),
        // Now initialize members specific to this class
        _manager(manager),
        _ia_matrix(this->extract_ia_matrix()),
        // datasets
        _dset_strategy(this->hdfgrp->open_dataset("strategy")),
        _dset_payoff(this->hdfgrp->open_dataset("payoff"))
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
        std::string initial_state = this->cfg["initial_state"].template as<std::string>();

        std::cout << "Initializing cells in '" << initial_state << "' mode ..."
                  << std::endl;

        // Distinguish according to the mode, which strategy to choose
        // NOTE that the payoff is already initialized to zero.
        if (initial_state == "random")
        {
            // Use a uniform int distribution for determining the state
            auto rand_strat = std::bind(std::uniform_int_distribution<>(0, 1),
                                        *this->rng);

            auto& cells = _manager.cells();
            auto set_random_strategy = [&rand_strat](const auto cell) {
                auto state = cell->state();
                state.strategy = static_cast<Strategy>(rand_strat());
                return state;
            };
            // Apply the rule
            apply_rule(set_random_strategy, cells);
        } 
        else if (initial_state == "fraction")
        {
            const auto s1_fraction = this->cfg["s1_fraction"].template as<double>();

            // Use a uniform real distribution to determine the state
            auto rand_proposal = std::bind(std::uniform_real_distribution<>(),
                                           *this->rng);

            auto& cells = _manager.cells();
            auto set_fraction_strategy = [&rand_proposal,&s1_fraction](const auto cell){
                auto state = cell->state();
                if (rand_proposal() < s1_fraction){
                    state.strategy = S1;
                }
                else {
                    state.strategy = S0;
                }

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
            if (initial_state == "single_s0") {
                default_strategy = S1;
                single_strategy = S0;
            }
            else {
                default_strategy = S0;
                single_strategy = S1;
            }

            auto& cells = _manager.cells();

            // Get the grid size and perform checks on it
            auto grid_size = this->cfg["grid_size"].template as<std::pair<std::size_t, std::size_t>>();
            // TODO Check with Benni: why pair?!

            if ((grid_size.first % 2 == 0) || (grid_size.second % 2 == 0)) {
                throw std::invalid_argument("Need odd values for grid_size to "
                                            "calculate central cell for "
                                            "setting initial state to '"
                                            + initial_state + "'!");
            }
            // FIXME This is rather fragile. Better approach: calculate the
            //       central point (here!) and find the cell beneath that
            //       point, setting it to single_strategy.
            //       Use rule application to set default_strategy.

            auto set_initial_strategy = [&](const auto cell) {
                // Get the state of this cell
                auto state = cell->state();

                // Get the position
                auto pos = cell->position();
                // NOTE  Careful! Is a Dune::FieldVector<double, 2>
                //       Thus, need to do float calculations. The case with
                //       even grid_size extensions is caught above

                // Set the initial strategy depending on pos in the grid
                if (   pos[0] == (float) grid_size.first  / 2 
                    && pos[1] == (float) grid_size.second / 2) {
                    // The cell _is_ in the center of the grid
                    state.strategy = static_cast<Strategy>(single_strategy);
                }
                else {
                    // The cell _is not_ in the center of the grid
                    state.strategy = static_cast<Strategy>(default_strategy);
                }
                
                return state;
            };
            // Apply the rule
            apply_rule(set_initial_strategy, cells);
        }
        else
        {
            throw std::invalid_argument("`initial_state` parameter with value "
                                        "'" + initial_state + "' is not "
                                        "supported!");
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
                state.strategy = fittest_nbs[dist(*this->rng)]->state().strategy;
            }
            else{
                // There is no fittest neighbor. This case should never occur
                throw std::runtime_error("There was no fittest neighbor in "
                                         "the cell update. Should not occur!");
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

        // TODO Once implemented, use the higher-level wrapper for writing data
    }

    // TODO Check what to do with the below methods
    /// Set model boundary condition
    // void set_boundary_condition (const BCType& bc) { _bc = bc; }

    /// Set model initial condition
    // void set_initial_condition (const Data& ic) { _state = ic; }

    /// Return const reference to stored data
    // const Data& data () const { return _state; }

private:
    /// Extract the interaction matrix from the config file
    /** In the model config file there are three different ways to specify
     *  the interaction:
     * 1) Explicitely setting the interaction matrix `ia_matrix`
     *        S0      S1
     *   S0 [ia_00  ia_01]
     *   S1 [ia_10  ia_11]
     *
     * 2) Setting a benefit and cost pair `bc_pair`
     *        S0    S1
     *   S0 [b-c    -c]
     *   S1 [b       0]
     * 
     * 3) Setting the benefit paramter `b` following the paper of Nowak&May1992
     *        S0    S1
     *   S0 [1       0]
     *   S1 [b(>1)   0]
     * 
     * 
     * If 1) is set, 2) and 3) will be ignored. The function returns the
     * explicitely given ia_matrix.
     * If 1) is not set, then the interaction matrix of 2) will be returned.
     * If 2) and 3) are not set, the interaction matrix of 3) will be returned.
     * 
     * @return std::vector<std::vector<double>> The interaction matrix
     */
    std::vector<std::vector<double>> extract_ia_matrix()
    {
        return this->cfg["ia_matrix"].template as<std::vector<std::vector<double>>>();
        
        // // NOTE: The part below should work, if the `Config` class is abandoned and the config is saved as a YAML::Node
        // //          untill then, it is commented out

        // Return the ia_matrix if it is explicitly given in the config
        // if (this->cfg["ia_matrix"]){
        //     return this->cfg["ia_matrix"].as<std::vector<std::vector<double>>>();
        // }
        // // If ia_matrix is not provided in the config, get the ia_matrix from the bc-pair
        // else if (this->cfg["bc_pair"]){
        //     auto [b, c] = this->cfg["bc_pair"].as<std::pair<double, double>>();
        //     double ia_00 = b - c;
        //     double ia_01 = -c;
        //     double ia_10 = b;
        //     double ia_11 = 0.;
        //     std::vector row0 {ia_00, ia_10};
        //     std::vector row1 {ia_10, ia_11};
        //     std::vector<std::vector<double>> ia_matrix {row0, row1};
        //     return ia_matrix;
        // }
        // // If both previous cases are not provided, then return the ia_matrix given by the paramter "b"
        // // NOTE: There is no check for b>1 implemented here.
        // else if (this->cfg["b"]){
        //     auto b = this->cfg["b"].as<double>();
        //     double ia_00 = 1;
        //     double ia_01 = 0;
        //     double ia_10 = b;
        //     double ia_11 = 0.;
        //     std::vector row0 {ia_00, ia_10};
        //     std::vector row1 {ia_10, ia_11};
        //     std::vector<std::vector<double>> ia_matrix {row0, row1};
        //     return ia_matrix;
        // }
        // // Case where no interaction parameters are provided
        // else{
        //     std::runtime_error("The interaction matrix is not given!");
        //     throw;
        // }
    }
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

} // namespace Models

} // namespace Utopia

#endif // UTOPIA_MODELS_SIMPLEEG_HH
