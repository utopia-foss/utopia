#ifndef UTOPIA_MODELS_SANDPILE_HH
#define UTOPIA_MODELS_SANDPILE_HH

#include <functional>
#include <set>

#include <utopia/core/model.hh>
#include <utopia/core/apply.hh>
#include <utopia/core/cell_manager.hh>


namespace Utopia {
namespace Models {
namespace SandPile {

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Type of the slope
using Slope = unsigned int;

/// Cell State for the SandPile model
struct State {
    /// The current value of the slope
    Slope slope;

    /// Whether the cell was touched by an avalanche; useful for updating
    bool in_avalanche;

    /// Default constructor
    State() = delete;

    /// Configuration based constructor
    template<typename RNG>
    State(const DataIO::Config& cfg, const std::shared_ptr<RNG>& rng)
    :
    in_avalanche{0}
    {
        // Read in the initial slope range
        auto init_slope_range = get_as<std::pair<unsigned, unsigned>>
                            ("initial_slope_range", cfg);

        // Make sure the parameters are valid
        if (init_slope_range.second <= init_slope_range.first) {
            throw std::invalid_argument("The `init_slope_range` parameter needs "
                "to specify a valid range, i.e. with first entry strictly "
                "smaller than the second one!");
        }

        // Depending on initial slope, set the initial slope of all cells to
        // a random value in that interval
        std::uniform_int_distribution<unsigned>
            dist(init_slope_range.first, init_slope_range.second);

        // Set the initial slopes that are not relaxed, yet.
        slope = dist(*rng);
    }
};

/// Cell traits specialization using the state type
/** \detail The first template parameter specifies the type of the cell state,
  *         the second sets them to be manually updated.
  *         The third argument sets the use of the default constructor.
  */
using CellTraits = Utopia::CellTraits<State, Update::manual, false>;


/// The Model type traits
using SandPileTypes = ModelTypes<>;


// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The SandPile Model
/** The SandPile model simulates a sandpile under the influence of new grains
 *  of sand that get added every iteration. The sand reaches a critical
 *  state _critical_slope, after which it collapses, passing sand on to 
 *  the neighboring cells
 */

class SandPile:
    public Model<SandPile, SandPileTypes>
{
public:
    /// The base model's type
    using Base = Model<SandPile, SandPileTypes>;
    
    /// The type of the cell manager
    using CellManager = Utopia::CellManager<CellTraits, SandPile>;
    
    /// Cell type
    using Cell = typename CellManager::Cell;

    /// Cell container type
    using CellContainer = std::vector<std::shared_ptr<Cell>>;

    /// Supply a type for rule functions that are applied to cells
    using RuleFunc = typename CellManager::RuleFunc;
    
    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// The uniform integer distribution type to use
    using UniformIntDist = typename std::uniform_int_distribution<std::size_t>;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor, _space
    // ... but you should definitely check out the documentation ;)

    // -- Members -------------------------------------------------------------
    /// The grid manager
    CellManager _cm;

    // -- Model parameters -- //
    /// The critical slope of the cells
    const Slope _critical_slope;

    /// The size of the last avalanche
    std::size_t _last_avalanche_size;   


    // .. Temporary objects ...................................................
    /// A distribution to select a random cell
    UniformIntDist _cell_distr;


    // .. Datasets ............................................................
    /// Dataset to store the slopes of all cells for all time steps
    const std::shared_ptr<DataSet> _dset_slope;

    /// Dataset to store the avalanche state of all cells for all time steps
    const std::shared_ptr<DataSet> _dset_avalanche;

    /// Dataset to store the avalanche size for each time step
    const std::shared_ptr<DataSet> _dset_avalanche_size;


public:
    // -- Model Setup ---------------------------------------------------------
    /// Construct the SandPile model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template<class ParentModel>
    SandPile (const std::string name,
                   ParentModel &parent)
    :
        // Initialize first via base model
        Base(name, parent),

        // Initialize the cell manager, binding it to this model
        _cm(*this),

        // Initialize other class members
        _critical_slope(get_as<Slope>("critical_slope", _cfg)),
        _last_avalanche_size{0},

        // Initialize the distribution such that a random cell can be selected
        _cell_distr(0, _cm.cells().size() - 1),
        
        // create datasets
        _dset_slope(this->create_cm_dset("slope", _cm)),
        _dset_avalanche(this->create_cm_dset("avalanche", _cm)),
        _dset_avalanche_size(this->create_dset("avalanche_size", {}))
    {}


private:
    // .. Helper functions ....................................................
    /// Select a random cell and increase its slope
    decltype(auto) add_sand_grain() {
        // Select a random cell to be modified
        auto cell = _cm.cells()[_cell_distr(*this->_rng)];

        // Adjust that cell's state
        cell->state.slope += 1;
        cell->state.in_avalanche = true;

        return cell;
    };
    
    /// Calculate the avalanche size
    /** \detail Loop through all cells and increase the _last_avalanche_size
     *          counter for each cell that has been marked 
     *          in_avalanche=true.
     */
    void calculate_avalanche_size() {
        // Reset counter
        _last_avalanche_size = 0;

        // Loop and collect toppled cells
        for (const auto& cell : _cm.cells()){
            if (cell->state.in_avalanche == true){
                ++_last_avalanche_size;
            }
        }
    }

    // .. Dynamic functions ...................................................
    /// Topple cells if the critical slope is exceeded
    /** \detail Starting from the first_cell, every time a cell topples the
     *          neighbors are also checked whether they need to topple.
     * 
     * \param first_cell The first cell from which the topple avalanche starts
     */
    void _topple(const std::shared_ptr<Cell>& first_cell){
        this->log->info("Toppling sand grains ...");

        // Create a queue that stores all the cells that need to topple
        std::queue<std::shared_ptr<Cell>> queue;
        queue.push(first_cell);

        while(not queue.empty()){
            // Get the first element from the queue, extract its state and 
            // remove it from the queue.
            const auto& cell = queue.front();
            auto& state = cell->state;
            queue.pop();
            
            // A cell will topple only its slope is greater than the critical
            // slope.
            if (state.slope > _critical_slope){
                state.in_avalanche = true;
                state.slope -= 4;

                // Add grains (=slopes) to the neighbors
                for (const auto& nb : _cm.neighbors_of(cell)){
                    nb->state.slope += 1;
                    queue.push(nb);
                }
            }
        }
    }

    // .. Rule functions ......................................................
    // Define functions that can be applied to the cells of the grid

    /// Resets cells for the next iteration
    /** \detail Marks cell as untouched by the avalanche and updates the slope
      *         to the cached future slope
      */
    RuleFunc _reset = [](const auto& cell){
        cell->state.in_avalanche = false;

        return cell->state;
    };


public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................
    /// Perform an iteration step
    void perform_step () {
        // Reset cells: All cells are not touched by an avalanche
        apply_rule<Update::async, Shuffle::off>(_reset, _cm.cells(), *this->_rng);

        // Add a grain of sand
        const auto& cell = add_sand_grain();

        // Let all cells topple until a relaxed state is reached
        _topple(cell);

        calculate_avalanche_size();
    }


    /// Supply monitor information to the frontend
    /** \detail Provides the mean_slope and model_is_active entries.
      */
    void monitor () {
        // Supply the last avalanche size to the monitor
        this->_monitor.set_entry("avalanche_size", _last_avalanche_size);
    }


    /// Write the cell slope and avalanche flag to the datasets
    void write_data () {
        // Write the slope of all cells
        _dset_slope->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return cell->state.slope;
            }
        );

        // Write a mask of whether a cell was touched by an avalanche
        _dset_avalanche->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return static_cast<unsigned short int>(
                            cell->state.in_avalanche);
            }
        );

        // Write the avalanche size
        _dset_avalanche_size->write(_last_avalanche_size);
    }   
};

} // namespace SandPile
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_SANDPILE_HH
