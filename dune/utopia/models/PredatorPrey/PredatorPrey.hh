#ifndef UTOPIA_MODELS_PREDATORPREY_HH
#define UTOPIA_MODELS_PREDATORPREY_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>
#include <dune/utopia/core/types.hh>

#include <functional>
#include <limits.h>
#include <algorithm>

namespace Utopia {
namespace Models {
namespace PredatorPrey {

/// Population enum
enum Population : unsigned short int { empty=0, prey=1, predator=2, pred_prey=3 };

/// State struct
struct State {
    Population population;
    unsigned short int resource_pred;
    unsigned short int resource_prey;
};


/// Boundary condition type
struct Boundary {};
// TODO do we need this?


// Alias the neighborhood classes
using NextNeighbor = Utopia::Neighborhoods::NextNeighbor;
using MooreNeighbor = Utopia::Neighborhoods::MooreNeighbor;


/// Typehelper to define data types of PredatorPrey model 
using PredatorPreyModelTypes = ModelTypes<State, Boundary>;


/// PredatorPrey Model on grid cells
/** Predators and prey correspond to the Population state of each cell, either empty, prey, predator or both. 
 * Cells are updated based on the following interactions:
 * 1) resource levels are reduced by 1 for both species and individuals removed if resource = 0 (_cost_of_living)
 * 2) predators move to neighbouring cells if there is a no prey on their own cell, prey flees with a certain probability
 * if there is a predator on the same cell
 * 3) prey takes up resources, predators eat prey if on the same cell
 * 4) both predators and prey reproduce if resources are sufficient and if there is a cell in their neighbourhood not already occupied
 * by the same species
 */
template<class ManagerType>
class PredatorPreyModel:
    public Model<PredatorPreyModel<ManagerType>, PredatorPreyModelTypes>
{
public:
    /// The base model
    using Base = Model<PredatorPreyModel<ManagerType>, PredatorPreyModelTypes>;
    
    /// Data type of the state
    using Data = typename Base::Data;
    
    /// Cell type
    using CellType = typename ManagerType::Cell;

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

    // -- Members of this model -- //
    /// The grid manager
    ManagerType _manager;


    // -- Temporary objects -- //
    /// A container to temporarily accumulate the neighbour cells that are occupied by prey, empty or not occupied by the same species
    CellContainer<typename ManagerType::Cell> _prey_cell_in_nbhood;
    CellContainer<typename ManagerType::Cell> _empty_cell_in_nbhood;
    CellContainer<typename ManagerType::Cell> _repro_cell_in_nbhood;
    
    // -- Datasets -- //
    std::shared_ptr<DataSet> _dset_population;
    std::shared_ptr<DataSet> _dset_resource_prey;
    std::shared_ptr<DataSet> _dset_resource_pred;


    // -- Rule functions -- //

    /// Cost of Living
    std::function<State(std::shared_ptr<CellType>)> _cost = [this](const auto cell){
        // Get the state of the cell
        auto state = cell->state();
        
        // check for the predators and preys that would die and remove them 
        if (state.resource_pred == 1) {
            state.population = static_cast<Population>(state.population - 2);
        }

        if (state.resource_prey == 1) {
                state.population = static_cast<Population>(state.population - 1);
            }

        // subtract the cost of living from the resources 
        state.resource_pred = std::clamp(state.resource_pred - 1, 0, USHRT_MAX);
        state.resource_prey = std::clamp(state.resource_prey - 1, 0, USHRT_MAX);
        return state;
    };

    /// Define the movement of individuals
    std::function<State(std::shared_ptr<CellType>)> _move = [this](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();


        // Go through cells, if only a predator populates it, look for prey in the neighbourhood and move to that cell
        // or go to an empty cell if no prey is found. If both predator and prey live on the same cell, the prey flees
        // with a certain probability
   
        if (state.population == predator) {
            // checking neighbouring cells for prey and moving to that cell

            // clear the containers for cells that contain prey or empty cells in the neighbourhood
            _prey_cell_in_nbhood.clear();
            _empty_cell_in_nbhood.clear();

            for (auto nb : MooreNeighbor::neighbors(cell, this->_manager))
            {
                auto nb_state = nb->state();


                if (nb_state.population == prey) {
                     _prey_cell_in_nbhood.push_back(nb);

                }

                else if (nb_state.population == empty) {
                    _empty_cell_in_nbhood.push_back(nb);

                }
            }
            // distributions to choose a random cell for the movement if there is more than one
            std::uniform_int_distribution<> dist_prey(0, _prey_cell_in_nbhood.size()-1);
            std::uniform_int_distribution<> dist_empty(0, _empty_cell_in_nbhood.size()-1);
            


            // now update the cell state and the respective neighbor
            if (_prey_cell_in_nbhood.size() > 0) {

                _prey_cell_in_nbhood[0] = _prey_cell_in_nbhood[dist_prey(*this->_rng)];
                _prey_cell_in_nbhood.resize(1);
                
                

                apply_rule<false>([state](const auto cell){
                    auto nb_state = cell->state();
                    nb_state.population = pred_prey;
                    nb_state.resource_pred = state.resource_pred;
                    return nb_state;
                }, 
                _prey_cell_in_nbhood);

                state.population = empty;
                state.resource_pred = 0;

            }

            else if (_empty_cell_in_nbhood.size() > 0) {

                 _empty_cell_in_nbhood[0] = _empty_cell_in_nbhood[dist_empty(*this->_rng)];
                 _empty_cell_in_nbhood.resize(1);
                
                apply_rule<false>([state](const auto cell){
                    auto nb_state = cell->state();
                    nb_state.population = predator;
                    nb_state.resource_pred = state.resource_pred;
                    return nb_state;
                }, 
                _empty_cell_in_nbhood);

                state.population = empty;
                state.resource_pred = 0;
                
            }

        }

        else if (state.population == pred_prey) {
            // checking neighbouring cells if empty for the prey to flee

            // clear container for empty cells
            _empty_cell_in_nbhood.clear();

            for (auto nb : MooreNeighbor::neighbors(cell, this->_manager))
            {
                auto nb_state = nb->state();
                

                if (nb_state.population == empty) {
                    _empty_cell_in_nbhood.push_back(nb);

                }
            }

            // choose a random cell for the movement if there is more than one
            std::uniform_int_distribution<> dist(0, _empty_cell_in_nbhood.size()-1);
            
            // random number for flight
            std::uniform_real_distribution<> rand(0, 1);

            // fleeing probability from config
            const auto p_flee = as_double(this->_cfg["p_flee"]);

            if (rand(*this->_rng) < p_flee && _empty_cell_in_nbhood.size() > 0) {

                _empty_cell_in_nbhood[0] = _empty_cell_in_nbhood[dist(*this->_rng)];
                _empty_cell_in_nbhood.resize(1);
                
                apply_rule<false>([state](const auto cell){
                    auto nb_state = cell->state();
                    nb_state.population = prey;
                    nb_state.resource_prey = state.resource_prey;
                    return nb_state;
                }, 
                _empty_cell_in_nbhood);

                state.population = predator;
                state.resource_prey = 0;
                
            }
        }
        return state;
    };

    /// Define the eating rule 
    std::function<State(std::shared_ptr<CellType>)> _eat = [this](const auto cell){
        // Update procedure is as follows:
        // prey is consumed if predator and prey on the same cell
        // prey resource is increased if there is just prey on the cell
        
        // Parameters for resources
        unsigned short int delta_e = as_<unsigned short int>(this->_cfg["delta_e"]);
        unsigned short int e_max = as_<unsigned short int>(this->_cfg["e_max"]);

        // Get the state of the cell
        auto state = cell->state();

        // Eating: preys get eaten by predators and preys eat
        if (state.population == pred_prey) {
            state.population = predator;
            state.resource_pred = std::clamp<unsigned short>(state.resource_pred + delta_e, 0, e_max);
            state.resource_prey = 0;
            
        }
        else if (state.population == prey) {
            state.resource_prey = std::clamp<unsigned short>(state.resource_prey + delta_e, 0, e_max);
        }
        return state;
    };

    /// Define the reproduction rule
    std::function<State(std::shared_ptr<CellType>)> _repro = [this](const auto cell) {
        // Reproduction with possibility p_repro if empty space available

        // Get the state of the cell
        auto state = cell->state();

        // random number for flight, distribution
        std::uniform_real_distribution<> rand(0, 1); 

        if ((state.population == predator) && (rand(*this->_rng) < as_double(this->_cfg["p_repro"])) && (state.resource_pred > as_<unsigned short int>(this->_cfg["e_min"]))) {
            
            _repro_cell_in_nbhood.clear();
            

            for (auto nb : MooreNeighbor::neighbors(cell, this->_manager))
            {
                auto nb_state = nb->state();


                if (nb_state.population == prey || nb_state.population == empty) {

                     _repro_cell_in_nbhood.push_back(nb);

                }
            }

            if (_repro_cell_in_nbhood.size() > 0) {
                //choose a random cell for the offspring to be placed on
                std::uniform_int_distribution<> dist(0, _repro_cell_in_nbhood.size()-1);
                _repro_cell_in_nbhood[0] = _repro_cell_in_nbhood[dist(*this->_rng)];
                _repro_cell_in_nbhood.resize(1);

                apply_rule<false>([](const auto cell){

                    auto nb_state = cell->state();

                    nb_state.population = static_cast<Population>(nb_state.population + 2); // new state will be predator or pred_prey

                    nb_state.resource_pred = 2; // give 2 units to offspring

                    return nb_state;

                }, _repro_cell_in_nbhood);
                
                // deduct cost of reproduction
                state.resource_pred -= 2;
            }
        }

        else if (state.population == prey && rand(*this->_rng) < as_double(this->_cfg["p_repro"]) && state.resource_prey > as_<unsigned short int>(this->_cfg["e_min"])) {
            
            _repro_cell_in_nbhood.clear();
            

            for (auto nb : MooreNeighbor::neighbors(cell, this->_manager))
            {
                auto nb_state = nb->state();


                if (nb_state.population == predator || nb_state.population == empty) {

                     _repro_cell_in_nbhood.push_back(nb);

                }
            }


            if (_repro_cell_in_nbhood.size() > 0) {
                //choose a random cell for the offspring to be placed on
                std::uniform_int_distribution<> dist(0, _repro_cell_in_nbhood.size()-1);
                _repro_cell_in_nbhood[0] = _repro_cell_in_nbhood[dist(*this->_rng)];
                _repro_cell_in_nbhood.resize(1);

                apply_rule<false>([](const auto cell){

                    auto nb_state = cell->state();

                    nb_state.population = static_cast<Population>(nb_state.population + 1); //new state will be predator or pred_prey

                    nb_state.resource_prey = 2; // give 2 units of resource to offspring

                    return nb_state;

                }, _repro_cell_in_nbhood);
                
                // deduct cost of reproduction
                state.resource_prey -= 2;
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
    template<class ParentModel>
    PredatorPreyModel (const std::string name,
                   ParentModel &parent,
                   ManagerType&& manager)
    :
        // Initialize first via base model
        Base(name, parent),
        // Now initialize members specific to this class
        _manager(manager),
        _prey_cell_in_nbhood(),
        _empty_cell_in_nbhood(),
        _repro_cell_in_nbhood(),
        // datasets
        _dset_population(this->_hdfgrp->open_dataset("Population")),
        _dset_resource_prey(this->_hdfgrp->open_dataset("resource_prey")),
        _dset_resource_pred(this->_hdfgrp->open_dataset("resource_pred"))

        
    {   
        // Initialize cells
        this->initialize_cells();
        // Set dataset capacities
        // We already know the maximum number of steps and the number of cells
        const hsize_t num_cells = std::distance(_manager.cells().begin(),
                                                _manager.cells().end());
        this->_log->debug("Setting dataset capacities to {} x {} ...",
                        this->get_time_max() + 1, num_cells);
        _dset_population->set_capacity({this->get_time_max() + 1, num_cells});
        _dset_resource_prey->set_capacity(  {this->get_time_max() + 1, num_cells});
        _dset_resource_pred->set_capacity(  {this->get_time_max() + 1, num_cells});
        // Write initial state
        this->write_data();


        // Create
    }

    // Setup functions ........................................................
    /// Initialize the cells randomly
    void initialize_cells()
    {
        // Extract the mode that determines the initial state
        auto initial_state = as_str(this->_cfg["initial_state"]);

        this->_log->info("Initializing cells in '{}' mode ...", initial_state);

        // Distinguish according to the mode, which state to choose
        if (initial_state == "random")
        {
            // Get the threshold probability value
            const auto prey_prob = as_double(this->_cfg["prey_prob"]);
            const auto pred_prob = as_double(this->_cfg["pred_prob"]);

            // Use a uniform real distribution for random numbers
            auto rand = std::bind(std::uniform_real_distribution<>(),
                                  std::ref(*this->_rng));

            // Define the update rule
            auto set_random_population = [&rand, &prey_prob, &pred_prob](const auto cell) {
                // Get the state
                auto state = cell->state();

                // Draw a random number and compare it to the thresholds
                double random_number = rand();

                if (random_number < prey_prob) {
                    // put prey on the cell and give 2 resource units to it
                    state.population = Population::prey;
                    state.resource_prey = 2;
                    state.resource_pred 0;
                }
                else if (random_number < (prey_prob + pred_prob)){
                    // put a predator on the cell and give 2 resource units to it
                    state.population = Population::predator;
                    state.resource_pred = 2;
                    state.resource_prey = 0;
                }
                else {
                    // initialize as empty
                    state.population = Population::empty;
                    state.resource_pred = 0;
                    state.resource_prey = 0;
                }
                return state;
            };
            
            // Apply the rule to all cells
            apply_rule<false>(set_random_population, _manager.cells());
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

    void perform_step ()
    {      
        
        // Apply the rules to all cells
        apply_rule<false>(_cost, _manager.cells());
        
        apply_rule(_move, _manager.cells(), *this->_rng);
        

        apply_rule<false>(_eat, _manager.cells());
        
        apply_rule(_repro, _manager.cells(), *this->_rng);
    }


    /// Write data
    void write_data ()
    {   
        // For the grid data, get the cells in order to iterate over them
        auto cells = _manager.cells();

        // Population
        _dset_population->write(cells.begin(), cells.end(),
                              [](auto& cell) {
                                return static_cast<unsigned short int>(cell->state().population);
                              });

        // resource of prey
        _dset_resource_prey->write(cells.begin(), cells.end(),
                              [](auto& cell) {
                                return static_cast<unsigned short int>(cell->state().resource_prey);
                              });
        

        // resource of pred

        _dset_resource_pred->write(cells.begin(), cells.end(), 
                              [](auto& cell) {
                                return static_cast<unsigned short int>(cell->state().resource_pred);
        });

    }


};


/// Setup the grid manager with an initial state
/** \param name          TODO
  * \param parent_model  TODO
  *
  * \tparam periodic     Whether the grid should be periodic
  * \tparam ParentModel  The parent model type
  */ 
template<bool periodic=true, typename ParentModel>
auto setup_manager(std::string name, ParentModel& parent_model)
{
    // Get the logger... and use it :)
    auto log = parent_model.get_logger();
    log->info("Setting up '{}' model ...", name);

    // Get the configuration and the rng
    auto cfg = parent_model.get_cfg()[name];
    auto rng = parent_model.get_rng();

    // Extract grid size from config
    const auto gsize = as_array<unsigned int, 2>(cfg["grid_size"]);

    // Inform about the size
    log->info("Creating 2-dimensional grid of size: {} x {} ...",
              gsize[0], gsize[1]);

    // Create grid of that size
    auto grid = Utopia::Setup::create_grid<2>(gsize);

    // Create the PredatorPrey initial state: empty cells and no resources
    State state_0 = {Population::empty, static_cast<unsigned short>(0), static_cast<unsigned short>(0)};

    // Create cells on that grid, passing the initial state
    auto cells = Utopia::Setup::create_cells_on_grid<false>(grid, state_0);

    // Create the grid manager, passing the template argument
    if (periodic) {
        log->info("Now initializing GridManager with periodic boundary "
                  "conditions ...");
    }
    else {
        log->info("Now initializing GridManager with fixed boundary "
                  "conditions ...");
    }
    
    return Utopia::Setup::create_manager_cells<true, periodic>(grid,
                                                               cells,
                                                               rng);
}


} // namespace PredatorPrey
} // namespace Models
} // namespace Utopia
#endif // UTOPIA_MODELS_PREDATORPREY_HH
