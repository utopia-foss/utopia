#ifndef UTOPIA_MODELS_COPYME_HH
#define UTOPIA_MODELS_COPYME_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>
#include <dune/utopia/core/types.hh>

#include <functional>

namespace Utopia {
namespace Models {
namespace CopyMe {

/// Enum that will be part of the internal state of a cell
enum SomeEnum : unsigned short int {Enum0, Enum1};


/// State struct for CopyMe model. 
// Here, you can collect all the states, a cell should have
struct State {
    int some_state;
    double some_trait;
    SomeEnum some_enum;
};


// Alias the neighborhood classes
using NextNeighbor = Utopia::Neighborhoods::NextNeighbor;
using MooreNeighbor = Utopia::Neighborhoods::MooreNeighbor;


/// Boundary condition type
struct Boundary {};


/// Typehelper to define data types of CopyMe model 
using CopyMeModelTypes = ModelTypes<State, Boundary>;


/// The CopyME Model
/** Add your class description here.
 *  This model's only right to exist is to be a template model. 
 *  That means its functionality is based on nonesense but it shows how 
 *  actually useful functionality could be implemented.
 */
template<class ManagerType>
class CopyMeModel:
    public Model<CopyMeModel<ManagerType>, CopyMeModelTypes>
{
public:
    /// The base model type
    using Base = Model<CopyMeModel<ManagerType>, CopyMeModelTypes>;
    
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

private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng

    // -- Members of this model -- //
    /// The grid manager
    ManagerType _manager;

    /// A model parameter I need
    const double _some_parameter;

    // -- Temporary objects -- //
    
    // -- Datasets -- //
    std::shared_ptr<DataSet> _dset_some_state;
    std::shared_ptr<DataSet> _dset_some_trait;


    // -- Rule functions -- //
    /// Define some initial state for all cells
    std::function<State(std::shared_ptr<CellType>)> _set_initial_state_A = [](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        // Set the internal variables
        state.some_state = 5;
        state.some_trait = 2.;
        state.some_enum = Enum0;

        return state;
    };


    /// Define some initial state for all cells
    std::function<State(std::shared_ptr<CellType>)> _set_initial_state_B = [](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        // Set the internal variables
        state.some_state = 3;
        state.some_trait = 4.2;
        state.some_enum = Enum1;

        return state;
    };


    /// Define some interaction for a cell
    std::function<State(std::shared_ptr<CellType>)> _some_interaction = [this](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        // Increase some_state by one
        state.some_state += 1;

        // Increase some_trait by adding up the some_state's from all neighbors
        for (auto nb : MooreNeighbor::neighbors(cell, this->_manager))
        {
            state.some_trait += static_cast<double>(nb->state().some_state);
        }
        // Ahhh and obviously you need to divide some float by _some_parameter because that makes totally sense
        state.some_trait /= _some_parameter;

        // Set some_enum to Enum0
        state.some_enum = Enum0;

        // Return the new cell state
        return state;
    };

    /// Define the update rule for a cell
    std::function<State(std::shared_ptr<CellType>)> _some_update = [this](const auto cell){
        // Here, you can write some update rule description

        // Get the state of the cell
        auto state = cell->state();

        // With a probablity of 0.3 set the cell's state.some_state to 0
        std::uniform_real_distribution<> dist(0., 1.);
        if (dist(*this->_rng) < 0.3)
        {
            state.some_state = 0;
        }

        // Set some_enum to Enum1
        state.some_enum = Enum1;

        // Return the new state cell
        return state;
    };


public:
    /// Construct the CopyMe model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template<class ParentModel>
    CopyMeModel (const std::string name,
                   ParentModel &parent,
                   ManagerType&& manager)
    :
        // Initialize first via base model
        Base(name, parent),
        // Now initialize members specific to this class
        _manager(manager),
        _some_parameter(as_double(this->_cfg["some_parameter"])),
        // create datasets
        _dset_some_state(this->_hdfgrp->open_dataset("some_state")),
        _dset_some_trait(this->_hdfgrp->open_dataset("some_trait"))        
    {
        // Call the method that initializes the cells
        this->initialize_cells();

        // Set the capacity of the datasets
        _dset_some_state->set_capacity({this->get_time_max() + 1});
        _dset_some_trait->set_capacity({this->get_time_max() + 1});

        // Write initial state
        this->write_data();
    }

    // Setup functions ........................................................
    /// Initialize the cells according to `initial_state` config parameter
    void initialize_cells()
    {
        // Extract the mode that determines the initial state
        const auto initial_state = as_str(this->_cfg["initial_state"]);

        // Apply a rule to all cells depending on the configuration
        if (initial_state == "init_0")
        {
            apply_rule(_set_initial_state_A, _manager.cells());
        }
        else if (initial_state == "init_1")
        {
            apply_rule(_set_initial_state_B, _manager.cells());
        }
        else
        {
            throw std::runtime_error("The initial state is not valid!");
        }

        // Write information that cells are initialized to the logger
        this->_log->info("Cells initialized.");
    }


    // Runtime functions ......................................................

    /** @brief Iterate a single step
     *  @detail Here you can add a detailed description what exactly happens 
     *          in a single iteration step
     */
    void perform_step ()
    {
        // Apply the rules to all cells, first the interaction, then the update
        apply_rule(_some_interaction, _manager.cells());
        apply_rule(_some_update, _manager.cells());
    }


    /// Write data
    void write_data ()
    {   
        // some_state
        _dset_some_state->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) {
                                    return cell->state().some_state;
                                });

        // some_trait
        _dset_some_trait->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) {
                                    return cell->state().some_trait;
                                });
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

    // Create the CopyMe initial state:
    // some_state 0 and some_trait 3.4 some_initial_enum "A", "B"
    // NOTE: This just sets a _default_ state. The actual initialization
    //       should be part of the model class and invoked during construction
    State state_0 = {0, 3.4, SomeEnum::Enum0};

    // Create cells on that grid, passing the initial state
    auto cells = Utopia::Setup::create_cells_on_grid<true>(grid, state_0);

    // Create the grid manager, passing the template argument
    if (periodic) {
        log->info("Now initializing GridManager with periodic boundary "
                  "conditions ...");
    }
    else {
        log->info("Now initializing GridManager with fixed boundary "
                  "conditions ...");
    }
    
    return Utopia::Setup::create_manager_cells<true, periodic>(grid,
                                                               cells,
                                                               rng);
}


} // namespace CopyMe
} // namespace Models
} // namespace Utopia
#endif // UTOPIA_MODELS_COPYME_HH
