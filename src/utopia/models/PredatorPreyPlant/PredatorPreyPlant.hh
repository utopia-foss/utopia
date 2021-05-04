#ifndef UTOPIA_MODELS_PREDATORPREYPLANT_HH
#define UTOPIA_MODELS_PREDATORPREYPLANT_HH

#include <algorithm>
#include <random>

#include <utopia/core/apply.hh>
#include <utopia/core/model.hh>
#include <utopia/core/cell_manager.hh>

#include <utopia/models/PredatorPrey/PredatorPrey.hh>

#include "species.hh"


namespace Utopia {
namespace Models {
namespace PredatorPreyPlant {

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Cell state, combining states for predator, prey and plant species
struct State {
    /// The state a predator on this cell has
    PredatorPrey::SpeciesState predator;

    /// The state a prey on this cell has
    PredatorPrey::SpeciesState prey;

    /// The state a plant on this cell has
    PlantState plant;

    /// Construct a cell state with the use of a RNG
    template<class RNGType>
    State(const DataIO::Config& cfg, const std::shared_ptr<RNGType>& rng)
    :
        predator{},
        prey{},
        plant{}
    {
        std::uniform_real_distribution<double> dist(0., 1.);

        // Get the threshold probability value
        const auto p_plant = get_as<double>("p_plant", cfg);
        const auto p_prey = get_as<double>("p_prey", cfg);
        const auto p_predator = get_as<double>("p_predator", cfg);

        auto cfg_prey = get_as<DataIO::Config>("prey", cfg);
        auto cfg_predator = get_as<DataIO::Config>("predator", cfg);

        // Check if the max resource limit is actually higher than the lower
        // limit.
        const auto min_init_resources_prey = get_as<int>("min_init_resources", 
            cfg_prey);
        const auto max_init_resources_prey = get_as<int>("max_init_resources", 
            cfg_prey);
        const auto min_init_resources_predator = 
            get_as<int>("min_init_resources", cfg_predator);
        const auto max_init_resources_predator = 
            get_as<int>("max_init_resources", cfg_predator);
        if(max_init_resources_predator < 
            min_init_resources_predator){
            throw::std::invalid_argument("The upper limit for the initial "
            "predator resources needs to be higher than the lower limit.");
        }
        if(max_init_resources_prey < 
            min_init_resources_prey){
            throw::std::invalid_argument("The upper limit for the initial "
            "prey resources needs to be higher than the lower limit.");
        }
        // Set a plant on this cell with a given probability
        plant.on_cell = (dist(*rng) < p_plant);

        // Set a predator on a cell with the given probability and the 
        // resources in the given range.
        if (dist(*rng) < p_predator) {
            predator.on_cell = true;
            std::uniform_int_distribution<> init_res_dist_pred(
                min_init_resources_predator,
                max_init_resources_predator
            );
            predator.resources = init_res_dist_pred(*rng);
        }

        // Set a prey on this cell with the desired probability; also set the
        // initial resource amount from an integer distribution.
        if (dist(*rng) < p_prey) {
            prey.on_cell = true;
            std::uniform_int_distribution<> init_res_dist_prey(
                min_init_resources_prey,
                max_init_resources_prey
            );

            prey.resources = init_res_dist_prey(*rng);
        }


    }
};


/// Cell traits specialization using the state type
/** The first template parameter specifies the type of the cell state,
  * the second sets them to not be synchronously updated.
  * The config constructor for the cell state is preferred here
  */
using CellTraits = Utopia::CellTraits<State, Update::manual>;

/// Typehelper to define data types of PredatorPreyPlant model
using ModelTypes = ModelTypes<>;

// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// PredatorPreyPlant Model on grid cells
/** Predators and prey correspond to the Population state of each cell, either
 * empty, prey, predator or both.
 * Cells are updated based on the following interactions:
 * 1) resource levels are reduced by a cost_of_living for both species
 * and individuals are removed if resource <= 0 
 * 2) predators move to neighbouring cells if there is a no prey on their 
 * own cell. Prey flees with a certain probability, if there is a predator on 
 * the same cell. Prey looks for resources if there are none on its cell.
 * 3) predators eat prey if on the same cell, else if there is only a prey it 
 * takes up resources, when this are present
 * 4) both predators and prey reproduce if resources are sufficient and if
 * there is a cell in their neighbourhood not already occupied by the same
 * species. The plant grows if it is set in mode other than 0.
 */
class PredatorPreyPlant
    : public Model<PredatorPreyPlant, ModelTypes>
{
public:
    /// The base model
    using Base = Model<PredatorPreyPlant, ModelTypes>;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// The type of the cell manager
    using CellManager = Utopia::CellManager<CellTraits, PredatorPreyPlant>;

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
    /// All species-specific parameters
    SpeciesParams _params;

    /// How many cells the movement rule should be applied to each time step
    std::size_t _num_moves;

    // .. Temporary objects ...................................................
    /// A container to temporarily accumulate the prey neighbour cells
    CellContainer<Cell> _prey_cell;
    
    /// A container to temporarily accumulate empty neighbour cells
    CellContainer<Cell> _empty_cell;

    /// A container to temporarily accumulate neighbour cells for reproduction
    CellContainer<Cell> _repro_cell;

    /// A container to temporarily accumulate neighbour cells with mature plants
    CellContainer<Cell> _resource_cell;


    /// Uniform real distribution [0, 1) for evaluating probabilities
    std::uniform_real_distribution<double> _prob_distr;

    /// Distribution for randomly selecting a cell in your cellmanager
    std::uniform_int_distribution<> _cm_dist;


    // .. Datasets ............................................................
    /// Dataset of Prey locations on the grid
    const std::shared_ptr<DataSet> _dset_prey;
    
    /// Dataset of Predator locations on the grid
    const std::shared_ptr<DataSet> _dset_predator;
    
    /// Dataset of Prey resources on the grid
    const std::shared_ptr<DataSet> _dset_resource_prey;
    
    /// Dataset of Predator resources on the grid
    const std::shared_ptr<DataSet> _dset_resource_predator;

    /// Dataset of Plant resources
    const std::shared_ptr<DataSet> _dset_plant;


    // .. Rule functions and helper methods ...................................
    /// Cost of Living
    /** subtract the cost of living from the resources of an individual and 
     * map the values below zero back to zero, then remove all individuals that
     * do not have sufficient resources. 
     * This function applies only to predator and prey, but not to plants.
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
        if (state.predator.on_cell and state.predator.resources <= 0.)
            state.predator.on_cell = false;

        // Remove prey that have no resources.
        if (state.prey.on_cell and state.prey.resources <= 0.) 
            state.prey.on_cell = false;

        return state;
    };

    /// Returns a random neighbor
    /** This method chooses uniform randomly a cell from the 
     *  neighborhood of the given cell.
     */
    std::shared_ptr<Cell>
    get_random_neighbor(const CellContainer<Cell>& nbs) const
    {
        return nbs[std::uniform_int_distribution<std::size_t>(0, nbs.size() - 1)(*this->_rng)];
    }

    /// Move a predator to a neighboring cell
    /** This function resets the states predator state and updates the
     *  neighboring predator state.
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
    /** This function resets the states prey state and updates the
     *  neighboring prey state.
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


    /// Move the prey looking for resources
    /** If plants are enabled, prey might need to look for available resources 
     *  in the form of plants in the neighboring cells. If there is a plant on
     *  a neighboring cell and no predator, the prey will move there. 
     *  If there are multiple cells with plants on them, one is selected at
     *  random. If there are no free cells just take your chance and randomly
     *  select a cell, if there is a predator on it nevertheless move to it.
     */
    auto move_prey(std::shared_ptr<Cell> cell) {
        _resource_cell.clear();

        // Collect empty neighboring cells with available resources, where the
        // prey could move
        for (const auto& nb : this->_cm.neighbors_of(cell)) {
            if (nb->state.plant.on_cell 
                and not nb->state.prey.on_cell 
                and not nb->state.predator.on_cell)
                _resource_cell.push_back(nb);
        }

        // Choose a random cell with resources.
        if (_resource_cell.size() > 0) {
            std::uniform_int_distribution<> dist(0, _resource_cell.size() - 1);
            const auto nb_cell = _resource_cell[dist(*this->_rng)];
            move_prey_to_nb_cell(cell, nb_cell);
            return nb_cell;
        }
        // if there isn't one, then try a random walk
        else {
            const std::shared_ptr<Cell> nb_cell = get_random_neighbor(_cm.neighbors_of(cell));
            // The will move even if there is a predator.
            if (not nb_cell->state.prey.on_cell) {
                move_prey_to_nb_cell(cell, nb_cell);
                return nb_cell;
            }
            return cell;
        }
    };


    /// Move the predator looking for preys
    /** Predators looks for prey in the neighborhood. If there are multiple 
     *  cells with prey on them randomly choose one cell to move to. 
     *  If there is no cell with prey just do a movement towards a randomly 
     *  selected neighboring cell.
     *  A predator can never move to a cell with another predator on it.
     */
    auto move_predator(std::shared_ptr<Cell> cell) {
        // clear the container for cells that contain prey or empty cells
        // in the neighbourhood
        _prey_cell.clear();

        // Collect neighboring cells with preys and without predator
        for (const auto& nb : this->_cm.neighbors_of(cell)) {
            if (    nb->state.prey.on_cell
                and not nb->state.predator.on_cell)
                _prey_cell.push_back(nb);
        }

        // now update the cell state and the respective neighbor
        if (_prey_cell.size() > 0) {
            // distribution to choose a random cell for the movement
            std::uniform_int_distribution<> dist_prey(0 ,_prey_cell.size() - 1);
            auto nb_cell = _prey_cell[dist_prey(*this->_rng)];
            // move the predator to the given cell
            move_predator_to_nb_cell(cell, nb_cell);

            return nb_cell;
        }
        else {
            // If no prey in the neighborhoods try a random step if possible
            const std::shared_ptr<Cell> nb_cell = get_random_neighbor(_cm.neighbors_of(cell));
            if (not nb_cell->state.predator.on_cell){
                // move the predator to the given cell
                move_predator_to_nb_cell(cell, nb_cell);
                return nb_cell;
            }
        }
        return cell;
    };


    /// Define the movement rule of an individual
    /*+ Go through all cells. If there are both predator and prey on the cell,
    *   then the prey attempts to flee with a given probability.
    *   If there is only a predator on the cell then it moves until it reaches 
    *   the movement limit or reaches a prey. 
    *   If there is only a prey on the cell and there are no plant resources, 
    *   then the prey moves until it reaches the movement limit or it finds a 
    *   cell with plant resource on it.
    */
    void move_entities(std::shared_ptr<Cell> cell) {
        auto& state = cell->state;

        // Marker used to compute how many steps the prey or predator has taken
        unsigned int step = 0;

        if (state.predator.on_cell) {
            while (    step++ < _params.predator.move_limit
                   and not cell->state.prey.on_cell)
            {
                cell = move_predator(cell);
            }
        }
        else if (state.prey.on_cell and not cell->state.plant.on_cell){
            while(step++ < _params.prey.move_limit and not cell->state.plant.on_cell)
                cell = move_prey(cell);
        }
    };



    /// If a prey is on the cell, determine whether it may flee and where to
    /** A prey should only flee if there is a predator on the cell, too.
     *  First empty neighboring cells are collected to which a prey can 
     *  potentially flee. Choose a random cell out of the selection to flee to.
     *  If there is no empty cell nothing happens.
     */
    Rule _flee_prey = [this](const auto& cell) {
        auto& state = cell->state;
        
        if (state.prey.on_cell and state.predator.on_cell and 
            (_prob_distr(*this->_rng) < _params.prey.p_flee)) {
            // Collect empty neighboring cells to which the prey could flee
            _empty_cell.clear();
            for (const auto& nb : this->_cm.neighbors_of(cell)) {
                if (    not nb->state.prey.on_cell
                    and not nb->state.predator.on_cell)
                    _empty_cell.push_back(nb);
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
        return state;
    };



    /// Define the eating rule
    /** Update procedure is as follows:
     * - prey is consumed by a predator if they are on the same cell. 
     *   The predator increases its resources.
     * - prey eats plants which increases its resources; if the plant growth 
     *   model is not none the plant is removed from the cell.
     */
    Rule _eat = [this](const auto& cell) {
        auto& state = cell->state;

        // Predator eats prey
        if (state.predator.on_cell and state.prey.on_cell) {
            // Increment resources and clamp to [0, resource_max]
            state.predator.resources =
                std::clamp(  state.predator.resources 
                           + _params.predator.resource_intake, 
                           0., _params.predator.resource_max);

            // Remove the prey from the cell
            state.prey.on_cell = false;
            state.prey.resources = 0.;
        }

        // Prey eats plants
        else if (state.prey.on_cell and state.plant.on_cell) {
            // Increment resources and clamp to [0, resource_max]
            state.prey.resources =
                std::clamp(  state.prey.resources 
                           + _params.prey.resource_intake, 
                           0., _params.prey.resource_max);
            
            if (_params.plant.growth_model != GrowthModel::none) {
                state.plant.on_cell = false;
                state.plant.regeneration_counter = 0;
            }
        }
        return state;
    };


    /// Define the reproduction rule
    /** If a neighboring cell if not already occupied by an individual of the 
     *  same species, individuals reproduce with reproduction probabilities
     *  of predator and prey respectively.
     *  Plants reproduce according to their GrowthMode.
     */
    Rule _reproduce = [this](const auto& cell) {
        auto& state = cell->state;
        
        // Reproduction of predators    
        if (    state.predator.on_cell
            and (this->_prob_distr(*this->_rng) < _params.predator.repro_prob)
            and (   state.predator.resources
                 >= _params.predator.repro_resource_requ))
        {
            const std::shared_ptr<Cell> nb_cell = get_random_neighbor(_cm.neighbors_of(cell));
            if (not nb_cell->state.predator.on_cell) {
                nb_cell->state.predator.on_cell = true;

                // transfer resources from parent to offspring
                nb_cell->state.predator.resources = _params.predator.repro_cost;
                state.predator.resources -= _params.predator.repro_cost; 
            }
        }

        // Reproduction of preys
        if (    state.prey.on_cell
            and this->_prob_distr(*this->_rng) < _params.prey.repro_prob
            and state.prey.resources >= _params.prey.repro_resource_requ)
        {
            const std::shared_ptr<Cell> nb_cell = get_random_neighbor(_cm.neighbors_of(cell));
            if (not nb_cell->state.prey.on_cell) {
                nb_cell->state.prey.on_cell = true;

                // transfer resources from parent to offspring
                nb_cell->state.prey.resources = _params.prey.repro_cost;
                state.prey.resources -= _params.prey.repro_cost;
            }
        }

        // If there is no plant, there _may_ be plant growth, depending on the
        // plant growth model
        if (not state.plant.on_cell) {
            // TODO Ideally, the model is decided once in the beginning, and
            //      not for each cell anew.
            if (_params.plant.growth_model == GrowthModel::deterministic) {
                // If the regeneration counter if not high enough, increment it
                if (   state.plant.regeneration_counter
                    >= _params.plant.regen_time)
                {
                    // Grow a plant :)
                    state.plant.on_cell = true;
                }
                else {
                    state.plant.regeneration_counter++;
                }
            }
            else if (_params.plant.growth_model == GrowthModel::stochastic) {
                // Regrow with a certain probability
                if (_prob_distr(*this->_rng) < _params.plant.regen_prob) {
                    state.plant.on_cell = true;
                }
            }
            // else: no regrowth
        }

        return state;
    };


public:
    // -- Model setup ---------------------------------------------------------
    /// Construct the PredatorPreyPlant model
    /** \param name             Name of this model instance; is used to extract
     *                          the configuration from the parent model and
     *                          set up a HDFGroup for this instance
     *  \param parent_model     The parent model this model instance resides in
     *  \param custom_cfg       A custom configuration to use instead of the
     *                          one extracted from the parent model using the
     *                          instance name
     */
    template <class ParentModel>
    PredatorPreyPlant (
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
        _num_moves([&](){
            const auto f = get_as<double>("num_moves_fraction", this->_cfg);
            return f * this->_cm.cells().size();
        }()),

        // Temporary cell containers
        _prey_cell(),
        _empty_cell(),
        _repro_cell(),

        // create random distributions
        _prob_distr(0., 1.),
        _cm_dist(0, _cm.cells().size() - 1),

        // create datasets
        _dset_prey(this->create_cm_dset("prey", _cm)),
        _dset_predator(this->create_cm_dset("predator", _cm)),
        _dset_resource_prey(this->create_cm_dset("resource_prey", _cm)),
        _dset_resource_predator(this->create_cm_dset("resource_predator", _cm)),
        _dset_plant(this->create_cm_dset("plant", _cm))
    {
        // Inform about number of movements per iteration step:
        this->_log->info("The movement rule will be applied {} times each "
                         "time step.", _num_moves);

        // Load the cell state from a file, overwriting the current state
        if (this->_cfg["cell_states_from_file"]) {
            setup_cell_states_from_file(this->_cfg["cell_states_from_file"]);
        }

        // Reserve memory in the size of the neighborhood for the temp. vectors
        const auto nb_size = _cm.nb_size();
        _prey_cell.reserve(nb_size);
        _empty_cell.reserve(nb_size);
        _repro_cell.reserve(nb_size);

        // Initialization finished
        this->_log->info("{} model fully set up.", this->_name);
    }

private:
    // .. Setup functions .....................................................

    /// Sets predator, prey, and plant positions from loaded HDF5 data
    void setup_cell_states_from_file(const Config& cs_cfg) {
        const auto hdf5_file = get_as<std::string>("hdf5_file", cs_cfg);

        if (get_as<bool>("load_predator", cs_cfg)) {
            this->_log->info("Loading predator positions from file ...");

            // Use the CellManager to set the cell state from the data
            // given by the `predator` dataset. Load as int to be able to
            // detect that a user supplied invalid values (better than
            // failing silently, which would happen with booleans).
            _cm.set_cell_states<int>(hdf5_file, "predator",
                [this](auto& cell, const int on_cell){
                    if (on_cell == 0 or on_cell == 1) {
                        // Place predator state on cell.
                        cell->state.predator.on_cell = on_cell;
                        // Take care of species resources.
                        if(on_cell){
                            const auto& predator_cfg = 
                                get_as<Config>("predator", 
                                _cfg["cell_manager"]["cell_params"]);
                            int min_init_resources_predator = 
                                get_as<int>("min_init_resources", 
                                predator_cfg);
                            int max_init_resources_predator = 
                                get_as<int>("max_init_resources", 
                                predator_cfg);
                            std::uniform_int_distribution<> 
                                init_res_dist_predator(
                                min_init_resources_predator,
                                max_init_resources_predator
                            );
                            cell->state.predator.resources = 
                                init_res_dist_predator(*this->_rng);
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
                [this](auto& cell, const int on_cell){
                    if (on_cell == 0 or on_cell == 1) {
                        // Place prey state on cell.
                        cell->state.prey.on_cell = on_cell;
                        // Take care of species resources.
                        if(on_cell){
                            const auto& prey_cfg = 
                                get_as<Config>("prey", 
                                _cfg["cell_manager"]["cell_params"]);
                            int min_init_resources_prey = 
                                get_as<int>("min_init_resources", prey_cfg);
                            int max_init_resources_prey = 
                                get_as<int>("max_init_resources", prey_cfg);
                            std::uniform_int_distribution<> 
                                init_res_dist_prey(
                                min_init_resources_prey,
                                max_init_resources_prey
                            );
                            cell->state.prey.resources = 
                                init_res_dist_prey(*this->_rng);
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

        if (get_as<bool>("load_plant", cs_cfg)) {
            this->_log->info("Loading plant positions from file ...");

            _cm.set_cell_states<int>(hdf5_file, "plant",
                [](auto& cell, const int on_cell){
                    if (on_cell == 0 or on_cell == 1) {
                        cell->state.plant.on_cell = on_cell;
                        return;
                    }
                    throw std::invalid_argument("While setting plant "
                        "positions, encountered an invalid value: "
                        + std::to_string(on_cell) + ". Allowed: 0 or 1.");
                }
            );
            this->_log->info("Plant positions loaded.");
        }
    }

public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Perform an iteration step
    /** An iteration step consists of:
     *     1. Subtracting costs of living
     *     2. Let predator and prey move
     *     3. Lunch time: Prey eats grass and predator eats prey if on the
     *        same cell
     *     4. Reproduction: Create offspring and grow plant
     */
    void perform_step() {
        apply_rule<Update::async, Shuffle::off>(_cost_of_living, _cm.cells());

        // Choose _num_moves cells randomly and apply the movement rule to them
        // NOTE Be aware that cells can be selected multiple times.
        for (std::size_t i = 0; i < _num_moves; ++i) {
            move_entities(_cm.cells()[_cm_dist(*this->_rng)]);
        }
        apply_rule<Update::async, Shuffle::on>(_flee_prey, _cm.cells(), 
                                                *this->_rng);
        apply_rule<Update::async, Shuffle::off>(_eat, _cm.cells());
        apply_rule<Update::async, Shuffle::on>(_reproduce, _cm.cells(),
                                               *this->_rng);
    }

    /// Monitor model information
    void monitor () {
        // Calculate the densities for all species
        auto [pred_density, prey_density, plant_density] = [this](){
            double predator_sum = 0.;
            double prey_sum = 0.;
            double plant_sum = 0.;
            double num_cells = this->_cm.cells().size();

            for (const auto& cell : this->_cm.cells()) {
                auto state = cell->state;

                if (state.prey.on_cell) prey_sum++;
                if (state.predator.on_cell) predator_sum++;
                if (state.plant.on_cell) plant_sum++;
            }
            return std::tuple{predator_sum / num_cells,
                              prey_sum / num_cells,
                              plant_sum /  num_cells};
        }();

        this->_monitor.set_entry("predator_density", pred_density);
        this->_monitor.set_entry("prey_density", prey_density);
        this->_monitor.set_entry("plant_density", plant_density);
    }

    /// Write data
    /** When invoked, stores cell positions and resources of both prey
      * and predators.
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

        // Plant
        _dset_plant->write(_cm.cells().begin(), _cm.cells().end(), 
            [](const auto& cell) {
                return static_cast<char>(cell->state.plant.on_cell);
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

} // namespace PredatorPreyPlant
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_PREDATORPREYPLANT_HH
