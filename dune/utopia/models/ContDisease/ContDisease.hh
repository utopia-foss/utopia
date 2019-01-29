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
enum CellState : unsigned short {
    empty = 0,
    tree = 1,
    infected = 2,
    herd = 3,
    stone = 4
};


/// Typehelper to define data types of ContDisease model
using ContDiseaseModelTypes = ModelTypes<>;


/// Contagious disease model on a grid
/** In this model, we model the spread of a disease through a forest on
 * a 2D grid. Each cell can have one of five different states: empty, tree,
 * infected, herd or empty. Each time step cells update their state according
 * to the update rules. Empty cells will convert with a certain probability
 * to tress, while trees represent cells that can be infected.
 * Infection can happen either through a neighboring cells, or through random
 * point infection. An infected cells reverts back to empty after one time
 * step.
 * Stones represent cells that can not be infected, therefore represent a
 * blockade for the spread of the infection.
 * Infection herds are cells that continously spread infection without dying
 * themselves.
 * Different starting conditions, and update mechanisms can be configured.
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

    /// Rule Function
    using RuleFunc = typename std::function<CellState(std::shared_ptr<CellType>)>;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    // Alias the neighborhood classes to make access more convenient
    using Neighborhood = Utopia::Neighborhoods::MooreNeighbor;

private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor

    // -- Members of this model -- //
    /// The grid manager
    ManagerType _manager;

public:
    // -- Parameters that determine model dynamics, publicly available -- //
    /// Probability for the appearance of a tree
    double _p_growth;

    /// Probability that an infected cell infects a neighbouring cell
    double _p_infect;

    /// Probability for a random infection
    double _p_rd_infect;

private:
    // -- Datasets -- //
    std::shared_ptr<DataSet> _dset_state;
    std::shared_ptr<DataSet> _dset_density_tree;
    std::shared_ptr<DataSet> _dset_density_infected;
    
    // -- Temporary objects -- //
    // Array that holds the densities for all states
    // NOTE: It is not automatically updated.
    std::array<double, 5> _density;

    // -- Helper functions -- //
    /// Initially calculate global averaged state densities
    void _initialize_densities()
    {
        // Count the trees, the infected trees, the herd, and the stones
        std::size_t cnt_tree=0, cnt_infected=0, cnt_stone=0, cnt_herd=0;
        for (const auto& cell : this->_manager.cells()) 
        {
            const auto& state = cell->state();
            
            if (state == tree) {
                ++cnt_tree;
            }
            else if (state == infected) {
                ++cnt_infected;
            }
            else if (state == stone) {
                ++cnt_stone;
            }
            else if (state == herd) {
                ++cnt_herd;
            }
        }

        // Calculate the densities for 
        //   empty        (_density[0]),
        //   tree         (_density[1]),
        //   infected     (_density[2]),
        //   herd         (_density[3]),
        //   stone        (_density[4])
        // The herd and stone density remain the same and do not need to be
        // recalculated every time.
        const std::size_t num_cells = this->_manager.cells().size();

        const double density_tree = cnt_tree/double(num_cells);
        const double density_infected = cnt_infected/double(num_cells);
        const double density_stone = cnt_stone/double(num_cells);
        const double density_herd = cnt_herd/double(num_cells);

        const double density_empty = 1. - density_tree 
                                        - density_infected 
                                        - density_herd
                                        - density_stone;

        // Store the densities in the array
        _density[0] = density_empty;
        _density[1] = density_tree;
        _density[2] = density_infected;
        _density[3] = density_herd;
        _density[4] = density_stone;
    };


    /// Update the array with the densities by calculating globally averaged state densities
    void _update_densities()
    {
        // Count the trees and the infected trees
        unsigned int cnt_tree = 0, cnt_infected = 0;
        for (const auto& cell : this->_manager.cells()) 
        {
            const auto& state = cell->state();
            
            if (state == tree) {
                ++cnt_tree;
            }
            else if (state == infected) {
                ++cnt_infected;
            }
        }

        // Calculate the densities for 
        //   empty        (_density[0]),
        //   tree         (_density[1]),
        //   infected     (_density[2]),
        //   herd         (_density[3]),
        //   stone        (_density[4])
        // The herd and stone density remain the same and do not need to be
        // recalculated every time.
        const unsigned int num_cells = this->_manager.cells().size();

        const double density_tree = cnt_tree/double(num_cells);
        const double density_infected = cnt_infected/double(num_cells);
        const double density_empty = 1. - density_tree 
                                        - density_infected 
                                        - _density[3] /*herd*/ 
                                        - _density[4] /*stone*/;

        // Store the densities in the array (stones and herd remain constant)
        _density[0] = density_empty;
        _density[1] = density_tree;
        _density[2] = density_infected;
    };

    /// Initialize stones at random so a certain areal density is reached
    void _init_stones_rd (const double stone_density,
                          const double stone_cluster) {
        std::uniform_real_distribution<> dist(0., 1.);

        auto cells_rd = _manager.cells();
        std::shuffle(cells_rd.begin(),cells_rd.end(), *this->_rng);

        // Set the state of the cell to stone with probability stone_density
        for (const auto& cell: cells_rd ){
            if (dist(*this->_rng) < stone_density){
                cell->state_new() = stone;
                cell->update();
            }
        }

        // Attempt to encourage the stones to form continuous clusters
        for (const auto& cell: cells_rd ){
            for (const auto& nb : Neighborhood::neighbors(cell, this->_manager)){
                if (   (cell->state() == empty)
                    && (nb->state() == stone)
                    && (dist(*this->_rng) < stone_cluster))
                {
                    cell->state_new() = stone;
                    cell-> update();
                }
                else {
                    break;
                }
            }
        }
    };

    // -- Rule functions -- //
    /// Sets the given cell to state "empty"
    RuleFunc _set_initial_state_empty = []([[maybe_unused]] const auto& cell){
        return empty;
    };

    /// Sets the given cell to state "tree" with probability p, else to "empty"
    // TODO write test
    RuleFunc _set_initial_density = [this](const auto& cell){
        // Get the state of the Cell
        auto state = cell->state();

        std::uniform_real_distribution<> dist(0., 1.);

        // Set the internal variables
        if (dist(*this->_rng) < _density[1])
        {
            state = tree;
        }
        else
        {
            state = empty;
        }

        return state;
    };

    /// Sets an infection herd at the southern end of the grid
    RuleFunc _set_infection_herd_south = [this](const auto& cell){
        auto state = cell->state();

        // Get position of the Cell, grid extensions and number of cells
        const auto& pos = cell->position();

        const auto& grid_ext = this->_manager.extensions();
        const auto& grid_num_cells = this->_manager.grid_cells();
        const auto& cell_size_y = grid_ext[1]  / grid_num_cells[1];

        // If in the southern-most row of cells, this is an infection herd
        if (pos[1] < cell_size_y) {
            return herd;
        }
        return state;
    };

    /// Define the update rule
    /** Update (all cells at the same time) according to the following rules:
     * Empty cells grow "trees with probability _p_growth.
     * Tree cells in neighborhood of an infected cell get infected with the
     * probability _p_infect.
     * Infected cells die and become an empty cell.
     */
    RuleFunc _update = [this](const auto& cell){
        // Get the state of the cell
        auto cellstate = cell->state();

        // Distinguish by current state
        if (cellstate == empty){
            // With a probability of _p_growth, set the cell's state to tree
            std::uniform_real_distribution<> dist(0., 1.);
            if (dist(*this->_rng) < _p_growth){
                cellstate = tree;
            }
        }
        else if (cellstate == tree){
            // Tree can be infected by neighbor our by random-point-infection.
            std::uniform_real_distribution<> dist(0., 1.);

            // Infection by random point infection
            if (dist(*this->_rng) < _p_rd_infect){
                cellstate = infected;
            }
            // Go through neighbor cells (here 5-cell neighbourhood), look if
            // they are infected (or an infection herd), if yes, infect cell
            // with the probability _p_infect.
            else 
            {               
                // TODO implement neighborhood as template argument
                for (const auto& nb : Neighborhood::neighbors(cell, this->_manager)){
                    if (cellstate == tree){
                        auto
                         nb_cellstate = nb->state();
                        if (nb_cellstate == infected || nb_cellstate == herd){
                            if (dist(*this->_rng) < _p_infect){
                                cellstate = infected;
                            }
                        }
                    }
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

        // Initialize probabilities from config parameters
        _p_growth(as_double(this->_cfg["p_growth"])),
        _p_infect(as_double(this->_cfg["p_infect"])),
        _p_rd_infect(as_double(this->_cfg["p_rd_infect"])),

        // Create dataset with compression level 1
        _dset_state(this->create_dset("state", {_manager.cells().size()})),
        _dset_density_tree(this->create_dset("density_tree", {})),
        _dset_density_infected(this->create_dset("density_infected", {}))
    {
        // Call the method that initializes the cells
        this->initialize_cells();

        // Write initial state
        this->write_data();

        // Add attributes to the datasets
        // NOTE Currently, attributes can be set only after the first write
        //      operation because else the datasets are not yet created.
        const auto grid_size = as_<std::array<std::size_t,2>>(this->_cfg["grid_size"]);
        _dset_state->add_attribute("content", "grid");
        _dset_state->add_attribute("grid_shape", grid_size);
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

        // Extract position of possible infection herd
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
        if (initial_state == "empty"){
            //Apply rule to all cells in the forest.
            apply_rule(_set_initial_state_empty, _manager.cells());
        }
        else if (initial_state == "init_density") {
            if (not stones) {
                _density[1] = as_double(this->_cfg["initial_density"]);
                apply_rule(_set_initial_density, _manager.cells());
            }
            // TODO write proper init function
            else {
                 throw std::invalid_argument("The stone initialization"
                                             " with init_density is not valid!"
                                             " Valid options: 'init_empty'");
            }
        }
        else {
            throw std::invalid_argument("The initial state ''" + initial_state
                                        + "'' is not valid! Valid options: "
                                        "'empty'");
        }

        // Different initializations for possible infection herds
        if (infection_herd){
            if (infection_herd_src == "south"){
                // Set infection herd at the southern border of the grid
                apply_rule(_set_infection_herd_south, _manager.cells());
            }
            else {
                throw std::invalid_argument("The infection herd source ''"
                                            + infection_herd_src + "'' is not "
                                            "valid! Valid options: 'south'");
            }
        }
        else {
            this->_log->debug("Not using an infection herd.");
        }

        // -- Different intializations for stones
        if (stones){
            if (stone_init == "random"){
                _init_stones_rd(stone_density, stone_cluster);
            }
            else {
                throw std::invalid_argument("The stone initialization ''"
                                            + stone_init + "'' is not valid! "
                                            "Valid options: 'random'");
            }
        }
        else {
            this->_log->debug("Not using stones.");
        }

        // Write information that cells are initialized to the logger
        this->_log->info("Cells initialized.");

        // Calculate all initial densities including the ones that stay
        // constant afterwards (stones, herd)
        _initialize_densities();
    }


    // Runtime functions ......................................................

    /** @brief Iterate a single step
     *  @detail Each cell updates depending on its state and the state of the
     *          neighbouring cells according to the following update rules:
     *          1: Infected --> Empty
     *          2: Empty    --> Tree
     *               - with a probability p_growth
     *          3: Tree     --> Infected
     *               - with the probability p_infect for each infected
     *                 or herd cell in the neighbourhood
     *               - with probability p_rd_infect for random-point infections
     *          4: Herd or stone cells remain in their respective state.
     */
    void perform_step () {
        // Apply the rules to all cells, first the interaction, then the update
        apply_rule(_update, _manager.cells());
    }


    /// Monitor model information
    /** @detail Here, functions and values can be supplied to the monitor that
     *          are then available to the frontend. The monitor() function is
     *          _only_ called if a certain emit interval has passed; thus, the
     *          performance hit is small. Also, if using set_by_func, the given
     *          lambda will only be called if an emission will happen.
     */
    void monitor () {        
        _update_densities();
        this->_monitor.set_entry("density", _density);
    }


    /// Write data
    void write_data () {
        // _dset_state
        _dset_state->write(_manager.cells().begin(), _manager.cells().end(),
                           [](const auto& cell) {
                             return static_cast<unsigned short>(cell->state());
                           });
        
        // state densities
        // TODO write test
        _update_densities();
        _dset_density_tree->write(_density[1]);
        _dset_density_infected->write(_density[2]);
    }


    // Getters and setters ....................................................
    // Can add some getters and setters here to interface with other model
};


/**
 * @brief Set up the grid manager and initialize the cells
 *
 * @tparam periodic=true Use periodic boundary conditions
 * @tparam ParentModel The parent model type
 *
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
