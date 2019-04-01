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

    /// The next value of the slope
    Slope future_slope;

    /// Whether the cell was touched by an avalanche; useful for updating
    bool touched_by_avalanche;

    /// Default constructor
    State() = delete;

    /// Configuration based constructor
    template<typename RNG>
    State(const DataIO::Config& cfg, const std::shared_ptr<RNG>& rng)
    :
    touched_by_avalanche{0}
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
        auto init_slope = dist(*rng);
        slope = init_slope;
        future_slope = init_slope;
    }
};

/// Cell traits specialization using the state type
/** \detail The first template parameter specifies the type of the cell state,
  *         the second sets them to be asynchronously updated.
  *         The third argument sets the use of the default constructor.
  */
using CellTraits = Utopia::CellTraits<State, Update::async, false>;


/// The Model type traits
using SandPileModelTypes = ModelTypes<>;


// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The SandPile Model
/** The SandPile model simulates a sandpile under the influence of new grains
 *  of sand that get added every iteration. The sand reaches a critical
 *  state _critical_slope, after which it collapses, passing sand on to 
 *  the neighboring cells
 */

class SandPileModel:
    public Model<SandPileModel, SandPileModelTypes>
{
public:
    /// The base model's type
    using Base = Model<SandPileModel, SandPileModelTypes>;
    
    /// The type of the cell manager
    using CellManager = Utopia::CellManager<CellTraits, SandPileModel>;
    
    /// Cell type
    using CellType = typename CellManager::Cell;

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
    /// Temporary and re-used object to store activated cells
    /** \detail This saves re-allocation in SandPileModel::perform_step
      */
    std::set<std::shared_ptr<CellType>> _activated_cells;
    
    /// The activated cells for the next iteration step
    std::set<std::shared_ptr<CellType>> _future_activated_cells;

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
    SandPileModel (const std::string name,
                   ParentModel &parent)
    :
        // Initialize first via base model
        Base(name, parent),

        // Initialize the cell manager, binding it to this model
        _cm(*this),

        // Initialize other class members
        _critical_slope(get_as<Slope>("critical_slope", _cfg)),
        _last_avalanche_size{0},

        // Initialize containers empty; are populated in initialize_cells()
        _activated_cells(),
        _future_activated_cells(),

        // Initialize the distribution such that a random cell can be selected
        _cell_distr(0, _cm.cells().size() - 1),

        // create datasets
        _dset_slope(this->create_cm_dset("slope", _cm)),
        _dset_avalanche(this->create_cm_dset("avalanche", _cm)),
        _dset_avalanche_size(this->create_dset("avalanche_size", {}))
    {
        // Call the method that initializes the cells
        initialize_cells();
        this->_log->debug("{} model fully set up.", this->_name);

        // Write initial state
        this->write_data();
        this->_log->debug("Initial state written.");
    }


private:
    // .. Setup functions .....................................................
    /// Initialize the cells according to `initial_state` config parameter
    void initialize_cells() {        
        // Mark all cells as activated by copying them into _activated_cells
        std::copy(_cm.cells().begin(), _cm.cells().end(),
                  std::inserter(_activated_cells, _activated_cells.end()));
        
        // As long as there are activated cells ...
        while (not _activated_cells.empty()) {
            // ... let all the activated cells topple their sand.
            apply_rule<false>(_topple_cell, _activated_cells);
            
            /** The "old" active cells are not needed any more because of the 
              * "future" active cells that are selected in the _topple_cell
              * function of the rule applied above. Now, update the
              * activated_cells with the ones for the next loop iteration.
              */
            _activated_cells = _future_activated_cells;
            _future_activated_cells.clear();
            
            // Reset all cells
            apply_rule<false>(_reset_cell, _cm.cells());
        }
    }


    // .. Helper functions ....................................................
    /// Select a random cell and increase its slope
    void add_sand_grain() {
        // Select a random cell to be modified
        auto cell = _cm.cells()[_cell_distr(*this->_rng)];

        // Adjust that cell's state
        cell->state().slope += 1;
        cell->state().future_slope += 1;
        cell->state().touched_by_avalanche = true;

        // Add it to the temporary container of activated cells
        _activated_cells.insert(cell);
    };


    /// Check whether the sand pile is active
    /** \detail Returns true if any cell has a slope higher as the critical
      *         slope.
      */
    bool model_is_active() const {
        return std::any_of(_cm.cells().begin(), _cm.cells().end(),
            [this](const auto& cell) -> bool {
                return cell->state().slope > this->_critical_slope;
            }
        );
    };


    /// Calculates the mean slope
    double mean_slope() const {
        return (std::accumulate(_cm.cells().begin(), _cm.cells().end(), 0.,
                                [](double s, const auto& cell){
                                    return s + ((double) cell->state().slope);
                                }
                ) / _cm.cells().size());
    }

    
    /// Calculate the avalanche size
    /** \detail Loop through all cells and increase the _last_avalanche_size
     *          counter for each cell that has been marked 
     *          touched_by_avalanche=true.
     */
    void calculate_avalanche_size() {
        // Reset counter
        _last_avalanche_size = 0;

        // Loop and collect toppled cells
        for (const auto& cell : _cm.cells()){
            if (cell->state().touched_by_avalanche == true){
                ++_last_avalanche_size;
            }
        }
    }


    // .. Rule functions ......................................................
    // Define functions that can be applied to the cells of the grid

    /// Check if a cell is active and, if so, topple it
    RuleFunc _topple_cell = [this](const auto& cell){
        // A cell will topple only if beyond the critical slope
        if (cell->state().slope > _critical_slope){
            cell->state().touched_by_avalanche = true;
            cell->state().future_slope -= _critical_slope;

            // Update all neighbors by increasing the slope of the next
            // iteration step by 1. Application happens in random order.
            apply_rule(_update_neighborhood,
                       _cm.neighbors_of(cell), *this->_rng);
        }

        return cell->state();
    };

    /// Updates the neighborhood of a toppled cell
    /** \detail This is called from _topple_cell
      */
    RuleFunc _update_neighborhood = [this](const auto& cell){
        // Get the cell state
        auto& state = cell->state();

        // Increase the slope of the next iteration round and mark the cell as
        // touched by the avalanche
        state.future_slope += 1;
        state.touched_by_avalanche = true;

        // If the slope of the next iteration is greater than the critical
        // slope, store the cell in the next active cells container
        if (state.future_slope > this->_critical_slope){
            _future_activated_cells.insert(cell);
        }

        return state;
    };

    /// Update the slope of the cell to its future value
    RuleFunc _update_cell_slope = [](const auto& cell){
        cell->state().slope = cell->state().future_slope;
        return cell->state();
    };

    /// Resets cells for the next iteration
    /** \detail Marks cell as untouched by the avalanche and updates the slope
      *         to the cached future slope
      */
    RuleFunc _reset_cell = [](const auto& cell){
        cell->state().touched_by_avalanche = false;
        cell->state().slope = cell->state().future_slope;

        return cell->state();
    };


public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................
    /// Perform an iteration step
    void perform_step () {
        // Reset all cells (cells shuffle=false)
        apply_rule<false>(_reset_cell, _cm.cells());

        // Add a grain of sand
        add_sand_grain();

        // As long as there are activated cells from the previous iteration,
        // let all the avalanches they initiate go.
        while (not _activated_cells.empty()) {
            // Let all the activated cells topple their sand
            apply_rule<false>(_topple_cell, _activated_cells);

            // Get the (cached) activated cells of the previous iteration step
            // and prepare the future cache container by clearing it
            _activated_cells = _future_activated_cells;
            _future_activated_cells.clear();
            
            // Update the cell slope from the cached future slope of the last
            // iteration step. 
            apply_rule<false>(_update_cell_slope, _cm.cells());   
        }

        calculate_avalanche_size();
    }


    /// Supply monitor information to the frontend
    /** \detail Provides the mean_slope and model_is_active entries.
      */
    void monitor () {
        // Supply the mean slope to the monitor
        this->_monitor.set_entry("mean_slope", mean_slope());

        // ...and whether the model is active, i.e. if any cell will topple.
        this->_monitor.set_entry("model_is_active", model_is_active());

        // Supply the last avalanche size to the monitor
        this->_monitor.set_entry("avalanche size", _last_avalanche_size);
    }


    /// Write the cell slope and avalanche flag to the datasets
    void write_data () {
        // Write the slope of all cells
        _dset_slope->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return cell->state().slope;
            }
        );

        // Write a mask of whether a cell was touched by an avalanche
        _dset_avalanche->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return static_cast<unsigned short int>(
                            cell->state().touched_by_avalanche);
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
