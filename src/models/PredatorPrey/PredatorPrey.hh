#ifndef UTOPIA_MODELS_PREDATORPREY_HH
#define UTOPIA_MODELS_PREDATORPREY_HH

#include <algorithm>
#include <random>

#include <utopia/core/apply.hh>
#include <utopia/core/model.hh>
#include <utopia/core/cell_manager.hh>


namespace Utopia {
namespace Models {
namespace PredatorPrey {

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Population enum, i.e.: possible cell states
enum Population : unsigned short {
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

    /// Default constructor (empty population, zero resources)
    State()
    :
        population(empty),
        resource_predator(0.),
        resource_prey(0.)
    {}
};


/// Cell traits specialization using the state type
/** \detail The first template parameter specifies the type of the cell state,
  *         the second sets them to not be synchronously updated.
  *         The default constructor for the cell state is preferred here
  */
using CellTraits = Utopia::CellTraits<State, UpdateMode::async, true>;

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
    /// The cost of living of a predator
    double _cost_of_living_pred;
	
    /// The cost of living of a prey
    double _cost_of_living_prey;		

    /// The resource uptake of a predator
    double _delta_e_pred;
	
    /// The resource uptake of a prey	
    double _delta_e_prey;

    /// The maximum of resources a predator can carry
    double _e_max_pred;
	
    /// The maximum of resources a prey can carry
    double _e_max_prey;	

    /// The minimum resource level necessary for reproduction of a predator
    double _e_min_pred;
	
    /// The minimum resource level necessary for reproduction of a prey
    double _e_min_prey;	

    /// Predator's cost of reproduction, i.e. the resources transferred to the offspring
    double _cost_of_repro_pred;
	
    /// Prey's cost of reproduction, i.e. the resources transferred to the offspring
    double _cost_of_repro_prey;
	
    /// The probability to reproduce for the predator
    double _p_repro_pred;
	
    /// The probability to reproduce for prey 	
    double _p_repro_prey; 	

    /// The probability for prey to flee
    double _p_flee;


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
                                             - _cost_of_living_pred, 0., _e_max_pred);
        state.resource_prey = std::clamp(state.resource_prey 
                                         - _cost_of_living_prey, 0., _e_max_prey);

