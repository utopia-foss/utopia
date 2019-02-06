#ifndef UTOPIA_MODELS_SANDPILE_HH
#define UTOPIA_MODELS_SANDPILE_HH

#include <functional>
#include <set>

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>
#include <dune/utopia/core/types.hh>


namespace Utopia {
namespace Models {
namespace SandPile {


/// Type of the slope
using Slope = std::size_t;

/// The Model traits
using SandPileModelTypes = ModelTypes<>;


/// State struct for the SandPile model
struct State {
    /// The current value of the slope
    Slope slope;

    /// The next value of the slope
    Slope future_slope;

    /// Whether the cell was touched by an avalanche; useful for updating
    bool touched_by_avalanche = false;
};


/// The datasets
template <typename DataSet>
struct DataSets {
    /// Dataset to store the slopes of all cells for all time steps
    const std::shared_ptr<DataSet> slope;

    /// Dataset to store the avalanche state of all cells for all time steps
    const std::shared_ptr<DataSet> avalanche;
};


/// The SandPile Model
/** The SandPile model simulates a sandpile under the influence of new grains
 *  of sand that get added every iteration. The sand reaches a critical
 *  state _critical_slope, after which it collapses, passing sand on to 
 *  the neighboring cells
 */
template<class ManagerType>
class SandPileModel:
    public Model<SandPileModel<ManagerType>, SandPileModelTypes>
{
public:
    /// The base model's type
    using Base = Model<SandPileModel<ManagerType>, SandPileModelTypes>;
    
    /// Cell type
    using CellType = typename ManagerType::Cell;

    /// Supply a type for rule functions that are applied to cells
    using RuleFunc = typename std::function<State(std::shared_ptr<CellType>)>;
    
    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Data type of the shared RNG
    using RNG = typename Base::RNG;

    /// Cells use the von-Neumann neighborhood
    /** \note When using MooreNeighbor, take care to change the _topple_cell
      *       function such that the change in level is correct.
      */
    using Neighbor = Utopia::Neighborhoods::NextNeighbor;

    /// The uniform distribution type to use
    using UniformDist = std::uniform_int_distribution<std::size_t>;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor

    // -- Members of this model -- //
    /// The grid manager
    ManagerType _manager;

    // -- Model parameters -- //
    /// The critical slope of the cells
    const Slope _critical_slope;

    /// The range of initial slopes of the cells
    const std::pair<Slope, Slope> _initial_slope;


    // -- Temporary objects -- //
    /// Temporary and re-used object to store activated cells
    /** \detail This saves re-allocation in SandPileModel::perform_step
      */
    std::set<std::shared_ptr<CellType>> _activated_cells;
    
    /// The activated cells for the next iteration step
    std::set<std::shared_ptr<CellType>> _future_activated_cells;
    

    // -- Datasets -- //
    /// Struct of datasets
    DataSets<DataSet> _dsets;


    // -- Helper functions // 
    /// Select a random cell and increase its slope
    void _add_sand_grain() {
        // Select a random cell to be modified
        UniformDist dist(0, this->_manager.cells().size() - 1);
        auto cell = this->_manager.cells()[dist(*this->_rng)];

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
    bool _model_is_active() {
        return std::any_of(this->_manager.cells().begin(),
                           this->_manager.cells().end(),
                           [this](const auto& cell) -> bool {
                                return cell->state().slope > _critical_slope;
                            });
    };

    double _mean_slope() {
        return (  std::accumulate(this->_manager.cells().begin(),
                                  this->_manager.cells().end(),
                                  0.,
                                  [](double s, const auto& cell){
                                    return s + ((double) cell->state().slope);
                                  })
                / this->_manager.cells().size());
    }


    // -- Rule functions -- //
    // Define functions that can be applied to the cells of the grid

    /// Initialize cell to a random slope value within the _initial_slope range
    RuleFunc _set_initial_state = [this](const auto& cell){
        UniformDist dist(_initial_slope.first, _initial_slope.second);

        cell->state().slope = dist(*this->_rng);
        cell->state().future_slope = cell->state().slope;

        return cell->state();
    };

    /// Check if a cell is active and, if so, topple it
    RuleFunc _topple_cell = [this](const auto& cell){
        // A cell will topple only if beyond the critical slope
        if (cell->state().slope > _critical_slope){
            cell->state().touched_by_avalanche = true;
            cell->state().future_slope -= _critical_slope;

            // Update all neighbors by increasing the slope of the next
            // iteration step by 1.
            apply_rule(_update_neighborhood,
                       Neighbor::neighbors(cell, this->_manager), *this->_rng);
        }

        return cell->state();
    };

    /// Updates the neighborhood of a toppled cell
    /** \detail This is called from _toplle_cell
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
    /// Construct the SandPile model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template<class ParentModel>
    SandPileModel (const std::string name,
                   ParentModel &parent,
                   ManagerType&& manager)
    :
        // Initialize first via base model
        Base(name, parent),

        // Now initialize members specific to this class
        _manager(manager),
        _critical_slope(as_<Slope>(this->_cfg["critical_slope"])),
        _initial_slope(as_<std::pair<Slope, Slope>>(this->_cfg["initial_slope"])),

        // create datasets
        _dsets({this->create_dset("slope", {_manager.cells().size()}),
                this->create_dset("avalanche", {_manager.cells().size()})})
    {
        // Make sure the parameters are valid
        if (_initial_slope.second <= _initial_slope.first) {
            throw std::invalid_argument("The `_initial_slope` parameter needs "
                "to specify a valid range, i.e. with first entry strictly "
                "smaller than the second one!");
        }

        // Call the method that initializes the cells
        this->initialize_cells();

        // Write initial state
        this->write_data();

        // Get the shape of the grid to supply that info to the dataset attrs
        const auto grid_shape = as_<std::array<std::size_t,2>>(this->_cfg["grid_size"]);

        // Add attributes to the datasets
        _dsets.slope->add_attribute("content", "grid");
        _dsets.slope->add_attribute("grid_shape", grid_shape);
        _dsets.avalanche->add_attribute("content", "grid");
        _dsets.avalanche->add_attribute("grid_shape", grid_shape);
    }

    // Setup functions ........................................................
    /// Initialize the cells according to `initial_state` config parameter
    void initialize_cells()
    {
        // Get the cells container from the manager
        auto& cells = _manager.cells();

        // Depending on the size of the grid, adjust the log message.
        if (cells.size() <= 64*64) {
            this->_log->info("Initializing cells...");
        }
        else {
            this->_log->info("Initializing cells... This may take a while.");
        }

        // Apply a rule to all cells depending on the config value.
        // NOTE This uses shuffle=false because when using a set it is
        //      required to change the ordering, which is not possible.
        apply_rule<false>(_set_initial_state, cells);
        
        // Activate all cells by copying them into the _activated_cells 
        // container.
        std::copy(cells.begin(), cells.end(),
                  std::inserter(_activated_cells, _activated_cells.end()));

        // As long as there are activated cells
        while (not _activated_cells.empty()) {
            // Let all the activated cells topple their sand.
            apply_rule<false>(_topple_cell, _activated_cells);
            
            /** The "old" active cells are not needed any more because of the 
              * "future" active cells that are selected in the _topple_cell
              * function of the rule applied above. Now, update the
              * activated_cells with the ones for the next loop iteration.
              */
            _activated_cells = _future_activated_cells;
            _future_activated_cells.clear();
            
            // Reset all cells
            apply_rule<false>(_reset_cell, cells);
        }

        // Write information that cells are initialized to the logger
        this->_log->info("Cells initialized.");
    }

