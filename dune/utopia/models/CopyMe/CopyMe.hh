#ifndef UTOPIA_MODELS_COPYME_HH
#define UTOPIA_MODELS_COPYME_HH
// TODO Adjust above include guard (and at bottom of file)

// standard library includes
#include <random>

// third-party library includes

// Utopia-related includes
#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/cell_manager.hh>
#include <dune/utopia/core/apply.hh>


namespace Utopia {
namespace Models {
namespace CopyMe {

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The type of a cell's state
struct CellState {
    /// A useful documentation string
    double some_state;

    /// Another useful documentation string, yeah
    int some_trait;

    /// Whether this cell is very important
    bool is_a_vip_cell;

    /// Construct the cell state from a configuration
    CellState(const DataIO::Config& cfg)
    :
        some_state(get_as<double>("some_state", cfg)),
        some_trait(get_as<int>("some_trait", cfg)),
        is_a_vip_cell(false)
    {}

    /// Construct the cell state from a configuration and an RNG
    template<class RNG>
    CellState(const DataIO::Config& cfg, const std::shared_ptr<RNG>& rng)
    :
        // Use the config constructor to get the values
        CellState(cfg)
    {
        // Do some more things using the random number generator
        // There is the possibility to set a random initial trait
        if (cfg["random_initial_trait"]) {
            if (not get_as<bool>("random_initial_trait", cfg)) {
                // Is set, but is false. Just return:
                return;
            }

            // Choose a random value between 0 and the current value
            some_trait = std::uniform_int_distribution(0, some_trait)(*rng);
        }
        // else: the config option was not available
    }
};


/// Specialize the CellTraits type helper for this model
/** \detail Specifies the type of each cells' state as first template argument
  *         and the update mode as second.
  *
  * See \ref Utopia::CellTraits for more information.
  */
using CellTraits = Utopia::CellTraits<CellState, UpdateMode::sync>;


/// Type helper to define types used by the model
using ModelTypes = Utopia::ModelTypes<>;


// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The CopyMe Model; a good start for a CA-based model
/** TODO Add your model description here.
 *  This model's only right to exist is to be a template for new models. 
 *  That means its functionality is based on nonsense but it shows how 
 *  actually useful functionality could be implemented.
 */
class CopyMe:
    public Model<CopyMe, ModelTypes>
{
public:
    /// The type of the Model base class of this derived class
    using Base = Model<CopyMe, ModelTypes>;
    
    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename Base::DataGroup;
    
    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Type of the CellManager to use
    using CellManager = Utopia::CellManager<CellTraits, CopyMe>;
    // NOTE that it requires the model's type as second template argument

    /// Extract the type of the rule function from the CellManager
    /** \detail This is a function that receives a reference to a cell and
      *         returns the new cell state. For more details, check out
      *         \ref Utopia::CellManager
      *
      * \note   Whether the cell state gets applied directly or
      *         requires a synchronous update depends on the update mode
      *         specified in the cell traits.
      */
    using RuleFunc = typename CellManager::RuleFunc;

    
private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor, _space
    // ... but you should definitely check out the documentation ;)

    // -- Members -------------------------------------------------------------
    /// The cell manager
    CellManager _cm;

    /// Some parameter
    double _some_parameter;

    // More parameters here ...


    /// A re-usable uniform real distribution to evaluate probabilities
    std::uniform_real_distribution<double> _prob_distr;

    // .. Temporary objects ...................................................

    
    // .. Datasets ............................................................
    // NOTE They should be named '_dset_<name>', where <name> is the
    //      dataset's actual name as set in its constructor. If you are using
    //      data groups, prefix them with _dgrp_<groupname>
    /// A dataset for storing all cells' some_state
    std::shared_ptr<DataSet> _dset_some_state;

    /// A dataset for storing all cells' some_trait
    std::shared_ptr<DataSet> _dset_some_trait;


public:
    // -- Public interface ----------------------------------------------------
    /// Construct the CopyMe model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     */
    template<class ParentModel>
    CopyMe (const std::string name, ParentModel &parent)
    :
        // Initialize first via base model
        Base(name, parent),

        // Now initialize the cell manager
        _cm(*this),

        // Initialize model parameters
        _some_parameter(get_as<double>("some_parameter", this->_cfg)),
        // ...

        // Initialize the uniform real distribution to range [0., 1.]
        _prob_distr(0., 1.),

        // Datasets
        // For setting up datasets that store CellManager data, you can use the
        // helper functions to take care of setting them up:
        _dset_some_state(this->create_cm_dset("some_state", _cm)),
        _dset_some_trait(this->create_cm_dset("some_trait", _cm))
        // NOTE To set up datasets that have a different shape, we suggest to
        //      use the Model::create_dset helper, which already takes care of
        //      using the correct length into the time dimension (depending on
        //      the num_steps and write_every parameters). The syntax is:
        //
        //        this->create_dset("mean_state", {})    // 1D {#writes}
        //        this->create_dset("a_vec", {num_cols}) // 2D {#writes, #cols}
    {
        // Can do remaining initialization steps here ...
        // Example:
        apply_rule([this](const auto& cell){
            auto state = cell->state();

            // Every 13th cell (on average) is a VIP cell
            if (this->_prob_distr(*this->_rng) < (1./13.)) {
                state.is_a_vip_cell = true;
            }

            return state;
        }, _cm.cells());
        // NOTE Compare this to the apply_rule calls in the perform_step method
        //      where a _stored_ lambda function is passed to it. For the setup
        //      done here, the function is only used once; thus, it makes more
        //      sense to just use a temporary lambda.
        this->_log->debug("VIP cells set up.");

        // Initialization should be finished here.
        this->_log->debug("{} model fully set up.", this->_name);

        // Write out the initial state
        this->write_data();
        this->_log->debug("Initial state written.");
    }

    // .. Setup functions .....................................................
    // Can add additional setup functions here ...


    // .. Rule functions ......................................................
    // Rule functions that can be applied to the CellManager's cells
    // NOTE The below are examples; delete and/or adjust them to your needs!
    //      Ideally, only define those rule functions as members that are used
    //      more than once.

    /// An interaction function of a single cell with its neighbors
    const RuleFunc _some_interaction = [this](const auto& cell){
        // Get the current state of the cell
        auto state = cell->state();

        // Increase some_state by one
        state.some_state += 1;

        // Iterate over all neighbors of the current cell
        for (auto& nb : this->_cm.neighbors_of(cell)) {
            // Obvious thing to do is to increase some_trait by the sum of
            // some_traits's of the neighbor. Sure thing.
            state.some_trait += nb->state().some_trait;

            // Let's add a random number in range [-1, +1] as well
            state.some_trait += (this->_prob_distr(*this->_rng) * 2. - 1.);
        }

        // Ahhh and obviously you need to divide some float by _some_parameter
        // because that makes totally sense
        state.some_trait /= this->_some_parameter;

        // Return the new cell state
        return state;
    };


    /// Some other rule function
    const RuleFunc _some_other_rule = [this](const auto& cell){
        // Get the current state of the cell
        auto state = cell->state();

        // With a probablity of 0.3 set the cell's state.some_state to 0
        if (this->_prob_distr(*this->_rng) < 0.3) {
            state.some_state = 0;
        }

        // Return the new state cell
        return state;
    };


    // .. Helper functions ....................................................

    /// Calculate the mean of all cells' some_state
    double calc_some_state_mean() const {
        double sum = 0.;
        for (const auto &cell : _cm.cells()) {
            sum += cell->state().some_state;
        }
        return sum / _cm.cells().size();
    }


    // .. Runtime functions ...................................................

    /// Iterate a single step
    /** \detail Here you can add a detailed description what exactly happens 
      *         in a single iteration step
      */
    void perform_step () {
        // Apply the rules to all cells, first the interaction, then the update
        apply_rule(_some_interaction, _cm.cells());
        apply_rule(_some_other_rule, _cm.cells());
    }


    /// Monitor model information
    /** \detail Here, functions and values can be supplied to the monitor that
     *          are then available to the frontend. The monitor() function is
     *          _only_ called if a certain emit interval has passed; thus, the
     *          performance hit is small.
     *          With this information, you can then define stop conditions on
     *          frontend side, that can stop a simulation once a certain set
     *          of conditions is fulfilled.
     */
    void monitor () {
        // Supply some number directly by value
        this->_monitor.set_entry("some_value", 42);
        this->_monitor.set_entry("state_mean", calc_some_state_mean());
    }


    /// Write data
    /** \detail This function is called to write out data. It should be called
      *         at the end of the model constructor to write out the initial
      *         state. After that, the configuration determines at which times
      *         data is written.
      *         See \ref Utopia::DataIO::Dataset::write
      */
    void write_data () {
        // Write out the some_state of all cells
        _dset_some_state->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return cell->state().some_state;
        });

        // Write out the some_trait of all cells
        _dset_some_trait->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return cell->state().some_trait;
        });
    }


    // .. Getters and setters .................................................
    // Add getters and setters here to interface with other models
    
};

} // namespace CopyMe
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_COPYME_HH
