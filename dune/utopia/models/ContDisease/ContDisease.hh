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
 * Infection herds are cells that continuously spread infection without dying
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

    /// Data type for a data group
    using DataGroup = typename Base::DataGroup;

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
    // -- Datagroups -- //
    std::shared_ptr<DataGroup> _hdfgrp_densities;

    // -- Datasets -- //
    std::shared_ptr<DataSet> _dset_state;
    std::shared_ptr<DataSet> _dset_density_empty;
    std::shared_ptr<DataSet> _dset_density_tree;
    std::shared_ptr<DataSet> _dset_density_infected;
    std::shared_ptr<DataSet> _dset_density_herd;
    std::shared_ptr<DataSet> _dset_density_stone;
    
    // -- Temporary objects -- //
    /// Densities for all states
    /** \note This array is used for temporary storage; it is not automatically updated!
     * \detail The array entries map to the CellState enum:
     *         0: empty
     *         1: tree
     *         2: infected
     *         3: herd
     *         4: stone
     */
    std::array<double, 5> _densities;

    // -- Helper functions -- //
    /// Update the densities array by calculating globally averaged state densities
    /** @brief Each density is calculated by counting the number of state 
     *         occurrences and afterwards dividing by the total number of cells.
     * @attention It is possible that rounding errors occur due to the division,
     *            thus, it is not guaranteed that the densities exactly add up to 1.
     *            The errors should be neglectably small.
     */
    void _update_densities()
    {
        // Temporarily overwrite every entry in the densities with zeroes
        _densities.fill(0);

        // Count the occurrence of each possible state
        // Afterwards, the _densities array contains the counts.
        for (const auto& cell : this->_manager.cells()) {
            // Cast enum to integer to arrive at the corresponding index
            ++_densities[static_cast<std::size_t>(cell->state())];
        }

        const double num_cells = this->_manager.cells().size();

        // Calculate the densities by dividing the counts by the total number 
        // of cells.
        for (auto&& d : _densities){
            d/=num_cells;
        }
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
                    cell->update();
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
    RuleFunc _set_initial_densities = [this](const auto& cell){
        // Get the state of the Cell
        auto state = cell->state();

        std::uniform_real_distribution<> dist(0., 1.);

        // Set the internal variables
        if (dist(*this->_rng) < as_double(this->_cfg["initial_density"]))
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

        // Create a data group
        _hdfgrp_densities(this->_hdfgrp->open_group("densities")),

        // Create dataset with compression level 1
        _dset_state(this->create_dset("state", {_manager.cells().size()})),
        _dset_density_empty(this->create_dset("empty", _hdfgrp_densities, {})),
        _dset_density_tree(this->create_dset("tree", _hdfgrp_densities, {})),
        _dset_density_infected(this->create_dset("infected", _hdfgrp_densities, {})),
        _dset_density_herd(this->create_dset("herd", _hdfgrp_densities, {})),
        _dset_density_stone(this->create_dset("stone", _hdfgrp_densities, {})),

        // Initialize the densities with NaN's which will be overwritten in the
        // actual initialization
        _densities{NAN, NAN, NAN, NAN, NAN}
    {
        // Call the method that initializes the cells
        this->initialize_cells();

        // Write initial state
        this->write_initial_data();

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

        // -- Trees -- //
        // First initialize the forest – set an initial tree density or no trees
        // at all.
        if (initial_state == "empty"){
            // Set all cell states to empty
            apply_rule(_set_initial_state_empty, _manager.cells());
        }
        else if (initial_state == "init_density") {
            // Set trees on the cells with probability initial_density
            apply_rule(_set_initial_densities, _manager.cells());
        }
        else {
            throw std::invalid_argument("The initial state ''" + initial_state
                                        + "'' is not valid! Valid options: "
                                        "'empty' and 'init_density'");
        }

        // -- Infection herd -- //
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

        // -- Stones -- //
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

        // Calculate the actual density of each state and store them in the
        // _densities array.
        _update_densities();
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
        this->_monitor.set_entry("densities", _densities);
    }


    /// Write data
    void write_data () {
        // _dset_state
        _dset_state->write(_manager.cells().begin(), _manager.cells().end(),
                           [](const auto& cell) {
                             return static_cast<unsigned short>(cell->state());
                           });
        
        // state densities
        _update_densities();
        _dset_density_empty->write(_densities[0]);
        _dset_density_tree->write(_densities[1]);
        _dset_density_infected->write(_densities[2]);
    }


     /// Write initial data
    void write_initial_data () {
        // update the densities
        _update_densities();
        
        // state densities that do not change throughout the simulation
        _dset_density_stone->write(_densities[3]);
        _dset_density_herd->write(_densities[4]);

        // write all other data that is written each write_data call
        this->write_data();
    }


    // Getters and setters ....................................................
    // Can add some getters and setters here to interface with other model
};


} // namespace ContDisease
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_CONTDISEASE_HH
