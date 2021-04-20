#ifndef UTOPIA_MODELS_PREDATORPREY_HH
#define UTOPIA_MODELS_PREDATORPREY_HH

#include <cstdint>
#include <algorithm>
#include <random>

#include <utopia/core/apply.hh>
#include <utopia/core/model.hh>
#include <utopia/core/cell_manager.hh>

#include "species.hh"


namespace Utopia {
namespace Models {
namespace PredatorPrey {

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Cell State struct
struct State {
    /// The state a predator on this cell has
    SpeciesState predator;

    /// The state a prey on this cell has
    SpeciesState prey;

    /// Flag to indicate if the predator on this cell has already moved  
    bool moved_predator;

    /// Construct a cell state with the use of a RNG
    template<class RNGType>
    State(const DataIO::Config& cfg, const std::shared_ptr<RNGType>& rng)
    :
        predator{},
        prey{},
        moved_predator(false)
    {
        std::uniform_real_distribution<double> dist(0., 1.);

        // Get the threshold probability value
        const auto p_prey = get_as<double>("p_prey", cfg);
        const auto p_predator = get_as<double>("p_predator", cfg);

        // Check wether the conditions for the probabilities are met
        if (   p_prey < 0.
            or p_predator < 0.
            or (p_prey + p_predator) > 1.)
        {
            throw std::invalid_argument(
                "Need `p_prey` and `p_predator` in [0, 1] and the sum not "
                "exceeding 1, but got values: "
                + std::to_string(p_prey) + " and "
                + std::to_string(p_predator) + "!");
        }

        // Set a prey on a cell with the given probability by generating a
        // random number in [0, 1) and compare it to wanted probability.
        if (dist(*rng) < p_prey){
            prey.on_cell = true;
            prey.resources = get_as<double>("init_resources",
                                            cfg["prey"]);
        }

        // Set a predator on a cell with the given probability.
        if (dist(*rng) < p_predator){
            predator.on_cell = true;
            predator.resources = get_as<double>("init_resources",
                                                cfg["predator"]);
        }
    }
};


/// Cell traits specialization using the state type
/** \details The first template parameter specifies the type of the cell state,
  *         the second sets them to not be synchronously updated.
  *         The config constructor for the cell state is preferred here
  */
using CellTraits = Utopia::CellTraits<State, Update::manual>;

/// Typehelper to define data types of PredatorPrey model
using ModelTypes = ModelTypes<>;


// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// PredatorPrey Model on grid cells
/** Predators and prey correspond to the Population state of each cell, either
 * empty, prey, predator or both.
 * Cells are updated based on the following interactions:
 * 1) resource levels are reduced by a cost_of_living for both species
 * and individuals are removed if resource <= 0
 * 2) predators move to neighbouring cells if there is a no prey on their
 * own cell. Prey flees with a certain probability, if there is a predator on
 * the same cell
 * 3) predators eat prey if on the same cell, else if there is only a prey it
 * takes up resources
 * 4) both predators and prey reproduce if resources are sufficient and if
 * there is a cell in their neighbourhood not already occupied by the same
 * species
 */
class PredatorPrey
    : public Model<PredatorPrey, ModelTypes>
{
public:
    /// The base model
    using Base = Model<PredatorPrey, ModelTypes>;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// The type of the cell manager
    using CellManager = Utopia::CellManager<CellTraits, PredatorPrey>;

    /// The type of a cell
    using Cell = typename CellManager::Cell;

    /// Type of the update rules
    using Rule = typename CellManager::RuleFunc;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor, _log, _space
    // ... but you should definitely check out the documentation ;)

    // -- Members -------------------------------------------------------------
    /// The cell manager
    CellManager _cm;

    // .. Model parameters ....................................................
    /// Predator-specific model parameters
    SpeciesParams _params;

    // .. Temporary objects ...................................................
    /// A container to temporarily accumulate the prey neighbour cells
    CellContainer<Cell> _prey_cell;

    /// A container to temporarily accumulate empty neighbour cells
    CellContainer<Cell> _empty_cell;

    /// A container to temporarily accumulate neighbour cells for reproduction
    CellContainer<Cell> _repro_cell;

    // Uniform real distribution [0, 1) for evaluating probabilities
    std::uniform_real_distribution<double> _prob_distr;


    // .. Datasets ............................................................
    /// Dataset of Prey locations on the grid
    const std::shared_ptr<DataSet> _dset_prey;

    /// Dataset of Predator locations on the grid
    const std::shared_ptr<DataSet> _dset_predator;

    /// Dataset of Prey resources on the grid
    const std::shared_ptr<DataSet> _dset_resource_prey;

    /// Dataset of Predator resources on the grid
    const std::shared_ptr<DataSet> _dset_resource_predator;


    // .. Rule functions and helper methods ...................................
    /// Cost of Living
    /** subtract the cost of living from the resources of an individual and
     * map the values below zero back to zero, then remove all individuals that
     * do not have sufficient resources
     */
    Rule _cost_of_living = [this](const auto& cell) {
        // Get the state of the cell
        auto& state = cell->state;

        // Subtract the cost of living and clamp the resources to the limits:
        // If the resources exceed the maximal resources they are equal to
        // the maximal resources and if they go below 0 they are mapped to 0.
        state.predator.resources =
            std::clamp(  state.predator.resources
                       - _params.predator.cost_of_living,
                       0., _params.predator.resource_max);
        state.prey.resources =
            std::clamp(  state.prey.resources
                       - _params.prey.cost_of_living,
                       0., _params.prey.resource_max);

        // Remove predators that have no resources.
        if (state.predator.on_cell and state.predator.resources <= 0.) {
            state.predator.on_cell = false;
        }

        // Remove prey that have no resources.
        if (state.prey.on_cell and state.prey.resources <= 0.) {
            state.prey.on_cell = false;
        }

        return state;
    };

    /// Move a predator to a neighboring cell
    /** \details This function resets the states predator state and updates the
     *          neighboring predator state.
     */
    void move_predator_to_nb_cell(const std::shared_ptr<Cell>& cell,
                                  const std::shared_ptr<Cell>& nb_cell) {
        auto& state = cell->state;
        auto& nb_state = nb_cell->state;

        nb_state.predator.on_cell = true;
        nb_state.predator.resources = state.predator.resources;

        state.predator.on_cell = false;
        state.predator.resources = 0.;
    }

    /// Move a prey to a neighboring cell
    /** \details This function resets the states prey state and updates the
     *          neighboring prey state.
     */
    void move_prey_to_nb_cell(const std::shared_ptr<Cell>& cell,
                              const std::shared_ptr<Cell>& nb_cell) {
        auto& state = cell->state;
        auto& nb_state = nb_cell->state;

        nb_state.prey.on_cell = true;
        nb_state.prey.resources = state.prey.resources;

        state.prey.on_cell = false;
        state.prey.resources = 0.;
    }

    /// Define the movement rule of predator
    /** If a predator is on the given cell, it moves random to a neighbour 
     * cell with prey on it. If no prey is available on neighbour cells, it 
     * moves randomly to an empty cell, if there is on. Otherwise it stays 
     * on its current cell.
     */  
    Rule _move_predator = [this](const auto& cell) {
        // Get the state of the Cell
        auto& state = cell->state;

        // Case: only a predator is on the cell
        if ((state.predator.on_cell) and (not state.moved_predator)) {
            // checking neighbouring cells for prey and moving to that cell

            // clear the containers for cells that contain prey or empty cells
            // in the neighbourhood
            _prey_cell.clear();
            _empty_cell.clear();

            for (const auto& nb : this->_cm.neighbors_of(cell)) {
                auto& nb_state = nb->state;

                if ((nb_state.prey.on_cell) and (not nb_state.predator.on_cell)) {
                    _prey_cell.push_back(nb);
                }
                // if neither a prey nor a predator is on the cell, then
                // it is empty
                else if (not nb_state.predator.on_cell) {
                    _empty_cell.push_back(nb);
                }
            }

            // if there is prey in the vicinity move the predator to a random
            // prey cell
            if (_prey_cell.size() > 0) {
                // distribution to choose a random cell for the movement if
                // there is more than one
                std::uniform_int_distribution<>
                    dist_prey(0, _prey_cell.size() - 1);

                // Select a random neighbor cell and move the predator to it
                auto nb_cell = _prey_cell[dist_prey(*this->_rng)];
                // Mark target cell as cell with moved predator
                nb_cell->state.moved_predator=true; 
                move_predator_to_nb_cell(cell, nb_cell);
            }

            // if there is no prey the predator makes a random move
            else if (_empty_cell.size() > 0) {
                // distribution to choose a random cell for the movement if
                // there is more than one
                std::uniform_int_distribution<>
                    dist_empty(0, _empty_cell.size() - 1);

                // Select a random empty neighbor cell and move the predator
                // to it
                auto nb_cell = _empty_cell[dist_empty(*this->_rng)];
                // Mark target cell as cell with moved predator
                nb_cell->state.moved_predator=true; 
                move_predator_to_nb_cell(cell, nb_cell);
            }
        }
        return state;
    };

    /// Define the movement rule of prey. 
    /**If a prey is on the same cell as a predator, determine whether it may 
     * flee and where to.
    */
    Rule _flee_prey = [this](const auto& cell) {
        auto& state = cell->state;

        if (state.prey.on_cell and state.predator.on_cell) {
            // Flee, if possible, with a given flee probability
            if (this->_prob_distr(*this->_rng) < _params.prey.p_flee){
                // Collect empty neighboring cells to which the prey could flee
                _empty_cell.clear();
                for (const auto& nb : this->_cm.neighbors_of(cell)) {
                    if (    (not nb->state.prey.on_cell)
                        and (not nb->state.predator.on_cell)) {
                        _empty_cell.push_back(nb);
                    }
                }

                // If there is an empty cell, move there
                if (_empty_cell.size() > 0) {
                    // Choose a random cell to move to
                    std::uniform_int_distribution<>
                        dist(0, _empty_cell.size() - 1);

                    auto nb_cell = _empty_cell[dist(*this->_rng)];
                    move_prey_to_nb_cell(cell, nb_cell);
                }
            }
        }
        return state;
        // NOTE Should the case be added that the neighboring cell has a predator?
        //      In case that there are no empty cells.
        //      Then the prey has again a small probability to flee in the next
        //      round.
    };

    /// Define the eating rule
    /** Update procedure is as follows:
     * - prey is consumed by a predator if they are on the same cell.
     *   The predator increases its resources.
     * - prey eats grass which increases its resources
     */
    Rule _eat = [this](const auto& cell) {
        // Get the state of the cell
        auto& state = cell->state;

        // Predator eats prey
        if (state.predator.on_cell and state.prey.on_cell) {
            // Increase the predator's resources and assure that the resource
            // maximum is not exceeded by clamping into the correct interval.
            state.predator.resources = std::clamp(
                                        state.predator.resources
                                            + _params.predator.resource_intake,
                                        0.,
                                        _params.predator.resource_max);

            // Remove the prey from the cell
            state.prey.on_cell = false;
            state.prey.resources = 0.;
        }

        // Prey eats grass
        else if (state.prey.on_cell == true) {
            // Increase the resources and clamp to the allowed range
            // [0, resource_max]
            state.prey.resources = std::clamp(
                                    state.prey.resources
                                        + _params.prey.resource_intake,
                                    0.,
                                    _params.prey.resource_max);
        }
        return state;
    };

    /// Define the reproduction rule
    /** \details If space is available reproduce with reproduction probabilities
     *          of predator and prey respectively.
     */
    Rule _repro = [this](const auto& cell) {
        // Get the state of the cell
        auto& state = cell->state;

        // Reproduce predators
        if (    state.predator.on_cell
            and (this->_prob_distr(*this->_rng) < _params.predator.repro_prob)
            and (state.predator.resources >= _params.predator.repro_resource_requ))
        {
            // Collect available neighboring spots without predators
            _repro_cell.clear();
            for (const auto& nb : this->_cm.neighbors_of(cell)) {
                if (not nb->state.predator.on_cell) {
                    _repro_cell.push_back(nb);
                }
            }

            // Choose an available neighboring cell to put an offspring on
            if (_repro_cell.size() > 0) {
                // choose a random cell for the offspring to be placed on
                std::uniform_int_distribution<> dist(0, _repro_cell.size()-1);
                const auto& nb_cell = _repro_cell[dist(*this->_rng)];
                auto& nb_state = nb_cell->state;

                // neighboring cell has now a predator.
                // Congratulations to your new baby! :)
                nb_state.predator.on_cell = true;

                // transfer resources from parent to offspring
                nb_state.predator.resources = _params.predator.repro_cost;
                state.predator.resources -= _params.predator.repro_cost;
            }
        }

        // Reproduce prey
        if (    state.prey.on_cell
            and this->_prob_distr(*this->_rng) < _params.prey.repro_prob
            and state.prey.resources >= _params.prey.repro_resource_requ)
        {
            _repro_cell.clear();

            for (const auto& nb : this->_cm.neighbors_of(cell)) {
                if (not nb->state.prey.on_cell) {
                    _repro_cell.push_back(nb);
                }
            }

            if (_repro_cell.size() > 0) {
                // choose a random cell for the offspring to be placed on
                std::uniform_int_distribution<> dist(0, _repro_cell.size() - 1);
                // TODO Use std::sample

                auto nb_cell = _repro_cell[dist(*this->_rng)];

                // neighboring cell has now a prey.
                // Congratulations to your new baby! :)
                nb_cell->state.prey.on_cell = true;

                // transfer energy from parent to offspring
                nb_cell->state.prey.resources = _params.prey.repro_cost;
                state.prey.resources -= _params.prey.repro_cost;

                // NOTE Should there be some dissipation of resources during
                //      reproduction?
            }
        }
        return state;
    };

    /// Resets the movement flag of predators to "false" for next turn.
    Rule _reset_predator_movement = [](const auto& cell) {
        auto& state = cell->state;
        state.moved_predator = false;
        return state;
    };


public:
    // -- Model setup ---------------------------------------------------------
    /// Construct the PredatorPrey model
    /** \param name             Name of this model instance; is used to extract
     *                          the configuration from the parent model and
     *                          set up a HDFGroup for this instance
     *  \param parent_model     The parent model this model instance resides in
     *  \param custom_cfg       A custom configuration to use instead of the
     *                          one extracted from the parent model using the
     *                          instance name
     */
    template <class ParentModel>
    PredatorPrey (
        const std::string& name,
        ParentModel& parent_model,
        const DataIO::Config& custom_cfg = {}
    )
    :
        Base(name, parent_model, custom_cfg),
        // Initialize the cell manager, binding it to this model
        _cm(*this),

        // Extract model parameters
        _params(this->_cfg),

        // Temporary cell containers
        _prey_cell(),
        _empty_cell(),
        _repro_cell(),

        // Initialize distributions
        _prob_distr(0., 1.),

        // Create datasets
        _dset_prey(this->create_cm_dset("prey", _cm)),
        _dset_predator(this->create_cm_dset("predator", _cm)),
        _dset_resource_prey(this->create_cm_dset("resource_prey", _cm)),
        _dset_resource_predator(this->create_cm_dset("resource_predator", _cm))
    {
        // Load the cell state from a file, overwriting the current state
        if (this->_cfg["cell_states_from_file"]) {
            const auto& cs_cfg = this->_cfg["cell_states_from_file"];
            const auto hdf5_file = get_as<std::string>("hdf5_file", cs_cfg);
            const auto predator_init_resources = get_as<double>("init_resources", 
                this->_cfg["cell_manager"]["cell_params"]["predator"]);
            const auto prey_init_resources = get_as<double>("init_resources", 
                this->_cfg["cell_manager"]["cell_params"]["prey"]);
            if (get_as<bool>("load_predator", cs_cfg)) {
                this->_log->info("Loading predator positions from file ...");

                // Use the CellManager to set the cell state from the data
                // given by the `predator` dataset. Load as int to be able to
                // detect that a user supplied invalid values (better than
                // failing silently, which would happen with booleans).
                _cm.set_cell_states<int>(hdf5_file, "predator",
                    [predator_init_resources](auto& cell, const int on_cell){
                        if (on_cell == 0 or on_cell == 1) {
                            cell->state.predator.on_cell = on_cell;
                            if(on_cell == 1){
                                cell->state.predator.resources = 
                                    predator_init_resources;
                            }
                            else{
                                cell->state.predator.resources = 0;
                            }
                            return;
                        }
                        throw std::invalid_argument("While setting predator "
                            "positions, encountered an invalid value: "
                            + std::to_string(on_cell) + ". Allowed: 0 or 1.");
                    }
                );

                this->_log->info("Predator positions loaded.");
            }

            if (get_as<bool>("load_prey", cs_cfg)) {
                this->_log->info("Loading prey positions from file ...");

                _cm.set_cell_states<int>(hdf5_file, "prey",
                    [prey_init_resources](auto& cell, const int on_cell){
                        if (on_cell == 0 or on_cell == 1) {
                            cell->state.prey.on_cell = on_cell;
                            if(on_cell == 1){
                                cell->state.prey.resources = 
                                    prey_init_resources;
                            }
                            else{
                                cell->state.prey.resources = 0;
                            }
                            return;
                        }
                        throw std::invalid_argument("While setting prey "
                            "positions, encountered an invalid value: "
                            + std::to_string(on_cell) + ". Allowed: 0 or 1.");
                    }
                );

                this->_log->info("Prey positions loaded.");
            }
        }

        // Reserve memory in the size of the neighborhood for the temp. vectors
        const auto nb_size = _cm.nb_size();
        _prey_cell.reserve(nb_size);
        _empty_cell.reserve(nb_size);
        _repro_cell.reserve(nb_size);

        // Initialization finished
        this->_log->info("{} model fully set up.", this->_name);
    }

public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Perform an iteration step
    /** \details An iteration step consists of:
     *     1. Subtracting costs of living
     *     2. Let predator and prey move to neighboring cells
     *     3. Lunch time: Prey eats grass and predator eats prey if on the
     *        same cell
     *     4. Reproduction: Create offspring
     */
    void perform_step() {
        apply_rule<Update::async, Shuffle::off>
            (_cost_of_living, _cm.cells());
        
        apply_rule<Update::async, Shuffle::on>
            (_move_predator, _cm.cells(), *this->_rng);

        apply_rule<Update::async, Shuffle::on>
            (_flee_prey, _cm.cells(), *this->_rng);

        apply_rule<Update::async, Shuffle::off>
            (_eat, _cm.cells());

        apply_rule<Update::async, Shuffle::on>
            (_repro, _cm.cells(), *this->_rng);

        apply_rule<Update::sync>
            (_reset_predator_movement, _cm.cells());
    }

    /// Monitor model information
    void monitor () {
        /// Calculate the densities for both species
        auto [pred_density, prey_density] = [this](){
            double predator_sum = 0.;
            double prey_sum = 0.;
            double num_cells = this->_cm.cells().size();

            for (const auto& cell : this->_cm.cells()) {
                auto state = cell->state;

                if (state.prey.on_cell) {
                    prey_sum++;
                }

                if (state.predator.on_cell) {
                    predator_sum++;
                }
            }
            return std::pair{predator_sum / num_cells, prey_sum / num_cells};
        }();

        this->_monitor.set_entry("predator_density", pred_density);
        this->_monitor.set_entry("prey_density", prey_density);
    }

    /// Write data
    /** \details When invoked, stores cell positions and resources of both prey
      *         and predators.
      *
      * \note   Positions are cast to char and therefore stored as such. This
      *         is because C has no native boolean type, and the HDF5 library
      *         thus cannot store it directly. With 8 bit width, char is the
      *         smallest data type available; short int is already 16 bit.
      */
    void write_data() {
        // Predator
        _dset_predator->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return static_cast<char>(cell->state.predator.on_cell);
            }
        );

        // Prey
        _dset_prey->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return static_cast<char>(cell->state.prey.on_cell);
            }
        );

        // resource of predator
        _dset_resource_predator->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return cell->state.predator.resources;
            }
        );

        // resource of prey
        _dset_resource_prey->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return cell->state.prey.resources;
            }
        );
    }
};

} // namespace PredatorPrey
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_PREDATORPREY_HH
