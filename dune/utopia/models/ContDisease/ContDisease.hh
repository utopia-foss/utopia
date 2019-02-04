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

/// Possible cell states
enum CellState : unsigned short {
    /// Unoccupied
    empty = 0,
    /// Cell represents a tree
    tree = 1,
    /// Cell is infected
    infected = 2,
    /// Cell is an infection source: constantly infected, spreading infection
    source = 3,
    /// Cell cannot be infected
    stone = 4
};


/// Typehelper to define data types of ContDisease model
using ContDiseaseModelTypes = ModelTypes<>;


/// Contagious disease model on a grid
/** \detail In this model, we model the spread of a disease through a forest on
 *          a 2D grid. Each cell can have one of five different states: empty,
 *          tree, infected, source or empty.
 *          Each time step, cells update their state according to the update
 *          rules. Empty cells will convert with a certain probability
 *          to tress, while trees represent cells that can be infected.
 *          Infection can happen either through a neighboring cells, or
 *          through random point infection. An infected cells reverts back to
 *          empty after one time step.
 *          Stones represent cells that can not be infected, therefore
 *          represent a blockade for the spread of the infection.
 *          Infection sources are cells that continuously spread infection
 *          without dying themselves.
 *          Different starting conditions, and update mechanisms can be
 *          configured.
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

    /// Rule function type
    using RuleFunc = typename std::function<CellState(std::shared_ptr<CellType>)>;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Data type for a data group
    using DataGroup = typename Base::DataGroup;

    /// The chosen neighborhood type
    using Neighborhood = Utopia::Neighborhoods::MooreNeighbor;

private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor

    // -- Members of this model -- //
    /// The grid manager
    ManagerType _manager;

    /// The range [0, 1] distribution to use for probability checks
    std::uniform_real_distribution<double> _prob_distr;

    // -- Temporary objects -- //
    /// Densities for all states
    /** \note   This array is used for temporary storage; it is not
      *         automatically updated!
      * \detail The array entries map to the CellState enum:
      *         0: empty
      *         1: tree
      *         2: infected
      *         3: source
      *         4: stone
      */
    std::array<double, 5> _densities;

public:
    // -- Parameters that determine model dynamics, publicly available -- //
    /// Probability for the appearance of a tree
    double _p_growth;

    /// Probability that an infected cell infects a neighbouring cell
    double _p_infect;

    /// Probability for a random infection
    double _p_rd_infect;


