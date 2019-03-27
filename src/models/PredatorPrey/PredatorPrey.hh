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

/// Population enum, i.e.: possible cell states
enum class Population : unsigned short {
    /// Nobody on cell
    empty = 0,

    /// Prey on cell
    prey = 1,

    /// Predator on cell
    predator = 2,

    /// Both predator and prey on cell
    pred_prey = 3

    // NOTE Do NOT change enumeration; some dynamics depend on it
};

/// Cell State struct
struct State {
    /// The population on this cell
    Population population;

    /// The resources a predator on this cell has
    double resource_predator;

    /// The resources a prey on this cell has
    double resource_prey;

    /// Construct a cell state with the use of a RNG
    template<class RNGType>
    State(const DataIO::Config& cfg, const std::shared_ptr<RNGType>& rng)
    {
        std::uniform_real_distribution<double> dist(0., 1.);

        // Get the threshold probability value
        const auto p_prey = get_as<double>("p_prey", cfg);
        const auto p_predator = get_as<double>("p_predator", cfg);
        const auto p_predprey = get_as<double>("p_predprey", cfg);

        // Check wether the conditions for the probabilities are met
        if (p_prey < 0. 
            or p_predator < 0.
            or p_predprey < 0.
            or (p_prey + p_predator + p_predprey) > 1.)
        {
            throw std::invalid_argument(
                "Need `p_prey`, `p_predator` and `p_predprey` in "
                "[0, 1] and the sum not exceeding 1, but got values: " 
                + std::to_string(p_prey) + ", "
                + std::to_string(p_predator) + " and "
                + std::to_string(p_predprey));
        }

        // Draw a random real number in [0., 1.)
        const double rnum = dist(*rng);

        // ... and compare it to the thresholds. Then set the cell
        // state accordingly and distribute initial resources
        if (rnum < p_prey) {
            // put prey on the cell
            population = Population::prey;
            resource_prey = get_as<double>("prey", cfg["init_res"]);
            resource_predator = 0;
        }
        else if (rnum < (p_prey + p_predator)) {
            // put a predator on the cell
            population = Population::predator;
            resource_predator = get_as<double>("predator", cfg["init_res"]);
            resource_prey = 0;
        }
        else if (rnum < (p_prey + p_predator + p_predprey)) {
            // put a predator and a prey on the cell
            population = Population::pred_prey;
            resource_predator = get_as<double>("predator", cfg["init_res"]);
            resource_prey = get_as<double>("prey", cfg["init_res"]);
        }
        else {
            // initialize as empty and without resources
            population = Population::empty;
            resource_predator = 0;
            resource_prey = 0;
        }
    }
};


/// Cell traits specialization using the state type
/** \detail The first template parameter specifies the type of the cell state,
  *         the second sets them to not be synchronously updated.
  *         The default constructor for the cell state is preferred here
  */
using CellTraits = Utopia::CellTraits<State, UpdateMode::async, false>;

/// Typehelper to define data types of PredatorPrey model
using ModelTypes = ModelTypes<>;


// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// PredatorPrey Model on grid cells
/** Predators and prey correspond to the Population state of each cell, either
 * empty, prey, predator or both.
 * Cells are updated based on the following interactions:
 * 1) resource levels are reduced by a cost_of_living for both species
 * removed if resource = 0 
 * 2) predators move to neighbouring cells if there is a no prey on their own cell.
 * prey flees with a certain probability, if there is a predator on the same cell
 * 3) predators eat prey if on the same cell, else if there is only a prey it takes up resources
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
    Predator _predator;

    /// Prey-specific model parameters
    Prey _prey; 

    // .. Temporary objects ...................................................
    /// A container to temporarily accumulate the prey neighbour cells
    CellContainer<typename CellManager::Cell> _prey_cell;
    
    /// A container to temporarily accumulate empty neighbour cells
    CellContainer<typename CellManager::Cell> _empty_cell;

    /// A container to temporarily accumulate neighbour cells for reproduction
    CellContainer<typename CellManager::Cell> _repro_cell;

    // Uniform real distribution [0, 1) for evaluating probabilities
    std::uniform_real_distribution<double> _prob_distr;


    // .. Datasets ............................................................
    const std::shared_ptr<DataSet> _dset_population;
    const std::shared_ptr<DataSet> _dset_resource_prey;
    const std::shared_ptr<DataSet> _dset_resource_predator;


    // .. Rule functions ......................................................
    /// Cost of Living
    /** subtract the cost of living from the resources of an individual and 
     * map the values below zero back to zero, then remove all individuals that
     * do not have sufficient resources 
     */
    Rule _cost = [this](const auto& cell) {
        // Get the state of the cell
        auto state = cell->state();

        state.resource_predator = std::clamp(state.resource_predator 
                                                - _predator.cost_of_living, 
                                             0., 
                                             _predator.resource_max);
        state.resource_prey = std::clamp(state.resource_prey 
                                            - _prey.cost_of_living, 
                                         0., 
                                         _prey.resource_max);

        // Remove predators and preys that have no resources.
        // Prey always finds food and can only run out of energy if 
        // reproduction is very costly.
        if (state.population == Population::predator and state.resource_predator == 0.) {
            // Remove the predator
            state.population = Population::empty;
        }
        else if (state.population == Population::prey and state.resource_prey == 0) {
            // Remove the prey
            state.population = Population::empty;
        }
        else if (state.population == Population::pred_prey) {
            // Remove either one of them or both, depending on resources
            if (state.resource_predator == 0. and state.resource_prey == 0.)
                state.population = Population::empty;
            else if (state.resource_predator == 0.)
                state.population = Population::prey;
            else if (state.resource_prey == 0.)
                state.population = Population::predator;
        }

        return state;
    };

    /// Define the movement rule of an individual
    /*+ Go through cells. If only a predator populates it, look for prey in
     * the neighbourhood and move to that cell or go to an empty cell if no
     * prey is found. If both predator and prey live on the same cell, the
     * prey flees with a certain probability 
     */
    Rule _move = [this](const auto& cell) {
        // Get the state of the Cell
        auto state = cell->state();

        //Continue if there is only a predator on the cell
        if (state.population == Population::predator) {
            // checking neighbouring cells for prey and moving to that cell

            // clear the containers for cells that contain prey or empty cells
            // in the neighbourhood
            _prey_cell.clear();
            _empty_cell.clear();

            for (const auto& nb : this->_cm.neighbors_of(cell)) {
                auto nb_state = nb->state();

                if (nb_state.population == Population::prey) {
                    _prey_cell.push_back(nb);
                }
                else if (nb_state.population == Population::empty) {
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

                auto nb_cell = _prey_cell[dist_prey(*this->_rng)];
                nb_cell->state().population = Population::pred_prey;
                nb_cell->state().resource_predator = state.resource_predator;
                    
                state.population = Population::empty;
                state.resource_predator = 0.;
            }
         // if there is no prey the predator makes a random move    
            else if (_empty_cell.size() > 0) {
                // distribution to choose a random cell for the movement if 
                // there is more than one
                std::uniform_int_distribution<>
                    dist_empty(0, _empty_cell.size() - 1);


                auto nb_cell = _empty_cell[dist_empty(*this->_rng)];
                nb_cell->state().population = Population::predator;
                nb_cell->state().resource_predator = state.resource_predator;

                state.population = Population::empty;
                state.resource_predator = 0.;
            }
        }
        
     // continue if there are both predator and prey on a cell
        else if (state.population == Population::pred_prey) {
            // checking neighbouring cells if empty for the prey to flee

            // clear container for empty cells
            _empty_cell.clear();

            for (const auto& nb : this->_cm.neighbors_of(cell)) {
                if (nb->state().population == Population::empty) {
                    _empty_cell.push_back(nb);
                }
            }

            // choose a random cell for the movement if there is more than one
            std::uniform_int_distribution<> dist(0, _empty_cell.size() - 1);

         // the prey has a certain chance to flee
            if (    _empty_cell.size() > 0
                and this->_prob_distr(*this->_rng) < _prey.p_flee)
            {
                auto nb_cell = _empty_cell[dist(*this->_rng)];
                nb_cell->state().population = Population::prey;
                nb_cell->state().resource_prey = state.resource_prey;

                state.population = Population::predator;
                state.resource_prey = 0.;
            }
        }

        return state;
    };

    /// Define the eating rule
    /** Update procedure is as follows:
     * prey is consumed if predator and prey on the same cell
     * prey resource is increased if there is just prey on the cell
     */
    Rule _eat = [this](const auto& cell) {
        // Get the state of the cell
        auto state = cell->state();

        // preys get eaten by predators 
        if (state.population == Population::pred_prey) {
            state.population = Population::predator;
            
            // increase the resources and clamp to the allowed range [0, e_max]
            state.resource_predator = std::clamp(state.resource_predator 
                                                    + _predator.resource_intake, 
                                                 0., 
                                                 _predator.resource_max);
            state.resource_prey = 0.;   
        }
    // preys eat
        else if (state.population == Population::prey) {
            // increase the resources and clamp to the allowed range [0, e_max]
            state.resource_prey = std::clamp(state.resource_prey 
                                                    + _prey.resource_intake, 
                                             0., 
                                             _prey.resource_max);
        }
        return state;
    };

    /// Define the reproduction rule
    /** \detail If space is available reproduce with reproduction probabilities
     *          of predator and prey respectively.
     */
    Rule _repro = [this](const auto& cell) {
        // Get the state of the cell
        auto state = cell->state();
        
        if (   ( state.population == Population::predator or state.population == Population::pred_prey)
            and this->_prob_distr(*this->_rng) < _predator.p_repro
            and state.resource_predator >= _predator.resource_min)
        {
            _repro_cell.clear();

            for (const auto& nb : this->_cm.neighbors_of(cell))
            {
                auto nb_state = nb->state();

                if (nb_state.population == Population::prey 
                    or nb_state.population == Population::empty)
                {
                    _repro_cell.push_back(nb);
                }
            }

            if (_repro_cell.size() > 0) {
                // choose a random cell for the offspring to be placed on
                std::uniform_int_distribution<> dist(0, _repro_cell.size()-1);
                auto nb_cell = _repro_cell[dist(*this->_rng)];

                // new state will be predator or pred_prey
                if (nb_cell->state().population == Population::empty)
                {
                    nb_cell->state().population = Population::predator;
                }
                else
                {
                    nb_cell->state().population = Population::pred_prey;
                }

                // transfer energy from parent to offspring
                nb_cell->state().resource_predator = _predator.cost_of_repr;
                state.resource_predator -= _predator.cost_of_repr;
            }
        }

        if ((state.population == Population::prey or state.population == Population::pred_prey)
                and this->_prob_distr(*this->_rng) < _prey.p_repro
                and state.resource_prey >= _prey.resource_min)
        {
            _repro_cell.clear();

            for (const auto& nb : this->_cm.neighbors_of(cell))
            {
                auto nb_state = nb->state();

                if (nb_state.population == Population::predator 
                    or nb_state.population == Population::empty)
                {
                    _repro_cell.push_back(nb);
                }
            }

            if (_repro_cell.size() > 0)
            {
                // choose a random cell for the offspring to be placed on
                std::uniform_int_distribution<> dist(0, _repro_cell.size() - 1);

                auto nb_cell = _repro_cell[dist(*this->_rng)];

                // new state will be prey or pred_prey
                if (nb_cell->state().population == Population::empty)
                {
                    nb_cell->state().population = Population::prey;
                }
                else
                {
                    nb_cell->state().population = Population::pred_prey;
                }                            
                //  transfer energy from parent to offspring
                nb_cell->state().resource_prey = _prey.cost_of_repr;
                state.resource_prey -= _prey.cost_of_repr;
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
        _predator(this->_cfg["predator"]),
        _prey(this->_cfg["prey"]),
        // Temporary cell containers
        _prey_cell(),
        _empty_cell(),
        _repro_cell(),
        // uniform real distribution
        _prob_distr(0., 1.),
        // create datasets
        _dset_population(this->create_cm_dset("population", _cm)),
        _dset_resource_prey(this->create_cm_dset("resource_prey", _cm)),
        _dset_resource_predator(this->create_cm_dset("resource_predator", _cm))
    {
        // Check if _cost_of_repro is in the allowed range
        if (_predator.cost_of_repr > _predator.resource_min or _prey.cost_of_repr >_prey.resource_min) {
            throw std::invalid_argument("cost_of_repro needs to be smaller "
                                        "than or equal to e_min!");
        }
        
        // Write initial state
        this->write_data();
        this->_log->debug("Initial state written.");
    }

public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    void perform_step() {
        // Apply the rules to all cells

        // cost of living is subducted and individuals are removed if
        // resources are 0
        apply_rule<false>(_cost, _cm.cells());

        // predators hunt and prey flees
        apply_rule(_move, _cm.cells(), *this->_rng);

        // uptake of resources, prey gets eaten
        apply_rule<false>(_eat, _cm.cells());

        // reproduction
        apply_rule(_repro, _cm.cells(), *this->_rng);
    }

    /// Monitor model information
    void monitor () {
        /// Calculate the densities for both species
        auto [pred_density, prey_density] = [this](){
            double pred_sum = 0.;
            double prey_sum = 0.;
            double num_cells = this->_cm.cells().size();

            for (const auto& cell : this->_cm.cells()) {
                auto state = cell->state();
                if (state.population == Population::prey) {
                    prey_sum++;
                }
                else if (state.population == Population::predator) {
                    pred_sum++;
                }
                else if (state.population == Population::pred_prey) {
                    prey_sum++;
                    pred_sum++;
                }
            }
            return std::pair{pred_sum / num_cells, prey_sum / num_cells};
        }();

        this->_monitor.set_entry("predator_density", pred_density);
        this->_monitor.set_entry("prey_density", prey_density);
    }

    /// Write data
    void write_data() {
        // Population
        _dset_population->write(_cm.cells().begin(), _cm.cells().end(), 
            [](auto& cell) {
                return static_cast<unsigned short>(cell->state().population);
            }
        );

        // resource of prey
        _dset_resource_prey->write(_cm.cells().begin(), _cm.cells().end(), 
            [](auto& cell) {
                return cell->state().resource_prey;
            }
        );

        // resource of predator
        _dset_resource_predator->write(_cm.cells().begin(), _cm.cells().end(), 
            [](auto& cell) {
                return cell->state().resource_predator;
            }
        );
    }
};

} // namespace PredatorPrey
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_PREDATORPREY_HH
