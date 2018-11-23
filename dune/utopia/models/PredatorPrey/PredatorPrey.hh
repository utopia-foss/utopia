#ifndef UTOPIA_MODELS_PREDATORPREY_HH
#define UTOPIA_MODELS_PREDATORPREY_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/apply.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/types.hh>

#include <algorithm>
#include <functional>

namespace Utopia
{
namespace Models
{
namespace PredatorPrey
{
/// Population enum, changing the numeration will lead to failing tests/plots
enum Population : unsigned short
{
    empty = 0,
    prey = 1,
    predator = 2,
    pred_prey = 3
};

/// State struct
struct State
{
    Population population;
    double resource_predator;
    double resource_prey;
};

// Alias the neighborhood classes
using NextNeighbor = Utopia::Neighborhoods::NextNeighbor;
using MooreNeighbor = Utopia::Neighborhoods::MooreNeighbor;

/// Typehelper to define data types of PredatorPrey model
using PredatorPreyModelTypes = ModelTypes<>;

/// PredatorPrey Model on grid cells
/** Predators and prey correspond to the Population state of each cell, either
 * empty, prey, predator or both.
 * Cells are updated based on the following interactions:
 * 1) resource levels are reduced by 1 for both species and individuals
 * removed if resource = 0 (_cost_of_living)
 * 2) predators move to neighbouring cells if there is a no prey on their own
 * cell, prey flees with a certain probability
 * if there is a predator on the same cell
 * 3) prey takes up resources, predators eat prey if on the same cell
 * 4) both predators and prey reproduce if resources are sufficient and if
 * there is a cell in their neighbourhood not already occupied by the same
 * species
 */
template <class ManagerType>
class PredatorPreyModel
    : public Model<PredatorPreyModel<ManagerType>, PredatorPreyModelTypes>
{
public:
    /// The base model
    using Base = Model<PredatorPreyModel<ManagerType>, PredatorPreyModelTypes>;

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

    /// Type of the update rules
    using Rule = std::function<State(std::shared_ptr<CellType>)>;

private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor

    // -- Members of this model -- //
    /// The grid manager
    ManagerType _manager;

    /// The model parameters
    // The cost of living
    double _cost_of_living;

    // The resource uptake of an idividual
    double _delta_e;

    // The maximum of resources an individual can carry
    double _e_max;

    // The minimum resource level necessary for reproduction
    double _e_min;

    // The cost of reproduction, i.e. the resources transferred to the offspring
    double _cost_of_repro;

    // The probability to reproduce
    double _p_repro;

    // The probability for prey to flee
    double _p_flee;




    // -- Temporary objects -- //
    /// A container to temporarily accumulate the neighbour cells that are
    // occupied by prey, empty or not occupied by the same species
    CellContainer<typename ManagerType::Cell> _prey_cell;
    CellContainer<typename ManagerType::Cell> _empty_cell;
    CellContainer<typename ManagerType::Cell> _repro_cell;

    // -- Datasets -- //
    std::shared_ptr<DataSet> _dset_population;
    std::shared_ptr<DataSet> _dset_resource_prey;
    std::shared_ptr<DataSet> _dset_resource_pred;

    // -- uniform real distribution [0, 1) for drawing of random numbers
    std::uniform_real_distribution<> _rand;

    // -- Rule functions -- //

    /// Cost of Living
    /** subtract the cost of living from the resources of an individual and 
     * map the values below zero back to zero, then remove all individuals that
     * do not have sufficient resources 
     */
    Rule _cost = [this](const auto cell) {
        // Get the state of the cell
        auto state = cell->state();

        state.resource_predator = std::clamp(state.resource_predator 
                                             - _cost_of_living, 0.0, _e_max);
        state.resource_prey = std::clamp(state.resource_prey 
                                         - _cost_of_living, 0.0, _e_max);

        // Check for the predators and preys that have no resources and remove
        // them
        if (state.population == predator && state.resource_predator == 0.0)
        {
            // Remove the predator if present
            state.population = empty;
        }
        else if (state.population == prey && state.resource_prey == 0)
        {
            // Remove the prey
            state.population = empty;
        }

        else if (state.population == pred_prey)
        {
            if (state.resource_predator == 0.0 && state.resource_prey == 0.0)
                state.population = empty;
            else if (state.resource_predator == 0.0)
                state.population = prey;
            else if (state.resource_prey == 0.0)
                state.population = predator;
        }

        return state;
    };

    /// Define the movement rule of an individual
    /*+ Go through cells, if only a predator populates it, look for prey in
     * the neighbourhood and move to that cell or go to an empty cell if no
     * prey is found. If both predator and prey live on the same cell, the
     * prey flees with a certain probability 
     */
    Rule _move = [this](const auto cell) {
        // Get the state of the Cell
        auto state = cell->state();

        if (state.population == predator)
        {
            // checking neighbouring cells for prey and moving to that cell

            // clear the containers for cells that contain prey or empty cells
            // in the neighbourhood
            _prey_cell.clear();
            _empty_cell.clear();

            for (const auto& nb : MooreNeighbor::neighbors(cell, this->_manager))
            {
                auto nb_state = nb->state();

                if (nb_state.population == prey)
                {
                    _prey_cell.push_back(nb);
                }

                else if (nb_state.population == empty)
                {
                    _empty_cell.push_back(nb);
                }
            }

            // now update the cell state and the respective neighbor
            if (_prey_cell.size() > 0)
            {
                // distribution to choose a random cell for the movement if 
                // there is more than one
                std::uniform_int_distribution<> dist_prey(0, 
                                                         _prey_cell.size() - 1);

                auto nb_cell = _prey_cell[dist_prey(*this->_rng)];
                nb_cell->state().population = pred_prey;
                nb_cell->state().resource_predator = state.resource_predator;
                    
                state.population = empty;
                state.resource_predator = 0.0;
            }

            else if (_empty_cell.size() > 0)
            {   
                // distribution to choose a random cell for the movement if 
                // there is more than one
                std::uniform_int_distribution<> dist_empty(0, 
                                                        _empty_cell.size() - 1);


                auto nb_cell = _empty_cell[dist_empty(*this->_rng)];
                nb_cell->state().population = predator;
                nb_cell->state().resource_predator = state.resource_predator;

                state.population = empty;
                state.resource_predator = 0.0;
            }
        }

        else if (state.population == pred_prey)
        {
            // checking neighbouring cells if empty for the prey to flee

            // clear container for empty cells
            _empty_cell.clear();

            for (const auto& nb : MooreNeighbor::neighbors(cell, this->_manager))
            {
                if (nb->state().population == empty)
                {
                    _empty_cell.push_back(nb);
                }
            }

            // choose a random cell for the movement if there is more than one
            std::uniform_int_distribution<> dist(0, _empty_cell.size() - 1);

            if (_empty_cell.size() > 0 && _rand(*this->_rng) < _p_flee)
            {
                auto nb_cell = _empty_cell[dist(*this->_rng)];
                nb_cell->state().population = prey;
                nb_cell->state().resource_prey = state.resource_prey;

                state.population = predator;
                state.resource_prey = 0.0;
            }
        }
        return state;
    };

    /// Define the eating rule
    /** Update procedure is as follows:
     * prey is consumed if predator and prey on the same cell
     * prey resource is increased if there is just prey on the cell
     */
    Rule _eat = [this](const auto cell) {
        // Get the state of the cell
        auto state = cell->state();

        // Eating: preys get eaten by predators and preys eat
        if (state.population == pred_prey)
        {
            state.population = predator;
            // increase the resources and clamp to the allowed range [0, e_max]
            state.resource_predator = std::clamp(state.resource_predator 
                                                 + _delta_e, 0.0, _e_max);
            state.resource_prey = 0.0;
        }
        else if (state.population == prey)
        {
            // increase the resources and clamp to the allowed range [0, e_max]
            state.resource_prey = std::clamp(state.resource_prey + _delta_e, 
                                             0.0, _e_max);
        }
        return state;
    };

    /// Define the reproduction rule
    /** Reproduction with probability p_repro if empty space available 
     */
    Rule _repro = [this](const auto cell) {
        // Get the state of the cell
        auto state = cell->state();

        if (state.population == predator
             && _rand(*this->_rng) < _p_repro
             && state.resource_predator >= _e_min)
        {
            _repro_cell.clear();

            for (const auto& nb : MooreNeighbor::neighbors(cell, this->_manager))
            {
                auto nb_state = nb->state();

                if (nb_state.population == prey 
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

                // new state will be predator or pred_prey
                if (nb_cell->state().population == empty)
                {
                    nb_cell->state().population = predator;
                }
                else
                {
                    nb_cell->state().population = pred_prey;
                }                            
                // give 2 units to offspring
                nb_cell->state().resource_predator = _cost_of_repro;

                // deduct cost of reproduction
                state.resource_predator -= _cost_of_repro;
            }
        }

        else if (state.population == prey
                 && _rand(*this->_rng) < _p_repro
                 && state.resource_prey >= _e_min)
        {
            _repro_cell.clear();

            for (const auto& nb : MooreNeighbor::neighbors(cell, this->_manager))
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
                // give 2 units to offspring
                nb_cell->state().resource_prey = _cost_of_repro;

                // deduct cost of reproduction
                state.resource_prey -= _cost_of_repro;
            }
        }

        return state;
    };

public:
    /// Construct the PredatorPrey model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template <class ParentModel>
    PredatorPreyModel(const std::string name,
                      ParentModel& parent,
                      ManagerType&& manager)
        : // Initialize first via base model
          Base(name, parent),
          // Now initialize members specific to this class
          _manager(manager),
          // the model parameters
          _cost_of_living(as_double(this->_cfg["cost_of_living"])),
          _delta_e(as_double(this->_cfg["delta_e"])),
          _e_max(as_double(this->_cfg["e_max"])),
          _e_min(as_double(this->_cfg["e_min"])),
          _cost_of_repro(as_double(this->_cfg["cost_of_repro"])),
          _p_repro(as_double(this->_cfg["p_repro"])),
          _p_flee(as_double(this->_cfg["p_flee"])),
          // temporary cell containers
          _prey_cell(),
          _empty_cell(),
          _repro_cell(),
          // uniform real distribution
          _rand(0, 1),
          // datasets
          _dset_population(this->_hdfgrp->open_dataset("population")),
          _dset_resource_prey(this->_hdfgrp->open_dataset("resource_prey")),
          _dset_resource_pred(this->_hdfgrp->open_dataset("resource_predator"))

    {
        // Check if _cost_of_repro is in the allowed range
        if (_cost_of_repro > _e_min)
            throw std::invalid_argument("cost_of_repro needs to be " 
                                       "smaller than or equal to e_min");
        // Initialize cells
        this->initialize_cells();
        // Set dataset capacities
        // We already know the maximum number of steps and the number of cells
        const hsize_t num_cells =
            std::distance(_manager.cells().begin(), _manager.cells().end());
        this->_log->debug("Setting dataset capacities to {} x {} ...",
                          this->get_time_max() + 1, num_cells);
        _dset_population->set_capacity({this->get_time_max() + 1, num_cells});
        _dset_resource_prey->set_capacity({this->get_time_max() + 1,
                                           num_cells});
        _dset_resource_pred->set_capacity({this->get_time_max() + 1, 
                                           num_cells});
        // Write initial state
        this->write_data();

        // Create
    }

    // Setup functions ........................................................
    /// Initialize the cells randomly
    void initialize_cells()
    {
        // Extract the mode that determines the initial strategy
        auto initial_state = as_str(this->_cfg["initial_state"]);

        // Get the initial resources for predator and prey
        const auto init_res_prey = as_double(this->_cfg["init_res_prey"]);
        const auto init_res_predator = as_double(this->_cfg["init_res_pred"]);

        this->_log->info("Initializing cells in '{}' mode ...", initial_state);

        if (initial_state == "random")
        {
            // Get the threshold probability value
            const auto prey_prob = as_double(this->_cfg["prey_prob"]);
            const auto pred_prob = as_double(this->_cfg["pred_prob"]);
            const auto predprey_prob = as_double(this->_cfg["predprey_prob"]);

            // check wether the conditions for the probabilities are met
            if (prey_prob < 0 
                || pred_prob < 0 
                || predprey_prob < 0
                || (prey_prob + pred_prob + predprey_prob) > 1)
            {
                throw std::invalid_argument(
                    "Need `prey_prob`, `pred_prob` and `predprey_prob` in "
                    "[0, 1] and the sum not exceeding 1, but got values: " 
                    + std::to_string(prey_prob) + ", "
                    + std::to_string(pred_prob) + " and "
                    + std::to_string(predprey_prob));
            }

            // Use a uniform real distribution for random numbers
            auto rand =
                std::bind(std::uniform_real_distribution<>(), 
                          std::ref(*this->_rng));

            // Define the update rule
            auto set_random_population = [&rand, &prey_prob, &pred_prob, 
                                          &predprey_prob, &init_res_prey, 
                                          &init_res_predator]
                                         (const auto cell) {
                // Get the state
                auto state = cell->state();

                // Draw a random number and compare it to the thresholds
                const double random_number = rand();

                if (random_number < prey_prob)
                {
                    // put prey on the cell and give 2 resource units to it
                    state.population = Population::prey;
                    state.resource_prey = init_res_prey;
                    state.resource_predator = 0;
                }
                else if (random_number < (prey_prob + pred_prob))
                {
                    // put a predator on the cell and give it 2 resource units
                    state.population = Population::predator;
                    state.resource_predator = init_res_predator;
                    state.resource_prey = 0;
                }
                else if (random_number < prey_prob + pred_prob + predprey_prob)
                {
                    // put a predator and a prey on the cell and give each 2 
                    // resource units
                    state.population = Population::pred_prey;
                    state.resource_predator = init_res_predator;
                    state.resource_prey = init_res_prey;
                }
                else
                {
                    // initialize as empty
                    state.population = Population::empty;
                    state.resource_predator = 0;
                    state.resource_prey = 0;
                }
                return state;
            };

            // Apply the rule to all cells
            apply_rule<false>(set_random_population, _manager.cells());
        }
        else if (initial_state == "fraction")
        {
            // Get the fraction of cells to be populated by prey
            const auto prey_frac = as_double(this->_cfg["prey_frac"]);

            // Get the fraction of cells to be populated by predators
            const auto pred_frac = as_double(this->_cfg["pred_frac"]);

            // Get the fraction of cells to be populated by predators and prey
            // together
            const auto predprey_frac = as_double(this->_cfg["predprey_frac"]);

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
            auto& cells = _manager.cells();

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
            auto random_cells = _manager.cells();
            std::shuffle(random_cells.begin(), random_cells.end(), *this->_rng);

            // Set the cells accordingly
            for (const auto& cell : random_cells)
            {
                // If the desired number of cells populated by prey is
                // not yet reached change another cell's strategy
                if (num_set_prey < num_prey)
                {
                    cell->state().population = Population::prey;
                    cell->state().resource_prey = init_res_prey;
                    cell->state().resource_predator = 0;

                    num_set_prey++;
                }
                else if (num_set_pred < num_pred)
                {
                    cell->state().population = Population::predator;
                    cell->state().resource_predator = init_res_predator;
                    cell->state().resource_prey = 0;

                    num_set_pred++;
                }
                else if (num_set_predprey < num_predprey)
                {
                    cell->state().population = Population::pred_prey;
                    cell->state().resource_predator = init_res_predator;
                    cell->state().resource_prey = init_res_prey;

                    num_set_predprey++;
                }
                // Break, if fraction of strategy 1 is reached
                else
                    break;
            }
        }
        else
        {
            throw std::invalid_argument(
                "`initial_state` parameter with value "
                "'" +
                initial_state +
                "' is not "
                "supported!");
        }

        this->_log->info("Cells initialized.");
    }

    // Runtime functions ......................................................

    void perform_step()
    {
        // Apply the rules to all cells

        // cost of living is subducted and individuals are removed if
        // resources are 0
        apply_rule<false>(_cost, _manager.cells());

        // predators hunt and prey flees
        apply_rule(_move, _manager.cells(), *this->_rng);

        // uptake of resources, prey gets eaten
        apply_rule<false>(_eat, _manager.cells());

        // reproduction
        apply_rule(_repro, _manager.cells(), *this->_rng);
    }

    /// Monitor model information
    void monitor ()
    {

    }

    /// Write data
    void write_data()
    {
        // For the grid data, get the cells in order to iterate over them
        auto cells = _manager.cells();

        // Population
        _dset_population->write(cells.begin(), cells.end(), [](auto& cell) {
            return static_cast<unsigned short>(cell->state().population);
        });

        // resource of prey
        _dset_resource_prey->write(cells.begin(), cells.end(), [](auto& cell) {
            return cell->state().resource_prey;
        });

        // resource of pred

        _dset_resource_pred->write(cells.begin(), cells.end(), [](auto& cell) {
            return cell->state().resource_predator;
        });
    }
};

} // namespace PredatorPrey
} // namespace Models
} // namespace Utopia
#endif // UTOPIA_MODELS_PREDATORPREY_HH