private:
    // -- Data groups -- //
    /// The data group where all density datasets are stored in
    std::shared_ptr<DataGroup> _hdfgrp_densities;

    // -- Datasets -- //
    /// 2D dataset (cell ID and time) of cell states
    std::shared_ptr<DataSet> _dset_state;

    /// 1D dataset of density of empty cells over time
    std::shared_ptr<DataSet> _dset_density_empty;

    /// 1D dataset of density of tree cells over time
    std::shared_ptr<DataSet> _dset_density_tree;
    
    /// 1D dataset of density of infected cells over time
    std::shared_ptr<DataSet> _dset_density_infected;

    /// 1D dataset of density of infected infection source cells over time
    std::shared_ptr<DataSet> _dset_density_source;

    /// 1D dataset of density of infected stone cells over time
    std::shared_ptr<DataSet> _dset_density_stone;
    

    // -- Helper functions -- //
    /// Update the densities array
    /** @detail   Each density is calculated by counting the number of state 
     *            occurrences and afterwards dividing by the total number of
     *            cells.
     * @attention It is possible that rounding errors occur due to the
     *            division, thus, it is not guaranteed that the densities
     *            exactly add up to 1. The errors should be negligible.
     */
    void update_densities() {
        // Temporarily overwrite every entry in the densities with zeroes
        _densities.fill(0);

        // Count the occurrence of each possible state. Use the _densities
        // member for that in order to not create a new array.
        for (const auto& cell : this->_manager.cells()) {
            // Cast enum to integer to arrive at the corresponding index
            ++_densities[static_cast<std::size_t>(cell->state())];
        }
        // The _densities array now contains the counts.

        const double num_cells = this->_manager.cells().size();

        // Calculate the actual densities by dividing the counts by the total
        // number of cells.
        for (auto&& d : _densities){
            d /= num_cells;
        }
    };

    // -- Rule functions -- //
    
    /// Define the update rule
    /** \detail Update the given cell according to the following rules:
      *         - Empty cells grow trees with probability _p_growth.
      *         - Tree cells in neighborhood of an infected cell get infected
      *           with the probability _p_infect.
      *         - Infected cells die and become an empty cell.
      */
    RuleFunc _update = [this](const auto& cell){
        // Get the current state of the cell
        auto cellstate = cell->state();

        // Distinguish by current state
        if (cellstate == empty) {
            // With a probability of _p_growth, set the cell's state to tree
            if (_prob_distr(*this->_rng) < _p_growth){
                return tree;
            }
        }
        else if (cellstate == tree){
            // Tree can be infected by neighbor our by random-point-infection.

            // Determine whether there will be a point infection
            if (_prob_distr(*this->_rng) < _p_rd_infect) {
                // Yes, point infection occurred.
                return infected;
            }
            else {
                // Go through neighbor cells (according to Neighborhood type), 
                // and check if they are infected (or an infection source).
                // If yes, infect cell with the probability _p_infect.
                // TODO implement neighborhood as template argument
                for (const auto& nb : Neighborhood::neighbors(cell,
                                                              this->_manager))
                {
                    // Get the neighbor cell's state
                    auto nb_cellstate = nb->state();
                    
                    if (   nb_cellstate == infected
                        or nb_cellstate == source)
                    {
                        // With a certain probability, become infected
                        if (_prob_distr(*this->_rng) < _p_infect) {
                            return infected;
                        }
                    }
                }
            }            
        }
        else if (cellstate == infected) {
            // Decease -> become an empty cell
            return empty;
        }
        // else: other cell states need no update

        // Return the (potentially changed) cell state for the next round
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
        _prob_distr(0., 1.),
        _densities{},  // undefined here, will be set in constructor body

        // Initialize probabilities from config parameters
        _p_growth(as_double(this->_cfg["p_growth"])),
        _p_infect(as_double(this->_cfg["p_infect"])),
        _p_rd_infect(as_double(this->_cfg["p_rd_infect"])),

        // Create a data group
        _hdfgrp_densities(this->_hdfgrp->open_group("densities")),

        // Create dataset for cell states, specifying grid size
        _dset_state(this->create_dset("state", {_manager.cells().size()})),

        // Create datasets for all densities
        _dset_density_empty(   this->create_dset("empty",
                                                 _hdfgrp_densities, {})),
        _dset_density_tree(    this->create_dset("tree",
                                                 _hdfgrp_densities, {})),
        _dset_density_infected(this->create_dset("infected",
                                                 _hdfgrp_densities, {})),
        _dset_density_source(  this->create_dset("source",
                                                 _hdfgrp_densities, {})),
        _dset_density_stone(   this->create_dset("stone",
                                                 _hdfgrp_densities, {}))
    {
        // Make sure the densities are not undefined
        _densities.fill(std::numeric_limits<double>::quiet_NaN());

        // Call the method that initializes the cells
        this->initialize_cells();

        // -- Write initial state
        // Write all other data that is written each write_data call, which
        // includes the remaining densities (indices 0, 1, and 2)
        this->write_data();

        // Now that all densities have been calculated (in write_data), write
        // those that do not change throughout the simulation (indices 3 and 4)
        _dset_density_stone->write(_densities[3]);   // stone
        _dset_density_source->write(_densities[4]);  // infection source

        // Can now specify attributes that declare the 'state' dataset to
        // be representing a 2D grid with a certain shape
        _dset_state->add_attribute("content", "grid");
        _dset_state->add_attribute("grid_shape",
                                   as_<std::array<std::size_t, 2>>(this->_cfg["grid_size"]));
    }


private:
    // Setup functions ........................................................

    /// Initialize all cells depending on the initialization parameters
    void initialize_cells() {
        // -- Extract Parameters -- //
        // Extract the mode that determines the initial state
        const auto initial_state = as_str(this->_cfg["initial_state"]);

        // For initialization with a density, get the density
        const auto initial_density = as_double(this->_cfg["initial_density"]); 

        // Extract if an infection source is activated
        const bool infection_source = as_bool(this->_cfg["infection_source"]);

        // Extract position of possible infection source
        const auto infection_source_loc = as_str(this->_cfg["infection_source_loc"]);

        // Extract if stones are activated
        const bool stones = as_bool(this->_cfg["stones"]);

        // Extract how stones are to be initialized
        const auto stone_init = as_str(this->_cfg["stone_init"]);

        // Extract stone density for stone_init = random
        const double stone_density = as_double(this->_cfg["stone_density"]);

        // Extract clustering weight for stone_init = random
        const double stone_cluster = as_double(this->_cfg["stone_cluster"]);


        // -- Define initialization functions -- //
        /// Given the density, randomly decides whether this cell is a tree
        RuleFunc become_tree_with_prob = [this, &initial_density](const auto&){
            if (_prob_distr(*this->_rng) < initial_density) {
                return tree;
            }
            // else: will be empty
            return empty;
        };

        // -- Stone initialization function -- // 
        /// Initialize stones randomly with probability stone_density
        RuleFunc init_stones = [this, &stone_density](const auto& cell){
            // Cell will be a stone with probability stone_density
            if (this->_prob_distr(*this->_rng) < stone_density){
                return stone;
            }
            // else: stay in the same state
            return cell->state();
        };


        /// Initialize clustered stones
        /** \detail Add a stone with probability stone_cluster to any empty
          *         cell with a neighboring stone.
          */
        RuleFunc init_stone_clusters = [this, &stone_cluster](const auto& cell){
            // Get the cell state and the cells
            auto state = cell->state();

            // Add the clustered stones
            for (const auto& nb : Neighborhood::neighbors(cell,
                                                          this->_manager))
            {
                if (    (cell->state() == empty) and (nb->state() == stone)
                    and (_prob_distr(*this->_rng) < stone_cluster))
                {
                    // Become a stone
                    state = stone;
                }
                else {
                    break;
                }
            }

            return state;
        };

        /// Sets an infection source at the southern end of the grid
        RuleFunc set_infection_source_south = [this](const auto& cell){
            // Get position of the Cell, grid extensions and number of cells
            const auto& pos = cell->position();

            const auto& grid_ext = this->_manager.extensions();
            const auto& grid_num_cells = this->_manager.grid_cells();
            const auto& cell_size_y = grid_ext[1]  / grid_num_cells[1];

            // If in the southern-most row of cells, this cell is an infection
            // source
            if (pos[1] < cell_size_y) {
                return source;
            }
            // else: not in such a row, stay in the same state
            return cell->state();
        };


        // -- Initialization -- //
        this->_log->info("Initializing cells in '{}' mode ...", initial_state);

        // -- Trees -- //
        // First initialize the forest – set an initial tree density or no
        // trees at all.
        if (initial_state == "empty"){
            // Set all cell states to empty
            apply_rule([](const auto&){return empty;}, _manager.cells());
        }
        else if (initial_state == "init_density") {
            // Set trees on the cells with probability initial_density
            apply_rule(become_tree_with_prob, _manager.cells());
        }
        else {
            throw std::invalid_argument("The initial state '" + initial_state
                + "' is not valid! Valid options: 'empty' and 'init_density'");
        }

        // -- Stones -- //
        if (stones){
            if (stone_init == "random"){
                // Copy cells and shuffle them to randomize cluster formation
                auto cells_shuffled = _manager.cells();
                std::shuffle(cells_shuffled.begin(),cells_shuffled.end(), 
                             *this->_rng);

                // Set stones randomly, then cluster them
                apply_rule(init_stones, cells_shuffled);
                apply_rule(init_stone_clusters, cells_shuffled);
            }
            else {
                throw std::invalid_argument("The stone initialization '"
                    + stone_init + "' is not valid! Valid options: 'random'");
            }
        }
        else {
            this->_log->debug("Not using stones.");
        }


        // -- Infection source -- //
        // Different initializations for possible infection sources
        if (infection_source){
            if (infection_source_loc == "south"){
                // Set infection source at the southern border of the grid
                apply_rule(set_infection_source_south, _manager.cells());
            }
            else {
                throw std::invalid_argument("The infection source value '"
                    + infection_source_loc + "' is not valid! "
                    "Valid options: 'south'");
            }
        }
        else {
            this->_log->debug("Not using an infection source.");
        }

        // Write information that cells are initialized to the logger
        this->_log->info("Cells initialized.");
    }


public:
    // Runtime functions ......................................................

    /// Iterate a single time step
    /** \detail This updates all cells (synchronously) according to the
      *         _update rule. For specifics, see there.
      */
    void perform_step () {
        // Apply the update rule to all cells.
        apply_rule(_update, _manager.cells());
        // NOTE The cell state is updated only after all cells have been
        //      visited and know their state for the next time step.
    }


    /// Monitor model information
    /** \detail Supplies the `densities` array to the monitor.
      */
    void monitor () {        
        update_densities();
        this->_monitor.set_entry("densities", _densities);
    }


    /// Write data
    /** \detail Writes out the cell state and the densities of cells with the
      *         states empty, tree, or infected (i.e.: those that may change)
      */
    void write_data () {
        // Write the cell state
        _dset_state->write(_manager.cells().begin(), _manager.cells().end(),
                           [](const auto& cell) {
                             return static_cast<unsigned short>(cell->state());
                           });
        
        // state densities
        update_densities();
        _dset_density_empty->write(_densities[0]);
        _dset_density_tree->write(_densities[1]);
        _dset_density_infected->write(_densities[2]);
    }

};


} // namespace ContDisease
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_CONTDISEASE_HH
