#ifndef UTOPIA_MODELS_PREDATORPREY_HH
#define UTOPIA_MODELS_PREDATORPREY_HH

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

    /// Construct a cell state with the use of a RNG
    template<class RNGType>
    State(const DataIO::Config& cfg, const std::shared_ptr<RNGType>& rng)
    :
    predator{},
    prey{}
    {
        std::uniform_real_distribution<double> dist(0., 1.);

        // Get the threshold probability value
        const auto p_prey = get_as<double>("p_prey", cfg);
        const auto p_predator = get_as<double>("p_predator", cfg);

        // Check wether the conditions for the probabilities are met
        if (p_prey < 0. 
            or p_predator < 0.
            or (p_prey + p_predator) > 1.)
        {
            throw std::invalid_argument(
                "Need `p_prey` and `p_predator` in [0, 1] and the sum not "
                "exceeding 1, but got values: " 
                + std::to_string(p_prey) + " and "
                + std::to_string(p_predator) + "!");
        }

        // Check that the prey and predator key are given in the configuration
        if (not cfg["prey"]){
            throw std::invalid_argument(
                "The 'prey' key is missing in the cell_params!"
            );
        }
        if (not cfg["predator"]){
            throw std::invalid_argument(
                "The 'predator' key is missing in the cell_params!"
            );
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
/** \detail The first template parameter specifies the type of the cell state,
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
    PredatorParams _predator_params;

    /// Prey-specific model parameters
    PreyParams _prey_params; 

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
    const std::shared_ptr<DataSet> _dset_prey;
    const std::shared_ptr<DataSet> _dset_predator;
    const std::shared_ptr<DataSet> _dset_resource_prey;
    const std::shared_ptr<DataSet> _dset_resource_predator;


    // .. Rule functions ......................................................
    /// Cost of Living
    /** subtract the cost of living from the resources of an individual and 
     * map the values below zero back to zero, then remove all individuals that
     * do not have sufficient resources 
     */
    Rule _cost_of_living = [this](const auto& cell) {
        // Get the state of the cell
        auto state = cell->state;

        // Subtract the cost of living and clamp the resources to the limits:
        // If the resources exceed the maximal resources they are equal to
        // the maximal resources and if they go below 0 they are mapped to 0.
        state.predator.resources = std::clamp(state.predator.resources 
                                              - _predator_params.cost_of_living, 
                                             0., 
                                             _predator_params.resource_max);
        state.prey.resources = std::clamp(state.prey.resources 
                                            - _prey_params.cost_of_living, 
                                          0., 
                                          _prey_params.resource_max);

        // Remove predators that have no resources.
        if (state.predator.on_cell and state.predator.resources <= 0.) 
        {
            state.predator.on_cell = false;
        }

        // Remove prey that have no resources.
        if (state.prey.on_cell and state.prey.resources <= 0.) 
        {
            state.prey.on_cell = false;
        }

        return state;
    };

    /// Move a predator to a neighboring cell
    /** \detail This function resets the states predator state and updates the
     *          neighboring predator state.
     */
    void _move_predator_to_nb_cell(const std::shared_ptr<Cell>& cell, 
                                   const std::shared_ptr<Cell>& nb_cell){
        auto state = cell->state;
        auto nb_state = nb_cell->state;

        state.predator.on_cell = false;
        state.predator.resources = 0.;

        nb_state.predator.on_cell = true;
        nb_state.predator.resources = state.predator.resources;
    }

    /// Move a prey to a neighboring cell
    /** \detail This function resets the states prey state and updates the
     *          neighboring prey state.
     */
    void _move_prey_to_nb_cell(const std::shared_ptr<Cell>& cell, 
                               const std::shared_ptr<Cell>& nb_cell){
        auto state = cell->state;
        auto nb_state = nb_cell->state;

        state.prey.on_cell = false;
        state.prey.resources = 0.;

        nb_state.prey.on_cell = true;
        nb_state.prey.resources = state.prey.resources;
    }

    void _move_predator(const std::shared_ptr<Cell>& cell) {
        // Get the state of the Cell
        auto state = cell->state;

        // Case: only a predator is on the cell
        if (state.predator.on_cell) {
            // checking neighbouring cells for prey and moving to that cell

            // clear the containers for cells that contain prey or empty cells
            // in the neighbourhood
            _prey_cell.clear();
            _empty_cell.clear();

            for (const auto& nb : this->_cm.neighbors_of(cell)) {
                auto nb_state = nb->state;

                if (nb_state.prey.on_cell) {
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
                _move_predator_to_nb_cell(cell, nb_cell);
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
                _move_predator_to_nb_cell(cell, nb_cell);
            }
        }
    }

    void _flee_prey(const std::shared_ptr<Cell>& cell) {
        auto& state = cell->state;

        if (state.prey.on_cell and state.predator.on_cell){

            // Flee, if possible, with a given flee probability
            if (this->_prob_distr(*this->_rng) < _prey_params.p_flee){
                // Collect the empty neighboring cells to which the prey could flee
                _empty_cell.clear();
                for (const auto& nb : this->_cm.neighbors_of(cell)) 
                {
                    if ((not nb->state.prey.on_cell) and 
                        (nb->state.predator.on_cell)) 
                    {
                        _empty_cell.push_back(nb);
                    }
                }

                // If there is an empty cell, move there
                if (_empty_cell.size() > 0)
                {
                    // Choose a random cell to move to
                    std::uniform_int_distribution<> 
                        dist(0, _empty_cell.size() - 1);

                    auto nb_cell = _empty_cell[dist(*this->_rng)];
                    _move_prey_to_nb_cell(cell, nb_cell);
                }
            }
        }
        // NOTE Should the case be added that the neighboring cell has a predator?
        //      In case that there are no empty cells.
        //      Then the prey has again a small probability to flee in the next
        //      round.
    }

    /// Define the movement rule of an individual
    /*+ Go through cells. If only a predator populates it, look for prey in
     * the neighbourhood and move to that cell or go to an empty cell if no
     * prey is found. If both predator and prey live on the same cell, the
     * prey flees with a certain probability 
     */
    Rule _move = [this](const auto& cell) {
        
        _move_predator(cell);

        _flee_prey(cell);

        return cell->state;
    };

    /// Define the eating rule
    /** Update procedure is as follows:
     * - prey is consumed by a predator if they are on the same cell. 
     *   The predator increases its resources.
     * - prey eats grass which increases its resources
     */
    Rule _eat = [this](const auto& cell) {
        // Get the state of the cell
        auto state = cell->state;

        // Predator eats prey
        if (state.predator.on_cell and state.prey.on_cell) {
            // Increase the predator's resources and assure that the resource 
            // maximum is not exceeded by clamping into the correct interval.
            state.predator.resources = std::clamp(
                                        state.predator.resources 
                                            + _predator_params.resource_intake, 
                                        0., 
                                        _predator_params.resource_max);
            
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
                                        + _prey_params.resource_intake, 
                                    0., 
                                    _prey_params.resource_max);
        }
        return state;
    };

    /// Define the reproduction rule
    /** \detail If space is available reproduce with reproduction probabilities
     *          of predator and prey respectively.
     */
    Rule _repro = [this](const auto& cell) {
        // Get the state of the cell
        auto state = cell->state;
        
        // Reproduce predators
        if (    (state.predator.on_cell)
            and (this->_prob_distr(*this->_rng) < _predator_params.repro_prob)
            and (state.predator.resources >= _predator_params.repro_resource_requ))
        {
            // Collect available neighboring spots without predators
            _repro_cell.clear();
            for (const auto& nb : this->_cm.neighbors_of(cell))
            {
                if (not nb->state.predator.on_cell)
                {
                    _repro_cell.push_back(nb);
                }
            }

            // Choose an available neighboring cell to put an offspring on
            if (_repro_cell.size() > 0) {
                // choose a random cell for the offspring to be placed on
                std::uniform_int_distribution<> dist(0, _repro_cell.size()-1);
                auto nb_cell = _repro_cell[dist(*this->_rng)];
                auto nb_state = nb_cell->state;

                // neighboring cell has now a predator. 
                // Congratulations to your new baby! :)
                nb_state.predator.on_cell = true;

                // transfer resources from parent to offspring
                nb_state.predator.resources = _predator_params.repro_cost;
                state.predator.resources -= _predator_params.repro_cost;
            }
        }

        // Reproduce prey
        if ((state.prey.on_cell)
                and this->_prob_distr(*this->_rng) < _prey_params.repro_prob
                and state.prey.resources >= _prey_params.repro_resource_requ)
        {
            _repro_cell.clear();

            for (const auto& nb : this->_cm.neighbors_of(cell))
            {
                if (not nb->state.prey.on_cell)
                {
                    _repro_cell.push_back(nb);
                }
            }

            if (_repro_cell.size() > 0)
            {
                // choose a random cell for the offspring to be placed on
                std::uniform_int_distribution<> dist(0, _repro_cell.size() - 1);

                auto nb_cell = _repro_cell[dist(*this->_rng)];

                // neighboring cell has now a prey. 
                // Congratulations to your new baby! :)
                nb_cell->state.prey.on_cell = true;

                // transfer energy from parent to offspring
                nb_cell->state.prey.resources = _prey_params.repro_cost;
                state.prey.resources -= _prey_params.repro_cost;

                // NOTE Should there be some dissipation of resources during
                //      reproduction?
            }
        }

        return state;
    };


public:
    // -- Model setup ---------------------------------------------------------
    /// Construct the PredatorPrey model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     */
    template <class ParentModel>
    PredatorPrey(const std::string name, ParentModel& parent)
    : 
        Base(name, parent),
        // Initialize the cell manager, binding it to this model
        _cm(*this),
        // Extract model parameters
        _predator_params(this->_cfg["predator_params"]),
        _prey_params(this->_cfg["prey_params"]),
        // Temporary cell containers
        _prey_cell(),
        _empty_cell(),
        _repro_cell(),
        // uniform real distribution
        _prob_distr(0., 1.),
        // create datasets
        _dset_prey(this->create_cm_dset("prey", _cm)),
        _dset_predator(this->create_cm_dset("predator", _cm)),
        _dset_resource_prey(this->create_cm_dset("resource_prey", _cm)),
        _dset_resource_predator(this->create_cm_dset("resource_predator", _cm))
    {
        // Check if _repro_cost is in the allowed range
        if (_predator_params.repro_cost > _predator_params.repro_resource_requ 
            or _prey_params.repro_cost > _prey_params.repro_resource_requ) {
            throw std::invalid_argument("repro_cost needs to be smaller "
                                        "than or equal to the minimal "
                                        "reproduction requirements of "
                                        "resources!");
        }
        
        // Reserve space for the three helper vectors
        const auto nbh_mode = get_as<std::string>
                            ("mode", this->_cfg["cell_manager"]["neighborhood"]);
        if (nbh_mode == "VonNeumann")
        {
            _prey_cell.reserve(4);
            _empty_cell.reserve(4);
            _repro_cell.reserve(4);
        }
        else if (nbh_mode == "Moore")
        {
            _prey_cell.reserve(8);
            _empty_cell.reserve(8);
            _repro_cell.reserve(8);
        }

        // Write initial state
        this->write_data();
        this->_log->debug("Initial state written.");
    }

public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Perform an iteration step
    /** \detail An iteration step consists of:
     * 1. Subtracting costs of living
     * 2. Let predator and prey move to neighboring cells
     * 3. Lunch time: Prey eats grass and predator eats prey if on the same cell
     * 4. Reproduction: Create offspring
     */
    void perform_step() {
        apply_rule<Update::async, Shuffle::off>
            (_cost_of_living, _cm.cells(), *this->_rng);

        apply_rule<Update::async, Shuffle::on>
            (_move, _cm.cells(), *this->_rng);

        apply_rule<Update::async, Shuffle::off>
            (_eat, _cm.cells(), *this->_rng);

        apply_rule<Update::async, Shuffle::on>
            (_repro, _cm.cells(), *this->_rng);
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
                else if (state.predator.on_cell) {
                    predator_sum++;
                }
            }
            return std::pair{predator_sum / num_cells, prey_sum / num_cells};
        }();

        this->_monitor.set_entry("predator_density", pred_density);
        this->_monitor.set_entry("prey_density", prey_density);
    }

    /// Write data
    void write_data() {
        // Predator
        _dset_predator->write(_cm.cells().begin(), _cm.cells().end(), 
            [](auto& cell) {                
                if (cell->state.predator.on_cell){
                    return 1;
                }
                else{
                    return 0;
                }
            }
        );

        // Prey
        _dset_prey->write(_cm.cells().begin(), _cm.cells().end(), 
            [](auto& cell) {                
                if (cell->state.prey.on_cell){
                    return 1;
                }
                else{
                    return 0;
                }
            }
        );

        // resource of prey
        _dset_resource_prey->write(_cm.cells().begin(), _cm.cells().end(), 
            [](auto& cell) {
                return cell->state.prey.resources;
            }
        );

        // resource of predator
        _dset_resource_predator->write(_cm.cells().begin(), _cm.cells().end(), 
            [](auto& cell) {
                return cell->state.predator.resources;
            }
        );
    }
};

} // namespace PredatorPrey
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_PREDATORPREY_HH
