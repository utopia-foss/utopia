#ifndef UTOPIA_MODELS_SIMPLEEG_HH
#define UTOPIA_MODELS_SIMPLEEG_HH

#include <functional>

#include <dune/utopia/core/types.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/cell_manager.hh>
#include <dune/utopia/core/apply.hh>


namespace Utopia {
namespace Models {
namespace SimpleEG {

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Strategy enum
enum Strategy : unsigned short int { S0=0, S1=1 };

/// The type of the cell state for the SimpleEG model
struct CellState {
    /// The strategy
    Strategy strategy;

    /// The payoff
    double payoff;

    /// Default constructor: strategy S0 and zero payoff
    CellState()
    :
        strategy(S0),
        payoff(0.)
    {}
};


/// Specialize the CellTraits type helper for the SimpleEG model
/** \detail Specifies the type of each cells' state as first template argument
  *         and the update mode as second. The SimpleEG model relies on sync
  *         update for all its cells.
  *         The third template parameter here specifies that the default
  *         constructor is to be used for constructing the CellState.
  *
  * See \ref Utopia::CellTraits for more information.
  */
using CellTraits = Utopia::CellTraits<CellState, UpdateMode::sync, true>;



/// Typehelper to define data types of SimpleEG model 
using ModelTypes = Utopia::ModelTypes<>;



// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Simple model of evolutionary games on grids
/** In this model, cells have an internal strategy, which determines their 
 * success in the interactions with their neighboring cells. The success is
 * given by an interaction matrix. In one interaction step, every cell
 * interacts with all its neighboring cells (interaction lambda function). 
 * Afterwards, all cells are updated synchronously (update lambda function). 
 * From a cells perspective, the mechanism is as follows: 
 * Look around in you neighborhood for the cell, which had the highest payoff 
 * from the interactions. Change your state to this `fittest` neighboring 
 * cell's state. If multiple cells within a neighborhood have the same payoff
 * choose randomly between their strategies.
 */
class SimpleEG:
    public Model<SimpleEG, ModelTypes>
{
public:
    /// The base model
    using Base = Model<SimpleEG, ModelTypes>;

    /// Data type that holds the configuration
    using Config = typename Base::Config;
    
    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Type of the CellManager to use
    using CellManager = Utopia::CellManager<CellTraits, SimpleEG>;

    /// Extract the type of the rule function from the CellManager
    /** \detail This is a function that receives a reference to a cell and
      *         returns the new cell state. For more details, check out
      *         \ref Utopia::CellManager
      *
      * \note   Whether the cell state gets applied directly or
      *         requires a synchronous update depends on the update mode
      *         specified in the cell traits.
      */
    using RuleFunc = typename CellManager::RuleFunc;

    /// Data type of the shared RNG
    using RNG = typename Base::RNG;

    /// Type of the interaction matrix
    using IAMatrixType = typename std::array<std::array<double, 2>, 2>;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _log, _monitor, _space
    // ... but you should definitely check out the documentation ;)

    // -- Members -------------------------------------------------------------
    /// The cell manager
    CellManager _cm;

    /// The interaction matrix (extracted during initialization)
    const IAMatrixType _ia_matrix;


    // .. Temporary objects ...................................................
    /// A container to temporarily accumulate the fittest neighbor cells in
    CellContainer<typename CellManager::Cell> _fittest_cells_in_nbhood;

    /// Uniform real distribution in [0., 1.) used for evaluating probabilities
    std::uniform_real_distribution<double> _prob_distr;

    
    // .. Datasets ............................................................
    /// Stores cell strategies over time
    std::shared_ptr<DataSet> _dset_strategy;

    /// Stores cell payoffs over time
    std::shared_ptr<DataSet> _dset_payoff;


public:
    // -- Public interface ----------------------------------------------------
    /// Construct the SimpleEG model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     */
    template<class ParentModel>
    SimpleEG (const std::string name, ParentModel &parent)
    :
        // Initialize first via base model
        Base(name, parent),

        // Now initialize the cell manager
        _cm(*this),

        // Initialize model parameters
        _ia_matrix(this->extract_ia_matrix()),

        // ... and helpers objects
        _fittest_cells_in_nbhood(),
        _prob_distr(0., 1.),

        // And open datasets for strategy and payoff
        _dset_strategy(this->create_cm_dset("strategy", _cm)),
        _dset_payoff(this->create_cm_dset("payoff", _cm))
    {   
        // Initialize cells
        this->initialize_cells();

        // Write initial state
        this->write_data();

        // Write _ia_matrix in _hdfgrp attribute
        this->_hdfgrp->add_attribute("ia_matrix", _ia_matrix);
    }


private:
    // Setup functions ........................................................
    /// Initialize the cells according to `initial_state` config parameter
    /** \detail The cell initialization for most cases is done directly 
     *          at cell construction. However, some initialization options
     *          depend on the position of the cell on the grid. Only these cases
     *          are dealt with in this initialization function.
     */ 
    void initialize_cells() {
        static_assert(Base::Space::dim == 2, "Only 2D space is supported.");

        // Extract the mode that determines the initial strategy
        auto initial_state = get_as<std::string>("initial_state", this->_cfg);

        this->_log->info("Initializing cells in '{}' mode ...", initial_state);

        // Distinguish according to the mode, which strategy to choose
        if (initial_state == "random") {
            // Get the threshold probability value
            const auto s1_prob = get_as<double>("s1_prob", this->_cfg);

            // Define the update rule
            auto set_random_strategy = [this, &s1_prob](const auto cell) {
                auto state = cell->state();

                // Draw a random number and compare it to the threshold
                if (this->_prob_distr(*this->_rng) < s1_prob) {
                    state.strategy = Strategy::S1;
                }
                else {
                    state.strategy = Strategy::S0;
                }

                return state;
            };
            
            // Apply the rule to all cells
            apply_rule(set_random_strategy, _cm.cells());
        } 

        else if (initial_state == "fraction") {
            // Get the value for the fraction of cells to have strategy 1
            const auto s1_fraction = get_as<double>("s1_fraction", this->_cfg);

            if (s1_fraction > 1. or s1_fraction < 0.) {
                throw std::invalid_argument("Need `s1_fraction` in [0, 1], "
                    "but got value: " + std::to_string(s1_fraction));
            }

            // Calculate the number of cells that should have that strategy
            const auto num_cells = _cm.cells().size();
            const std::size_t num_s1 = s1_fraction * num_cells;
            // NOTE this is a flooring calculation!

            this->_log->debug("Cells with strategy 1:  {} of {}",
                              num_s1, num_cells);

            // TODO Optionally, can add some logic here to make more clever
            //      assignments, i.e. starting out with all S1 if the number
            //      to set is higher than half ...

            // Need a counter of cells that were set to S1
            std::size_t num_set = 0;

            // Get a copy of the cells container... and shuffle them.
            auto random_cells = _cm.cells();
            std::shuffle(random_cells.begin(), random_cells.end(),
                         *this->_rng);

            // Make num_s1 cells use strategy 1
            for (const auto& cell : random_cells) {
                // If the desired number of cells using strategy 1 is not yet reached change another cell's strategy
                if (num_set < num_s1) {
                    // Check if it already has strategy 1.
                    if (cell->state().strategy == Strategy::S1) {
                        // Already has strategy 1, don't set it again
                        continue;
                    }
                    // else: has strategy 0 -> set to S1 and increment counter
                    cell->state_new().strategy = Strategy::S1;
                    cell->update();

                    num_set++;
                }
                // Break, if fraction of strategy 1 is reached
                else break;
            }
        }

        else if (initial_state == "single_s0" or initial_state == "single_s1"){
            // Get the grid shape and perform checks on it
            const auto& grid_shape = _cm.grid()->shape();

            // Need to throw an error for non-odd-valued grid shape
            if (grid_shape[0] % 2 == 0 or grid_shape[1] % 2 == 0) {
                throw std::invalid_argument("In mode '" + initial_state + "', "
                    "odd grid extensions are required to calculate the "
                    "central cell! Either adapt your grid resolution or the "
                    "space's extent in order to have a different shape. "
                    );
            }
            // FIXME This and the below is rather fragile and should be
            //       improved. Better approach: calculate the central point
            //       (here!) and find the cell beneath that point, setting it
            //       to single_strategy. Then use rule application to set
            //       default_strategy.

            // Determine which strategy is the common default strategy 
            // and which one is the single strategy in the center of the grid
            Strategy default_strategy, single_strategy;
            if (initial_state == "single_s0") {
                default_strategy = Strategy::S1;
                single_strategy = Strategy::S0;
            }
            else {
                default_strategy = Strategy::S0;
                single_strategy = Strategy::S1;
            }

            // Define the rule function
            auto set_initial_strategy = [&](const auto& cell) {
                // Get the state of this cell
                auto state = cell->state();

                // Get the multi index to find out if this cell is central
                const auto midx = _cm.midx_of(cell);

                if (    midx[0] == grid_shape[0] / 2 
                    and midx[1] == grid_shape[1] / 2) {
                    // The cell _is_ in the center of the grid
                    state.strategy = single_strategy;
                }
                else {
                    // The cell _is not_ in the center of the grid
                    state.strategy = default_strategy;
                }
                
                return state;
            };

            // Apply the rule
            apply_rule(set_initial_strategy, _cm.cells());
        }
        else {
            throw std::invalid_argument("Invalid value '" + initial_state
                + "' for parameter `initial_state`! Allowed values: random, "
                "fraction, single_s0, single_s1.");
        }

        // Done.
        this->_log->info("Initialized all {} cells.", _cm.cells().size());
    }

    // .. Helper functions ....................................................

    /// Extract the interaction matrix from the config file
    /** In the model config file there are three different ways to specify
     *  the interaction:
     * 1) Explicitly setting the interaction matrix `ia_matrix`
     *        S0      S1
     *   S0 [ia_00  ia_01]
     *   S1 [ia_10  ia_11]
     *
     * 2) Setting a benefit and cost pair `bc_pair`
     *        S0    S1
     *   S0 [b-c    -c]
     *   S1 [b       0]
     * 
     * 3) Setting the benefit parameter `b` following the paper of Nowak&May1992
     *        S0    S1
     *   S0 [1       0]
     *   S1 [b(>1)   0]
     * 
     * 
     * If 1) is set, 2) and 3) will be ignored. The function returns the
     * explicitly given ia_matrix.
     * If 1) is not set, then the interaction matrix of 2) will be returned.
     * If 1) and 2) are not set, the interaction matrix of 3) will be returned.
     * 
     * @return IAMatrixType The interaction matrix
     */
    IAMatrixType extract_ia_matrix() const {
        // Return the ia_matrix if it is explicitly given in the config
        if (this->_cfg["ia_matrix"]) {
            return get_as<IAMatrixType>("ia_matrix", this->_cfg);
        }
        else if (this->_cfg["bc_pair"]) {
            // If ia_matrix is not provided in the config, get the ia_matrix
            // from the bc-pair
            const auto [b, c] = get_as<std::pair<double, double>>("bc_pair",
                                                                  this->_cfg);
            const double ia_00 = b - c;
            const double ia_01 = -c;
            const double ia_10 = b;
            const double ia_11 = 0.;

            const std::array<double,2> row0({{ia_00, ia_01}});
            const std::array<double,2> row1({{ia_10, ia_11}});

            return IAMatrixType({{row0, row1}});
        }
        else if (this->_cfg["b"]) {
            // If both previous cases are not provided, then return the
            // ia_matrix given by the parameter "b"
            const auto b = get_as<double>("b", this->_cfg);

            if (b <= 1.) {
                throw std::invalid_argument("Parameter `b` needs to be >1, "
                    "but was: " + std::to_string(b));
            }

            const double ia_00 = 1;
            const double ia_01 = 0;
            const double ia_10 = b;
            const double ia_11 = 0.;

            const std::array<double, 2> row0({{ia_00, ia_01}});
            const std::array<double, 2> row1({{ia_10, ia_11}});

            return IAMatrixType({{row0, row1}});;
        }

        // If we reach this point, not enough parameters were provided
        throw std::invalid_argument("No interaction matrix given! Check that "
                                    "at least one of the following config "
                                    "entries is available: `ia_matrix`, "
                                    "`bc_pair`, `b`");
    }


    // .. Rule Functions ......................................................
    // Rule functions that can be applied to the CellManager's cells

    /// The interaction between players
    /** \detail This rule calculates the payoff for a given cell, depending on
      *         the interaction matrix and the payoffs of the neighbors.
      */
    RuleFunc _interaction = [this](const auto& cell){
        // Get the state of the current cell and its strategy
        auto state = cell->state();

        // First, reset payoff to zero
        state.payoff = 0.;

        // Go through neighboring cells, look at their strategies and add
        // the corresponding payoff only to the current cell's payoff.
        // NOTE: Careful! Adding the corresponding payoff to the neighboring
        //       cell would lead to payoffs being added multiple times!
        for (const auto& nb : this->_cm.neighbors_of(cell)) {
            const auto nb_strategy = nb->state().strategy;

            // Calculate the payoff depending on this cell's and the other
            // cell's state
            if (state.strategy == S0 and nb_strategy == S0) {
                state.payoff += _ia_matrix[0][0];
            }
            else if (state.strategy == S0 and nb_strategy == S1) {
                state.payoff += _ia_matrix[0][1];
            }
            else if (state.strategy == S1 and nb_strategy == S0) {
                state.payoff += _ia_matrix[1][0];
            }
            else if (state.strategy == S1 and nb_strategy == S1) {
                state.payoff += _ia_matrix[1][1];
            }
        }

        // Return the updated state of this cell
        return state;
    };

    /// The update rule
    /** Update procedure is as follows:
      *   - Loop through the neighbors and store all neighbors with the
      *     highest payoff.
      *   - Use the member _fittest_cells_in_nbhood for this, such that the
      *     vector does not need to be recreated for each cell.
      *
      * \note In most cases the vector will contain only one cell. However,
      *       there are parameter regimes where multiple cells can have the
      *       same payoff; this approach copes with spatial artifacts
      *       regardless of the parameter regime.
      */
    RuleFunc _update = [this](const auto& cell){
        // Get the state of the cell
        auto state = cell->state();

        // Set highest payoff in the neighborhood to the cell's payoff
        double highest_payoff = state.payoff;
        _fittest_cells_in_nbhood.clear();
        _fittest_cells_in_nbhood.push_back(cell);
        
        // Iterate over neighbors of this cell:
        for (const auto& nb : this->_cm.neighbors_of(cell)){
            if (nb->state().payoff > highest_payoff) {
                // Found a new highest payoff
                highest_payoff = nb->state().payoff;
                _fittest_cells_in_nbhood.clear();
                _fittest_cells_in_nbhood.push_back(nb);
            }
            else if (nb->state().payoff == highest_payoff) {
                // Have a payoff equal to that of another cell
                _fittest_cells_in_nbhood.push_back(nb);
            }
            // else: payoff was below highest payoff
        }

        // Now, update the strategy according to the fittest neighbor
        if (_fittest_cells_in_nbhood.size() == 1) {
            // Only one fittest neighbor. -> The state of the current cell
            // is updated with that of the fittest neighbor.
            state.strategy = _fittest_cells_in_nbhood[0]->state().strategy;
        }
        else if (_fittest_cells_in_nbhood.size() > 1) {
            // There are multiple nbs with the same (highest) payoff.
            // -> Choose randomly one of them to pass on its strategy
            std::uniform_int_distribution<> dist(0, _fittest_cells_in_nbhood.size()-1);
            state.strategy = _fittest_cells_in_nbhood[dist(*this->_rng)]->state().strategy;
        }
        else {
            // There is no fittest neighbor. This case should never occur!
            throw std::runtime_error("There was no fittest neighbor in the "
                                     "cell update. Should not have occurred!");
        }

        return state;
    };


public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

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
    void perform_step () {
        // Apply the rules to all cells
        apply_rule(_interaction, _cm.cells());
        apply_rule(_update, _cm.cells());
    }


    /// Monitor model information
    void monitor () {

    }


    /// Write data: the strategy and payoff of each cell
    void write_data () {
        // strategy
        _dset_strategy->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return static_cast<unsigned short int>(cell->state().strategy);
            }
        );

        // payoffs
        _dset_payoff->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return cell->state().payoff;
            }
        );
    }

    // .. Getters and setters .................................................
    // Add getters and setters here to interface with other models
 
};


} // namespace SimpleEG
} // namespace Models
} // namespace Utopia
#endif // UTOPIA_MODELS_SIMPLEEG_HH
