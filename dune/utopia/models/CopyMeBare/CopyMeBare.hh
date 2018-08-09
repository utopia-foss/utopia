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


/**
 * @brief Set up the grid manager and initialize the cells
 * 
 * @tparam periodic=true Use periodic boundary conditions
 * @tparam ParentModel The parent model type
 * @param name The name of the model
 * @param parent_model The parent model
 * @return auto The manager
 */
template<bool periodic=true, typename ParentModel>
auto setup_manager(std::string name, ParentModel& parent_model)
{
    // Get the logger... and use it :)
    auto log = parent_model.get_logger();
    log->info("Setting up '{}' model ...", name);

    // Get the configuration and the rng
    auto cfg = parent_model.get_cfg()[name];
    auto rng = parent_model.get_rng();

    // Extract grid size from config
    const auto gsize = as_array<unsigned int, 2>(cfg["grid_size"]);

    // Inform about the size
    log->info("Creating 2-dimensional grid of size: {} x {} ...",
              gsize[0], gsize[1]);

    // Create grid of that size
    auto grid = Utopia::Setup::create_grid<2>(gsize);

    // Create the CopyMeBare initial state
    // NOTE: This just sets a _default_ state. The actual initialization
    //       should be part of the model class and invoked during construction
    State state_0 = {};

    // Create cells on that grid, passing the initial state
    auto cells = Utopia::Setup::create_cells_on_grid<true>(grid, state_0);

    // Create the grid manager, passing the template argument
    log->info("Initializing GridManager with {} boundary conditions ...",
              (periodic ? "periodic" : "fixed"));
    
    return Utopia::Setup::create_manager_cells<true, periodic>(grid,
                                                               cells,
                                                               rng);
}


} // namespace CopyMeBare
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_COPYMEBARE_HH