        // Remove predators and preys that have no resources 
	// ( prey always finds food and can only run out of energy if reproduction is very costly)
        if (state.population == predator && state.resource_predator == 0.) {
            // Remove the predator
            state.population = empty;
        }
        else if (state.population == prey && state.resource_prey == 0) {
            // Remove the prey
            state.population = empty;
        }
        else if (state.population == pred_prey) {
            // Remove either one of them or both, depending on resources
            if (state.resource_predator == 0. && state.resource_prey == 0.)
                state.population = empty;
            else if (state.resource_predator == 0.)
                state.population = prey;
            else if (state.resource_prey == 0.)
                state.population = predator;
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
        if (state.population == predator) {
            // checking neighbouring cells for prey and moving to that cell

            // clear the containers for cells that contain prey or empty cells
            // in the neighbourhood
            _prey_cell.clear();
            _empty_cell.clear();

            for (const auto& nb : this->_cm.neighbors_of(cell)) {
                auto nb_state = nb->state();

                if (nb_state.population == prey) {
                    _prey_cell.push_back(nb);
                }
                else if (nb_state.population == empty) {
                    _empty_cell.push_back(nb);
                }
            }

            // if there is prey in the vicinity move the predator to a random prey cell
            if (_prey_cell.size() > 0) {
                // distribution to choose a random cell for the movement if 
                // there is more than one
                std::uniform_int_distribution<>
                    dist_prey(0, _prey_cell.size() - 1);

                auto nb_cell = _prey_cell[dist_prey(*this->_rng)];
                nb_cell->state().population = pred_prey;
                nb_cell->state().resource_predator = state.resource_predator;
                    
                state.population = empty;
                state.resource_predator = 0.;
            }
	     // if there is no prey the predator makes a random move	
            else if (_empty_cell.size() > 0) {
                // distribution to choose a random cell for the movement if 
                // there is more than one
                std::uniform_int_distribution<>
                    dist_empty(0, _empty_cell.size() - 1);


                auto nb_cell = _empty_cell[dist_empty(*this->_rng)];
                nb_cell->state().population = predator;
                nb_cell->state().resource_predator = state.resource_predator;

                state.population = empty;
                state.resource_predator = 0.;
            }
        }
		
	 // continue if there are both predator and prey on a cell
        else if (state.population == pred_prey) {
            // checking neighbouring cells if empty for the prey to flee

            // clear container for empty cells
            _empty_cell.clear();

            for (const auto& nb : this->_cm.neighbors_of(cell)) {
                if (nb->state().population == empty) {
                    _empty_cell.push_back(nb);
                }
            }

            // choose a random cell for the movement if there is more than one
            std::uniform_int_distribution<> dist(0, _empty_cell.size() - 1);

	     // the prey has a certain chance to flee
            if (    _empty_cell.size() > 0
                and this->_prob_distr(*this->_rng) < _p_flee)
            {
                auto nb_cell = _empty_cell[dist(*this->_rng)];
                nb_cell->state().population = prey;
                nb_cell->state().resource_prey = state.resource_prey;

                state.population = predator;
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
        if (state.population == pred_prey) {
            state.population = predator;
			
            // increase the resources and clamp to the allowed range [0, e_max]
            state.resource_predator = std::clamp(state.resource_predator 
                                                 + _delta_e_pred, 0., _e_max_pred);
            state.resource_prey = 0.;	
        }
	// preys eat
        else if (state.population == prey) {
            // increase the resources and clamp to the allowed range [0, e_max]
            state.resource_prey = std::clamp(state.resource_prey + _delta_e_prey, 
                                             0., _e_max_prey);
        }
        return state;
    };

    /// Define the reproduction rule
    /** If space is available reproduction with probabilities p_repro_pred or p_repro_prey respecitively
     */
    Rule _repro = [this](const auto& cell) {
        // Get the state of the cell
        auto state = cell->state();
		
        if (   ( state.population == predator || state.population==pred_prey)
            and this->_prob_distr(*this->_rng) < _p_repro_pred
            and state.resource_predator >= _e_min_pred)
        {
            _repro_cell.clear();

            for (const auto& nb : this->_cm.neighbors_of(cell))
            {
                auto nb_state = nb->state();

                if (nb_state.population == prey 
                    || nb_state.population == empty)
                {
                    _repro_cell.push_back(nb);
                }
            }

            if (_repro_cell.size() > 0) {
                // choose a random cell for the offspring to be placed on
                std::uniform_int_distribution<> dist(0, _repro_cell.size()-1);
                auto nb_cell = _repro_cell[dist(*this->_rng)];

                // new state will be predator or pred_prey
                if (nb_cell->state().population == empty)
                {
                    nb_cell->state().population = predator;
                }
                else
                {
                    nb_cell->state().population = pred_prey;
                }                            
                // transfer energy from parent to offspring
                nb_cell->state().resource_predator = _cost_of_repro_pred;
                state.resource_predator -= _cost_of_repro_pred;
            }
        }

        if (    (state.population == prey    ||   state.population ==pred_prey)
                 and this->_prob_distr(*this->_rng) < _p_repro_prey
                 and state.resource_prey >= _e_min_prey)
        {
            _repro_cell.clear();

            for (const auto& nb : this->_cm.neighbors_of(cell))
            {
                auto nb_state = nb->state();

                if (nb_state.population == predator 
                    || nb_state.population == empty)
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
                if (nb_cell->state().population == empty)
                {
                    nb_cell->state().population = prey;
                }
                else
                {
                    nb_cell->state().population = pred_prey;
                }                            
                //  transfer energy from parent to offspring
                nb_cell->state().resource_prey = _cost_of_repro_prey;
                state.resource_prey -= _cost_of_repro_prey;
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
        _cost_of_living_pred(get_as<double>("cost_of_living_pred", _cfg)),
	 _cost_of_living_prey(get_as<double>("cost_of_living_prey", _cfg)),	
        _delta_e_pred(get_as<double>("delta_e_pred", _cfg)),
	 _delta_e_prey(get_as<double>("delta_e_prey",_cfg)),	
        _e_max_pred(get_as<double>("e_max_pred", _cfg)),
	 _e_max_prey(get_as<double>("e_max_prey",_cfg)),	
        _e_min_pred(get_as<double>("e_min_pred", _cfg)),
	 _e_min_prey(get_as<double>("e_min_prey",_cfg)),	
        _cost_of_repro_pred(get_as<double>("cost_of_repro_pred", _cfg)),
	 _cost_of_repro_prey(get_as<double>("cost_of_repro_prey", _cfg)),	
        _p_repro_pred(get_as<double>("p_repro_pred", _cfg)),
	 _p_repro_prey(get_as<double>("p_repro_prey", _cfg)),	
        _p_flee(get_as<double>("p_flee", _cfg)),
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
        if (_cost_of_repro_pred > _e_min_pred || _cost_of_repro_prey >_e_min_prey) {
            throw std::invalid_argument("cost_of_repro needs to be smaller "
                                        "than or equal to e_min!");
        }
        
        // Initialize cells
        this->initialize_cells();
        this->_log->debug("{} model fully set up.", this->_name);

        // Write initial state
        this->write_data();
        this->_log->debug("Initial state written.");
    }


private:
    /// Initialize the cells
    void initialize_cells() {
        // Extract the mode that determines the initialization strategy
        const auto initial_state = get_as<std::string>("initial_state", _cfg);

        this->_log->info("Initializing cells in '{}' mode ...", initial_state);

        // Get the initial resources for predator and prey
        const auto e_init_prey = get_as<double>("e_init_prey", _cfg);
        const auto e_init_pred = get_as<double>("e_init_pred", _cfg);

        // Distinguish by mode
        if (initial_state == "random") {
            // Get the threshold probability value
            const auto prey_prob = get_as<double>("prey_prob", _cfg);
            const auto pred_prob = get_as<double>("pred_prob", _cfg);
            const auto predprey_prob = get_as<double>("predprey_prob", _cfg);

            // check wether the conditions for the probabilities are met
            if (prey_prob < 0. 
                || pred_prob < 0.
                || predprey_prob < 0.
                || (prey_prob + pred_prob + predprey_prob) > 1.)
            {
                throw std::invalid_argument(
                    "Need `prey_prob`, `pred_prob` and `predprey_prob` in "
                    "[0, 1] and the sum not exceeding 1, but got values: " 
                    + std::to_string(prey_prob) + ", "
                    + std::to_string(pred_prob) + " and "
                    + std::to_string(predprey_prob));
            }

            // Define the update rule
            auto set_random_population = [&, this](const auto& cell) {
                // Get the state
                auto& state = cell->state();

                // Draw a random real number in [0., 1.)
                const double rnum = this->_prob_distr(*this->_rng);

                // ... and compare it to the thresholds. Then set the cell
                // state accordingly and distribute initial resources
                if (rnum < prey_prob) {
                    // put prey on the cell
                    state.population = Population::prey;
                    state.resource_prey = e_init_prey;
                    state.resource_predator = 0;
                }
                else if (rnum < (prey_prob + pred_prob)) {
                    // put a predator on the cell
                    state.population = Population::predator;
                    state.resource_predator = e_init_pred;
                    state.resource_prey = 0;
                }
                else if (rnum < (prey_prob + pred_prob + predprey_prob)) {
                    // put a predator and a prey on the cell
                    state.population = Population::pred_prey;
                    state.resource_predator = e_init_pred;
                    state.resource_prey = e_init_prey;
                }
                else {
                    // initialize as empty and without resources
                    state.population = Population::empty;
                    state.resource_predator = 0;
                    state.resource_prey = 0;
                }
                return state;
            };

            // Apply the rule to all cells
            apply_rule<false>(set_random_population, _cm.cells());
        }

        else if (initial_state == "fraction") {
            // Get the fraction of cells to be populated by prey
            const auto prey_frac = get_as<double>("prey_frac", _cfg);

            // Get the fraction of cells to be populated by predators
            const auto pred_frac = get_as<double>("pred_frac", _cfg);

            // Get the fraction of cells to be populated by predators and prey
            // together
            const auto predprey_frac = get_as<double>("predprey_frac", _cfg);

            if (prey_frac < 0 
                || pred_frac < 0 
                || predprey_frac < 0
                || (prey_frac + pred_frac + predprey_frac) > 1)
            {
                throw std::invalid_argument(
                    "Need `prey_frac`, `pred_frac` and `predprey_frac` in "
                    "[0, 1] and the sum not exceeding 1, but got values: " 
                    + std::to_string(prey_frac) + ", "
                    + std::to_string(pred_frac) + " and " 
                    + std::to_string(predprey_frac));
            }

            // Get the cells container
            auto& cells = _cm.cells();

            // Calculate the number of cells that should have that strategy
            const auto num_cells = cells.size();
            const std::size_t num_prey = prey_frac * num_cells;
            const std::size_t num_pred = pred_frac * num_cells;
            const std::size_t num_predprey = predprey_frac * num_cells;
            // NOTE this is a flooring calculation!

            this->_log->debug("Cells with popupation prey, pred and predprey: "
                              "{}, {} and {}",
                              num_prey, num_pred, num_predprey);

            // Need a counter of cells that were set to prey and pred
            std::size_t num_set_prey = 0;
            std::size_t num_set_pred = 0;
            std::size_t num_set_predprey = 0;

            // Get the cells... and shuffle them.
            auto random_cells = _cm.cells();
            std::shuffle(random_cells.begin(), random_cells.end(), *this->_rng);

            // Set the cells accordingly
            for (const auto& cell : random_cells) {
                // If the desired number of cells populated by prey is
                // not yet reached change another cell's strategy
                if (num_set_prey < num_prey)
                {
                    cell->state().population = Population::prey;
                    cell->state().resource_prey = e_init_prey;
                    cell->state().resource_predator = 0;

                    num_set_prey++;
                }
                else if (num_set_pred < num_pred)
                {
                    cell->state().population = Population::predator;
                    cell->state().resource_predator = e_init_pred;
                    cell->state().resource_prey = 0;

                    num_set_pred++;
                }
                else if (num_set_predprey < num_predprey)
                {
                    cell->state().population = Population::pred_prey;
                    cell->state().resource_predator = e_init_pred;
                    cell->state().resource_prey = e_init_prey;

                    num_set_predprey++;
                }
                // Break, if desired fractions have been reached
                else
                    break;
            }
        }

        else {
            throw std::invalid_argument("`initial_state` parameter with value "
                "'" + initial_state + "' is not supported!");
        }

        this->_log->info("Cells successfully initialized.");
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
                if (state.population == prey) {
                    prey_sum++;
                }
                else if (state.population == predator) {
                    pred_sum++;
                }
                else if (state.population == pred_prey) {
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

        // resource of pred
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
