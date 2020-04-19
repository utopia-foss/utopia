#ifndef UTOPIA_MODELS_GAMEOFLIFE_HH
#define UTOPIA_MODELS_GAMEOFLIFE_HH

// standard library includes
#include <random>
#include <string>

// third-party library includes

// Utopia-related includes
#include <utopia/core/model.hh>
#include <utopia/core/cell_manager.hh>
#include <utopia/core/apply.hh>


namespace Utopia::Models::GameOfLife {

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The type of a cell's state
struct CellState {
    /// Whether a cell lives or not
    bool living;

    /// Construct the cell state from a configuration
    CellState(const DataIO::Config& cfg)
    {}

    /// Construct the cell state from a configuration and an RNG
    template<class RNG>
    CellState(const DataIO::Config& cfg, const std::shared_ptr<RNG>& rng)
    {
        // Initialize cells randomly
        // NOTE This check can be improved if the string comparison is replaced
        //      e.g. by an enum class comparisson. However, reading in the 
        //      configuration directly as enum class generates errors because  
        //      because of not allowed type conversions.
        if (get_as<std::string>("mode", cfg) == "random") {
            if (not get_as<DataIO::Config>("random", cfg)) {
                // Is set, but is false. Just return:
                return;
            }

            // Get the probability to be living
            auto p_living = get_as<double>("p_living", cfg["random"]);

            // Create a uniform real distribution 
            auto dist = std::uniform_real_distribution<double>(0., 1.);

            // With probability p_living, a cell lives
            if (dist(*rng) < p_living){
                living = true;
            }
            else{
                living = false;
            }
        }
        // else: the config option was not available
    }
};


/// Specialize the CellTraits type helper for this model
/** Specifies the type of each cells' state as first template argument and the 
  * update mode as second.
  *
  * See \ref Utopia::CellTraits for more information.
  */
using CellTraits = Utopia::CellTraits<CellState, Update::sync>;


/// Type helper to define types used by the model
using ModelTypes = Utopia::ModelTypes<>;


// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The GameOfLife Model
/** This model implement Conways Game of in the Utopia way.
 */
class GameOfLife:
    public Model<GameOfLife, ModelTypes>
{
public:
    /// The type of the Model base class of this derived class
    using Base = Model<GameOfLife, ModelTypes>;

    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename Base::DataGroup;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Type of the CellManager to use
    using CellManager = Utopia::CellManager<CellTraits, GameOfLife>;
    // NOTE that it requires the model's type as second template argument

    /// Extract the type of the rule function from the CellManager
    /** This is a function that receives a reference to a cell and returns the 
      * new cell state. For more details, check out \ref Utopia::CellManager
      *
      * \note   Whether the cell state gets applied directly or
      *         requires a synchronous update depends on the update mode
      *         specified in the cell traits.
      */
    using RuleFunc = typename CellManager::RuleFunc;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor, _log, _space
    // ... but you should definitely check out the documentation ;)

    // -- Members -------------------------------------------------------------
    /// The cell manager
    CellManager _cm;


    // .. Temporary objects ...................................................


    // .. Datasets ............................................................
    /// A dataset for storing all cells living or dead status
    std::shared_ptr<DataSet> _dset_living;

public:
    // -- Model Setup ---------------------------------------------------------
    /// Construct the GameOfLife model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     */
    template<class ParentModel>
    GameOfLife (const std::string name, ParentModel &parent)
    :
        // Initialize first via base model
        Base(name, parent),

        // Now initialize the cell manager
        _cm(*this),

        // Initialize model parameters
        
        // Datasets
        // For setting up datasets that store CellManager data, you can use the
        // helper functions to take care of setting them up:
        _dset_living(this->create_cm_dset("living", _cm))
    {
        // Initialization should be finished here.
        this->_log->debug("{} model fully set up.", this->_name);
    }


private:
    // .. Setup functions .....................................................


    // .. Helper functions ....................................................

    /// Calculate the mean of all cells' some_state
    double calculate_living_cell_density () const {
        double sum = 0.;
        for (const auto &cell : _cm.cells()) {
            sum += cell->state().living;
        }
        return sum / _cm.cells().size();
    }


    // .. Rule functions ......................................................
    // Rule functions that can be applied to the CellManager's cells
    
    /// Implement the Game of Life rules
    const RuleFunc _live_and_let_die = [this](const auto& cell){
        // Get the current state of the cell
        auto state = cell->state();

        // TODO implement GoL dynamics


        // Return the new cell state
        return state;
    };


public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Iterate a single step
    void perform_step () {
        // Apply the rules to all cells, first the interaction, then the update
        apply_rule(_live_and_let_die, _cm.cells());
    }


    /// Monitor model information
    /** \details Here, functions and values can be supplied to the monitor that
     *           are then available to the frontend. The monitor() function is
     *           _only_ called if a certain emit interval has passed; thus, the
     *           performance hit is small.
     *           With this information, you can then define stop conditions on
     *           frontend side, that can stop a simulation once a certain set
     *           of conditions is fulfilled.
     */
    void monitor () {
        this->_monitor.set_entry("state_mean", calculate_living_cell_density());
    }


    /// Write data
    /** \details This function is called to write out data.
      *          The configuration determines the times at which it is invoked.
      *          See \ref Utopia::DataIO::Dataset::write
      */
    void write_data () {
        // Write out the some_state of all cells
        _dset_living->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return static_cast<unsigned short>(cell->state().living);
        });
    }


    // .. Getters and setters .................................................
    // Add getters and setters here to interface with other models

};

} // namespace Utopia::Models::GameOfLife

#endif // UTOPIA_MODELS_GAMEOFLIFE_HH
