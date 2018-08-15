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

/// Define initial state variable to use in setup function
State state0 = {};

/// Boundary condition type
struct Boundary {};
// NOTE if you do not use the boundary condition type, you can delete the
//      definition of the struct above and the passing to the type helper


/// Typehelper to define data types of CopyMeBare model 
using CopyMeBareModelTypes = ModelTypes<State, Boundary>;


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
    
    /// Data type of the state
    using Data = typename Base::Data;
    
    /// Cell type
    using CellType = typename ManagerType::Cell;

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
    // Base members: _time, _name, _cfg, _hdfgrp, _rng

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


    /// Write data
    void write_data ()
    {   
        // State which data should be written
    }
};

} // namespace CopyMeBare
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_COPYMEBARE_HH
