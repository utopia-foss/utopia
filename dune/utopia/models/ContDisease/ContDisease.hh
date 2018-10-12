#ifndef UTOPIA_MODELS_CONTDISEASE_HH
#define UTOPIA_MODELS_CONTDISEASE_HH

#include <functional>

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>
#include <dune/utopia/core/types.hh>




namespace Utopia {
namespace Models {
namespace ContDisease {

/// State of forest cell
enum CellState : unsigned short int {empty = 0, tree = 1, infected = 2, herd = 3, stone = 4};



/// Typehelper to define data types of ContDisease model
using ContDiseaseModelTypes = ModelTypes<>;



/// Contagious disease model on a grid
/** In this model, we model the spread of a disease through a forest on
 * a 2D grid. Each cell can have one of five different states: empty, tree,
 * infected, herd or empty. Each time step cells update their state according
 * to the update rules. Empty cells will convert with a certain probability
 * to tress, while trees represent cells that can be infected. Infection can
 * happen either through a neighboring cells, or through random point infection.
 * A infected cells reverts back to empty after one time step. Stones represent
 * cells that can not be infected, therefore represent a blockade for the spread
 * of the infection. Infection herds are cells that continously spread infection
 * without dying themselves. Different starting conditions, and update
 * mechanisms can be configured.
 */

template<class ManagerType>
class ContDiseaseModel:
    public Model<ContDiseaseModel<ManagerType>, ContDiseaseModelTypes>
{
public:
    /// The base model type
    using Base = Model<ContDiseaseModel<ManagerType>, ContDiseaseModelTypes>;

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

    // Alias the neighborhood classes to make access more convenient
    using NextNeighbor = Utopia::Neighborhoods::NextNeighbor;
    using MooreNeighbor = Utopia::Neighborhoods::MooreNeighbor;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng

    // -- Members of this model -- //
    /// The grid manager
    ManagerType _manager;

    /// Probability for the appearance of a tree
    const double _p_growth;

    /// Probability that an infected cell infects a neighbouring cell
    const double _p_infect;

    /// Probability for a random infection
    const double _p_rd_infect;


    // -- Datasets -- //
    std::shared_ptr<DataSet> _dset_state;


    // -- Rule functions -- //

    /// Sets the given cell to state "empty"
    std::function<CellState(std::shared_ptr<CellType>)> _set_initial_state_empty = [](const auto cell){
        // Get the state of the Cell
        auto cellstate = cell->state();

        // Set cell to empty
        cellstate = empty;
        return cellstate;
    };




    /// Define the update rule
    std::function<CellState(std::shared_ptr<CellType>)> _update = [this](const auto cell){
        // Update (all cells at the same time) according to the following rules:
        // Empty cells grow "trees with probability _p_growth.
        // Tree cells in neighborhood of an infected cell get infected with the
        // probability _p_infect.
        // Infected cells die and become an empty cell.

        // Get the state of the cell
        auto cellstate = cell->state();


        if (cellstate == empty){

          // With a probablity of _p_growth set the treestate of the cell to tree.
          std::uniform_real_distribution<> dist(0., 1.);
          if (dist(*this->_rng) < _p_growth){
              cellstate = tree;
          }
        }

        else if (cellstate == tree){
          // Tree can be infected by neighbor our by random-point-infection.
          std::uniform_real_distribution<> dist(0., 1.);

          // infection by random point infection
          if (dist(*this->_rng) < _p_rd_infect){
            cellstate = infected;
          }


          // Go through neighbor cells (here 5-cell neighbourhood), look if they
          // are infected (or an infection herd), if yes, infect cell with the probability _p_infect.

          for (auto nb : NextNeighbor::neighbors(cell, this->_manager)){

            if (cellstate == tree){
              auto nb_cellstate = nb->state();


              if (nb_cellstate == infected || nb_cellstate == herd){
                if (dist(*this->_rng) < _p_infect){
                    cellstate = infected;
                  }
              }
            }
            else {
              break;
            }
          }

        }

        else if (cellstate == infected){
          cellstate = empty;
        }

        return cellstate;
    };


public:
    /// Construct the ContDisease model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template<class ParentModel>
    ContDiseaseModel (const std::string name,
                 ParentModel &parent,
                 ManagerType&& manager)
    :
        // Initialize first via base model
        Base(name, parent),
        // Now initialize members specific to this class
        _manager(manager),

        // initialize probabilities
        _p_growth(as_double(this->_cfg["p_growth"])),
        _p_infect(as_double(this->_cfg["p_infect"])),
        _p_rd_infect(as_double(this->_cfg["p_rd_infect"])),

        // create dataset for treestates
        _dset_state(this->_hdfgrp->open_dataset("state"))

    {
        // Call the method that initializes the cells
        this->initialize_cells();

        // Set the capacity of the datasets
        // We know the maximum number of steps (== #rows), and the number of
        // grid cells (== #columns); that is the final extend of the dataset.
        const hsize_t num_cells = std::distance(_manager.cells().begin(),
                                                _manager.cells().end());
        this->_log->debug("Setting dataset capacities to {} x {} ...",
                          this->get_time_max() + 1, num_cells);
        _dset_state->set_capacity({this->get_time_max() + 1, num_cells});


        // Write initial state
        this->write_data();
    }

    // Setup functions ........................................................

    /// Initialize the cells according to `initial_state` and
    /// the 'infection_herd' and 'infection_herd_src' config parameters
    void initialize_cells()
    {
        // Extract the mode that determines the initial state
        const auto initial_state = as_str(this->_cfg["initial_state"]);

        // Extract if an infection herd is activated
        const bool infection_herd = as_bool(this->_cfg["infection_herd"]);

        // Extract postion of possible infection herd
        const auto infection_herd_src = as_str(this->_cfg["infection_herd_src"]);

        // Extract if stones are activated
        const bool stones = as_bool(this->_cfg["stones"]);

        // Extract how stones are to be initialized
        const auto stone_init = as_str(this->_cfg["stone_init"]);

        // Extract stone density for stone_init = random
        const double stone_density = as_double(this->_cfg["stone_density"]);

        // Extract clustering weight for stone_init = random
        const double stone_cluster = as_double(this->_cfg["stone_cluster"]);



        this->_log->info("Initializing cells in '{}' mode ...", initial_state);


        // Different initialization methods for the forest

        //empty forest
        if (initial_state == "empty")
        {
            //Apply rule to all cells in the forest.
            apply_rule(_set_initial_state_empty, _manager.cells());
        }
        else
        {
            throw std::invalid_argument("The initial state ''" + initial_state + "'' is not valid! Valid options: 'empty'");
        }

        // Different initializations for possible infection herds.
        if (infection_herd){

          if (infection_herd_src == "south"){

            auto _set_infection_herd_south = [&](const auto cell){
              auto cellstate = cell->state();

              // Get postion of the Cell, grid extensions and number of cells
              const auto& pos = cell->position();
              const auto& grid_ext = _manager.extensions();
              const auto& grid_num_cells = _manager.grid_cells();

              const auto& cell_size_y = grid_ext[1]  / grid_num_cells[1];

              if (pos[1] < cell_size_y){
                cellstate = herd;
              }
              return cellstate;
            };
            apply_rule(_set_infection_herd_south, _manager.cells());
          }

          else {
            throw std::invalid_argument("The infection herd source ''" + infection_herd_src + "'' is not valid! Valid options: 'south'");
          }
        }

        else {
          this->_log->debug("Not using an infection herd.");
        }


        if (stones){

          if (stone_init == "random"){
            // initialize stones at random so a certain areal density is reached
            std::uniform_real_distribution<> dist(0., 1.);

            auto cells_rd = _manager.cells();
            std::shuffle(cells_rd.begin(),cells_rd.end(), *this->_rng);

            for (auto&& cell: cells_rd ){
              if (dist(*this->_rng) < stone_density){
                cell->state_new() = stone;
                cell->update();
              }
            }

            for (auto&& cell: cells_rd ){
              for (auto nb : MooreNeighbor::neighbors(cell, this->_manager)){
                if(cell->state() == empty){
                  if (nb->state() == stone){
                    if (dist(*this->_rng) < stone_cluster){
                      cell->state_new() = stone;
                      cell-> update();
                    }
                  }
                }
                else{
                  break;
                }
              }
            }
          }

          else{
            throw std::invalid_argument("The stone initialization ''" + stone_init + "'' is not valid! Valid options: 'random'");
          }

        }
        else{
          this->_log->debug("Not using stones.");
        }

        // Write information that cells are initialized to the logger
        this->_log->info("Cells initialized.");
    }


    // Runtime functions ......................................................

    /** @brief Iterate a single step
     *  @detail Here you can add a detailed description what exactly happens
     *          in a single iteration step
     */
    void perform_step ()
    {
        // Apply the rules to all cells, first the interaction, then the update
        apply_rule(_update, _manager.cells());
    }


    /// Write data
    void write_data ()
    {
        // _dset_state
        _dset_state->write(_manager.cells().begin(), _manager.cells().end(),
                           [](auto& cell) {
                                return static_cast<unsigned short int>(cell->state());
                            });
    }


    // Getters and setters ....................................................
    // Can add some getters and setters here to interface with other model
};


/**
 * @brief Set up the grid manager and initialize the cells
 *
 * @tparam periodic=true Use periodic boundary conditions
 * @tparam ParentModel The parent model type
 * @param name The name of the model
 * @param parent_model The parent model
 * @return auto The manager
 */
template<bool periodic=true, typename ParentModel>
auto setup_manager(std::string name, ParentModel& parent_model)
{
    // Get the logger... and use it :)
    auto log = parent_model.get_logger();
    log->info("Setting up '{}' model ...", name);

    // Get the configuration
    auto cfg = parent_model.get_cfg()[name];

    // Extract grid size from config
    const auto gsize = as_array<unsigned int, 2>(cfg["grid_size"]);

    // Inform about the size
    log->info("Creating 2-dimensional grid of size: {} x {} ...",
              gsize[0], gsize[1]);

    // Create grid of that size
    auto grid = Utopia::Setup::create_grid<2>(gsize);

    // Create the ContDisease initial state:
    CellState cellstate_0 = empty;

    // Create cells on that grid, passing the initial state
    auto cells = Utopia::Setup::create_cells_on_grid<true>(grid, cellstate_0);

    // Create the grid manager, passing the template argument
    log->info("Initializing GridManager with {} boundaries ...",
              (periodic ? "periodic" : "fixed"));

    return Utopia::Setup::create_manager_cells<true, periodic>(grid, cells);
}


} // namespace ContDisease
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_CONTDISEASE_HH