    // Runtime functions ......................................................

    /// Perform an iteration step
    void perform_step ()
    {
        // Reset all cells (cells shuffle=false)
        apply_rule<false>(_reset_cell, _manager.cells());

        // Add a sand grain
        _add_sand_grain();

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
            apply_rule<false>(_update_cell_slope, _manager.cells());   
        }
    }


    /// Supply monitor information to the frontend
    /** \detail Provides the mean_slope and model_is_active entries.
      */
    void monitor ()
    {
        // Supply the mean slope to the monitor
        this->_monitor.set_entry("mean_slope", _mean_slope());

        // ...and whether the model is active, i.e. if any cell will topple.
        this->_monitor.set_entry("model_is_active", _model_is_active());
    }


    /// Write the cell slope and avalanche flag to the datasets
    void write_data ()
    {   
        // Write the slope of all cells
        _dsets.slope->write(_manager.cells().begin(),
                            _manager.cells().end(),
                            [](const auto& cell) {
                                return cell->state().slope;
                            });

        // Write a mask of whether a cell was touched by an avalanche
        _dsets.avalanche->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](const auto& cell) {
                                    return static_cast<unsigned short int>
                                        (cell->state().touched_by_avalanche);
                                });
    }   
};

} // namespace SandPile
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_SANDPILE_HH
