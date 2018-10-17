#ifndef UTOPIA_MODELS_SIMPLEEG_HH
#define UTOPIA_MODELS_SIMPLEEG_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>
#include <dune/utopia/core/types.hh>

#include <functional>


namespace Utopia {
namespace Models {
namespace SimpleEG {

/// Strategy enum
enum Strategy : unsigned short int { S0=0, S1=1 };


/// State struct for SimpleEG model, consisting of strategy and payoff
struct State {
    Strategy strategy;
    double payoff;
};


/// Typehelper to define data types of SimpleEG model 
using SimpleEGModelTypes = ModelTypes<>;



/// Simple model of evolutionary games on grids
/** In this model, cells have an internal strategy, which determines their 
 * success in the interacions with their neighboring cells. The success is
 * given by an interaction matrix. In one interaction step, every cell
 * interacts with all its neighboring cells (interaction lambda function). 
 * Afterwards, all cells are updated synchroneously (update lambda function). 
 * From a cells perspective, the mechanism is as follows: 
 * Look around in you neighborhood for the cell, which had the highest payoff 
 * from the interactions. Change your state to this `fittest` neighboring 
 * cell's state. If multiple cells within a neighborhood have the same payoff
 * choose randomly between their strategies.
 */
template<class ManagerType>
class SimpleEGModel:
    public Model<SimpleEGModel<ManagerType>, SimpleEGModelTypes>
{
public:
    /// The base model
    using Base = Model<SimpleEGModel<ManagerType>, SimpleEGModelTypes>;
    
    /// Cell type
    using CellType = typename ManagerType::Cell;

    /// Data type that holds the configuration
    using Config = typename Base::Config;
    
    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename Base::DataGroup;
    
    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Data type of the shared RNG
    using RNG = typename Base::RNG;

    /// Type of the interaction matrix
    using IAMatrixType = typename std::array<std::array<double,2>,2>;

    // Alias the neighborhood classes for easier access
    using NextNeighbor = Utopia::Neighborhoods::NextNeighbor;
    using MooreNeighbor = Utopia::Neighborhoods::MooreNeighbor;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _log, _mgr

    // -- Members of this model -- //
    /// The grid manager
    ManagerType _manager;

    /// The interaction matrix (extracted during initialization)
    const IAMatrixType _ia_matrix;


    // -- Temporary objects -- //
    /// A container to temporarily accumulate the fittest neighbour cells in
    CellContainer<typename ManagerType::Cell> _fittest_cells_in_nbhood;

    
    // -- Datasets -- //
    std::shared_ptr<DataSet> _dset_strategy;
    std::shared_ptr<DataSet> _dset_payoff;


    // -- Rule functions -- //
    /// Define the interaction between players
    std::function<State(std::shared_ptr<CellType>)> _interaction = [this](const auto cell){
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

    /// Define the update rule 
    std::function<State(std::shared_ptr<CellType>)> _update = [this](const auto cell){
        // Update procedure is as follows:
        // Loop through the neighbors and store all neighbors with the
        // highest payoff.
        // Use the member _fittest_cells_in_nbhood for this, such that the vector does
        // not need to be recreated for each cell.

        // NOTE In most cases the vector will contain only one cell.
        //      However, there are parameter regimes where multiple cells
        //      can have the same payoff; this approach copes with spatial
        //      artefacts regardless of the parameter regime and 

        // Get the state of the cell
        auto state = cell->state();

        // Set highest payoff in the neighborhood to the cell's payoff
        double highest_payoff = state.payoff;
        _fittest_cells_in_nbhood.clear();
        _fittest_cells_in_nbhood.push_back(cell);
        
        // Iterate over neighbours of this cell:
        for (auto nb : MooreNeighbor::neighbors(cell, this->_manager)){
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

        // Now, update the strategy according to the fittest neighbour
        if (_fittest_cells_in_nbhood.size() == 1) {
            // Only one fittest neighbour. -> The state of the current cell
            // is updated with that of the fittest neighbour.
            state.strategy = _fittest_cells_in_nbhood[0]->state().strategy;
        }
        else if (_fittest_cells_in_nbhood.size() > 1) {
            // There are multiple nbs with the same (highest) payoff.
            // -> Choose randomly one of them to pass on its strategy
            std::uniform_int_distribution<> dist(0, _fittest_cells_in_nbhood.size()-1);
            state.strategy = _fittest_cells_in_nbhood[dist(*this->_rng)]->state().strategy;
        }
        else {
            // There is no fittest neighbor. This case should never occur
            throw std::runtime_error("There was no fittest neighbor in "
                                        "the cell update. Should not occur!");
        }

        return state;
    };


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
        _fittest_cells_in_nbhood(),
        // And open datasets for strategy and payoff
        _dset_strategy(this->_hdfgrp->open_dataset("strategy")),
        _dset_payoff(this->_hdfgrp->open_dataset("payoff"))
    {   
        // Initialize cells
        this->initialize_cells();

        // Set dataset capacities
        // We already know the maximum number of steps and the number of cells
        const hsize_t num_cells = std::distance(_manager.cells().begin(),
                                                _manager.cells().end());
        this->_log->debug("Setting dataset capacities to {} x {} ...",
                          this->get_time_max() + 1, num_cells);
        _dset_strategy->set_capacity({this->get_time_max() + 1, num_cells});
        _dset_payoff->set_capacity(  {this->get_time_max() + 1, num_cells});

        // Write initial state
        this->write_data();

        // Write _ia_matrix in _hdfgrp attribute
        this->_hdfgrp->add_attribute("ia_matrix", _ia_matrix);
    }


    // Setup functions ........................................................
    /// Initialize the cells according to `initial_state` config parameter
    void initialize_cells()
    {
        // Extract the mode that determines the initial strategy
        auto initial_state = as_str(this->_cfg["initial_state"]);

        this->_log->info("Initializing cells in '{}' mode ...", initial_state);

        // Distinguish according to the mode, which strategy to choose
        // NOTE that the payoff is already initialized to zero.
        if (initial_state == "random")
        {
            // Get the threshold probability value
            const auto s1_prob = as_double(this->_cfg["s1_prob"]);

            // Use a uniform real distribution for random numbers
            auto rand = std::bind(std::uniform_real_distribution<>(),
                                  std::ref(*this->_rng));

            // Define the update rule
            auto set_random_strategy = [&rand, &s1_prob](const auto cell) {
                // Get the state
                auto state = cell->state();

                // Draw a random number and compare it to the threshold
                if (rand() < s1_prob) {
                    // Use strategy 1
                    state.strategy = Strategy::S1;
                }
                else {
                    // Use strategy 0
                    state.strategy = Strategy::S0;
                }

                return state;
            };
            
            // Apply the rule to all cells
            apply_rule(set_random_strategy, _manager.cells());
        } 
        else if (initial_state == "fraction")
        {
            // Get the value for the fraction of cells to have strategy 1
            const auto s1_fraction = as_double(this->_cfg["s1_fraction"]);

            if (s1_fraction > 1. || s1_fraction < 0.) {
                throw std::invalid_argument("Need `s1_fraction` in [0, 1], "
                                            "but got value: "
                                            + std::to_string(s1_fraction));
            }

            // Get the cells container
            auto& cells = _manager.cells();

            // Calculate the number of cells that should have that strategy
            const auto num_cells = cells.size();
            const std::size_t num_s1 = s1_fraction * num_cells;
            // NOTE this is a flooring calculation!

            this->_log->debug("Cells with strategy 1:  {} of {}",
                              num_s1, num_cells);

            // TODO Optionally, can add some logic here to make more clever
            //      assignments, i.e. starting out with all S1 if the number
            //      to set is higher than half ...

            // Need a counter of cells that were set to S1
            std::size_t num_set = 0;

            // Get the cells... and shuffle them.
            auto random_cells = _manager.cells();
            std::shuffle(random_cells.begin(), random_cells.end(),
                         *this->_rng);

            // Make num_s1 cells use strategy 1
            for (auto&& cell : random_cells){
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
        else if (initial_state == "single_s0" || initial_state == "single_s1")
        {
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

            const auto& cells = _manager.cells();

            // Get the grid extensions and perform checks on it
            const auto& grid_ext = _manager.extensions();
            // NOTE It is more robust to use the grid extensions than using the
            //      values stored in cfg["grid_size"].

            // For now, need to throw an error for non-odd-valued grid_ext
            if (!(   (std::fmod(grid_ext[0], 2) != 0.)
                  && (std::fmod(grid_ext[1], 2) != 0.))) {
                throw std::invalid_argument("Need odd grid extensions to "
                                            "calculate central cell for "
                                            "setting initial state to '"
                                            + initial_state + "'!");
            }
            // FIXME This and the below is rather fragile and should be
            //       improved. Better approach: calculate the central point
            //       (here!) and find the cell beneath that point, setting it
            //       to single_strategy. Then use rule application to set
            //       default_strategy.

            auto set_initial_strategy = [&](const auto cell) {
                // Get the state of this cell
                auto state = cell->state();

                // Get the position
                const auto& pos = cell->position();
                // NOTE  Careful! Is a Dune::FieldVector<double, 2>
                //       Thus, need to do float calculations. The case with
                //       even grid_size extensions is caught above

                // Set the initial strategy depending on pos in the grid
                if (   pos[0] == grid_ext[0] / 2 
                    && pos[1] == grid_ext[1] / 2) {
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
            apply_rule(set_initial_strategy, cells);
        }
        else
        {
            throw std::invalid_argument("`initial_state` parameter with value "
                                        "'" + initial_state + "' is not "
                                        "supported!");
        }

        this->_log->info("Cells initialized.");
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
        // Apply the rules to all cells
        apply_rule(_interaction, _manager.cells());
        apply_rule(_update, _manager.cells());
    }


    /// Monitor model information
    void monitor ()
    {

    }


    /// Write data
    void write_data ()
    {
        // strategy
        _dset_strategy->write(_manager.cells().begin(), _manager.cells().end(),
                              [](auto& cell) {
                                return static_cast<unsigned short int>(cell->state().strategy);
                              });

        // payoffs
        _dset_payoff->write(_manager.cells().begin(), _manager.cells().end(),
                            [](auto& cell) {
                                return cell->state().payoff;
                            });
    }


    // Getters and setters ....................................................
    // TODO Add them here once it is clear how models interface with each
    //      others' data.


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
     * If 1) and 2) are not set, the interaction matrix of 3) will be returned.
     * 
     * @return IAMatrixType The interaction matrix
     */
    const IAMatrixType extract_ia_matrix() const
    {
        // Return the ia_matrix if it is explicitly given in the config
        if (this->_cfg["ia_matrix"]) {
            return as_<IAMatrixType>(this->_cfg["ia_matrix"]);
        }
        else if (this->_cfg["bc_pair"]) {
            // If ia_matrix is not provided in the config, get the ia_matrix
            // from the bc-pair
            const auto [b, c] = as_<std::pair<double, double>>(this->_cfg["bc_pair"]);
            const double ia_00 = b - c;
            const double ia_01 = -c;
            const double ia_10 = b;
            const double ia_11 = 0.;

            const std::array<double,2> row0({{ia_00, ia_01}});
            const std::array<double,2> row1({{ia_10, ia_11}});

            const IAMatrixType ia_matrix({{row0, row1}});
            return ia_matrix;
        }
        else if (this->_cfg["b"]) {
            // If both previous cases are not provided, then return the
            // ia_matrix given by the paramter "b"
            // NOTE: There is no check for b>1 implemented here.
            const auto b = as_double(this->_cfg["b"]);
            const double ia_00 = 1;
            const double ia_01 = 0;
            const double ia_10 = b;
            const double ia_11 = 0.;

            const std::array<double,2> row0({{ia_00, ia_01}});
            const std::array<double,2> row1({{ia_10, ia_11}});

            const IAMatrixType ia_matrix({{row0, row1}});
            return ia_matrix;
        }

        // If we reach this point, not enough parameters were provided
        throw std::invalid_argument("No interaction matrix given! Check that "
                                    "at least one of the following config "
                                    "entries is available: `ia_matrix`, "
                                    "`bc_pair`, `b`");
    }
};


} // namespace SimpleEG
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_SIMPLEEG_HH
