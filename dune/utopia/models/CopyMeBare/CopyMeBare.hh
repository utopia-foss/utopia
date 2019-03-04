#ifndef UTOPIA_MODELS_COPYMEBARE_HH
#define UTOPIA_MODELS_COPYMEBARE_HH
// TODO Adjust above include guard (and at bottom of file)

// standard library includes
#include <random>

// third-party library includes

// Utopia-related includes
#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/types.hh>


namespace Utopia {
namespace Models {
namespace CopyMeBare {

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Type helper to define types used by the model
using ModelTypes = Utopia::ModelTypes<>;


// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++
/// The CopyMeBare Model; the bare-basics a model needs
/** TODO Add your class description here.
 *  ...
 */
class CopyMeBare:
    public Model<CopyMeBare, ModelTypes>
{
public:
    /// The type of the Model base class of this derived class
    using Base = Model<CopyMeBare, ModelTypes>;
    
    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename Base::DataGroup;
    
    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    
private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor, _space
    // ... but you should definitely check out the documentation ;)

    // -- Members -------------------------------------------------------------


    // .. Temporary objects ...................................................

    
    // .. Datasets ............................................................
    // NOTE They should be named '_dset_<name>', where <name> is the
    //      dataset's actual name as set in its constructor. Ideally, do not
    //      hide them inside a struct ...
    // std::shared_ptr<DataSet> _dset_my_var;


public:
    // -- Model Setup ---------------------------------------------------------
    /// Construct the CopyMeBare model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     */
    template<class ParentModel>
    CopyMeBare (const std::string name, ParentModel& parent)
    :
        // Initialize first via base model
        Base(name, parent)
        // Now initialize members specific to this class
        // ...

        // Open the datasets
        // e.g. via _dset_state(this->create_dset("state", {})) <- 1d
        //      or  _dset_state(this->create_dset("state", {num_states})) <- 2d
    {
        // Can do remaining initialization steps here ...
        // ...

        // Write out the initial state
        this->write_data();
    }


private:
    // .. Setup functions .....................................................
   
    // .. Helper functions ....................................................

public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Iterate a single step
    /** \detail Here you can add a detailed description what exactly happens 
      *         in a single iteration step
      */
    void perform_step () {
        
    }


    /// Monitor model information
    /** \detail Here, functions and values can be supplied to the monitor that
     *          are then available to the frontend. The monitor() function is
     *          _only_ called if a certain emit interval has passed; thus, the
     *          performance hit is small. 
     */
    void monitor () {
        // Can supply information to the monitor here in two ways:
        // this->_monitor.set_entry("key", value);
        // this->_monitor.set_entry("key", [this](){return 42.;});
    }


    /// Write data
    /** \detail This function is called to write out data. It should be called
      *         at the end of the model constructor to write out the initial
      *         state. After that, the configuration determines at which times
      *         data is written.
      *         See \ref Utopia::DataIO::Dataset::write
      */
    void write_data () {
        // Example:
        // _dset_foo->write(it.begin(), it.end(),
        //     [](const auto& element) {
        //         return element.get_value();
        // });
    }

    
    // Getters and setters ....................................................
    // Add getters and setters here to interface with other model
    
};

} // namespace CopyMeBare
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_COPYMEBARE_HH
