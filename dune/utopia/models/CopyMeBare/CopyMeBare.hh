#ifndef UTOPIA_MODELS_COPYMEBARE_HH
#define UTOPIA_MODELS_COPYMEBARE_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>
#include <dune/utopia/core/types.hh>

#include <functional>


namespace Utopia {
namespace Models {
namespace CopyMeBare {


/// State struct for CopyMeBare model. 
struct State {
    // TODO Define the state a cell of this model can have
};


/// Typehelper to define types of CopyMeBare model 
using CopyMeBareModelTypes = ModelTypes<>;


/// The CopyMeBare Model
/** Add your class description here.
 *  ...
 */
template<class ManagerType>
class CopyMeBareModel:
    public Model<CopyMeBareModel<ManagerType>, CopyMeBareModelTypes>
{
public:
    /// The base model type
    using Base = Model<CopyMeBareModel<ManagerType>, CopyMeBareModelTypes>;
    
    /// Cell type
    using CellType = typename ManagerType::Cell;

    /// Supply a type for rule functions that are applied to cells
    using RuleFunc = typename std::function<State(std::shared_ptr<CellType>)>;

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
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor

    // -- Members of this model -- //
    /// The grid manager
    ManagerType _manager;


    // -- Temporary objects -- //

    
    // -- Datasets -- //
    // NOTE They should be named '_dset_<name>', where <name> is the
    //      dataset's actual name as set in the constructor.
    // std::shared_ptr<DataSet> _dset_my_var;


    // -- Rule functions -- //
    // Define functions that can be applied to the cells of the grid

public:
    /// Construct the CopyMeBare model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template<class ParentModel>
    CopyMeBareModel (const std::string name,
                     ParentModel &parent,
                     ManagerType&& manager)
    :
        // Initialize first via base model
        Base(name, parent),
        // Now initialize members specific to this class
        _manager(manager)
        // Open the datasets
        // e.g. via this->_hdfgrp->open_dataset("my_var")
    {
        // Initialize grid cells

        // Set dataset capacities

        // Write out the initial state
    }

    // Setup functions ........................................................
   

    // Runtime functions ......................................................

    /** @brief Iterate a single step
     *  @detail Here you can add a detailed description what exactly happens 
     *          in a single iteration step
     */
    void perform_step ()
    {
        // Apply the rules or an iteration step
    }


    /// Monitor model information
    /** @detail Here, functions and values can be supplied to the monitor that
     *          are then available to the frontend. The monitor() function is
     *          _only_ called if a certain emit interval has passed; thus, the
     *          performance hit is small. Also, if using set_by_func, the given
     *          lambda will only be called if an emission will happen.
     */
    void monitor ()
    {
        // Can supply information to the monitor here in two ways:
        // this->_monitor.set_entry("key", value);
        // this->_monitor.set_entry("key", [this](){return 42.;});
    }


    /// Write data
    void write_data ()
    {   
        // State which data should be written
    }

    
    // Getters and setters ....................................................
    // Add getters and setters here to interface with other model
    
};

} // namespace CopyMeBare
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_COPYMEBARE_HH
