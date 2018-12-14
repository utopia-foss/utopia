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
struct State {
    int some_state;
    double some_trait;
    SomeEnum some_enum;
};


/// Struct for all Dataset
template <typename DataSet>
struct DataSets {
    const std::shared_ptr<DataSet> some_state;
    const std::shared_ptr<DataSet> some_trait;
};


/// Typehelper to define types of CopyMe model 
using CopyMeModelTypes = ModelTypes<>;


/// The CopyMe Model
/** Add your model description here.
 *  This model's only right to exist is to be a template for new models. 
 *  That means its functionality is based on nonsense but it shows how 
 *  actually useful functionality could be implemented.
 */
template<class ManagerType>
class CopyMeModel:
    public Model<CopyMeModel<ManagerType>, CopyMeModelTypes>
{
public:
    /// The base model type
    using Base = Model<CopyMeModel<ManagerType>, CopyMeModelTypes>;
    
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

    /// A model parameter I need
    const double _some_parameter;


    // -- Temporary objects -- //


    // -- Datasets -- //
    DataSets<DataSet> _dsets;


    // -- Rule functions -- //
    // Define functions that can be applied to the cells of the grid
    // NOTE The below are examples; delete and/or adjust them to your needs!

    /// Sets the given cell to state "A"
    RuleFunc _set_initial_state_A = [](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        // Set the internal variables
        state.some_state = 5;
        state.some_trait = 2.;
        state.some_enum = Enum0;

        return state;
    };


    /// Sets the given cell to state "B"
    RuleFunc _set_initial_state_B = [](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        // Set the internal variables
        state.some_state = 3;
        state.some_trait = 4.2;
        state.some_enum = Enum1;

        return state;
    };


    /// Define some interaction for a cell
    RuleFunc _some_interaction = [this](const auto cell){
        // Get the state of the Cell
        auto state = cell->state();

        // Increase some_state by one
        state.some_state += 1;

        // Increase some_trait by adding up the some_state's from all neighbors
        for (auto nb : MooreNeighbor::neighbors(cell, this->_manager))
        {
            state.some_trait += static_cast<double>(nb->state().some_state);
        }
        // Ahhh and obviously you need to divide some float by _some_parameter
        // because that makes totally sense
        state.some_trait /= _some_parameter;

        // Set some_enum to Enum0
        state.some_enum = Enum0;

        // Return the new cell state
        return state;
    };


    /// Define the update rule for a cell
    RuleFunc _some_update = [this](const auto cell){
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
        // Datasets
        // Create two datasets with 2d shape {num_write_steps, num_cells}
        _dsets({this->create_dset("some_state", {_manager.cells().size()}),
                this->create_dset("some_trait", {_manager.cells().size()})})
        // NOTE: To create a 1d dataset with one entry per time step just use
        //       _dset.mean_state(this->create_dset("mean_state", {}))
    {
        // Call the method that initializes the cells
        this->initialize_cells();

        // Write initial state
        this->write_data();

        // Add attributes to the datasets
        // NOTE Currently, attributes can be set only after the first write
        //      operation because else the datasets are not yet created.
        const auto grid_size = as_<std::array<std::size_t,2>>(this->_cfg["grid_size"]);
        
        _dsets.some_state->add_attribute("content", "grid");
        _dsets.some_state->add_attribute("grid_shape", grid_size);
        _dsets.some_trait->add_attribute("content", "grid");
        _dsets.some_trait->add_attribute("grid_shape", grid_size);
    }

    // Setup functions ........................................................
    /// Initialize the cells according to `initial_state` config parameter
    void initialize_cells()
    {
        // Extract the mode that determines the initial state
        const auto initial_state = as_str(this->_cfg["initial_state"]);

        // Apply a rule to all cells depending on the config value
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


    /// Monitor model information
    /** @detail Here, functions and values can be supplied to the monitor that
     *          are then available to the frontend. The monitor() function is
     *          _only_ called if a certain emit interval has passed; thus, the
     *          performance hit is small.
     */
    void monitor ()
    {
        // Supply some number -- for illustration -- directly by value
        this->_monitor.set_entry("some_value", 42);

        // Supply the state mean to the monitor
        this->_monitor.set_entry("state_mean", [this](){
            double state_sum = 0.;
            for (const auto &cell : this->_manager.cells()) {
                state_sum += cell->state().some_state;
            }
            return state_sum / std::distance(this->_manager.cells().begin(),
                                             this->_manager.cells().end());
        });
    }


    /// Write data
    void write_data ()
    {   
        // some_state
        _dsets.some_state->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) {
                                    return cell->state().some_state;
                                });

        // some_trait
        _dsets.some_trait->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) {
                                    return cell->state().some_trait;
                                });
    }


    // Getters and setters ....................................................
    // Add getters and setters here to interface with other model
    
};

} // namespace CopyMe
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_COPYME_HH
