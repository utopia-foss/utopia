#ifndef GEOMORPHOLOGY_HH
#define GEOMORPHOLOGY_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>

#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/hdfdataset.hh>

namespace Utopia {

namespace Models {

namespace Geomorphology {

/// Define data type and boundary condition type of geomorphology model
template<class Manager>
using DataType = typename Manager::Container;

using Rain = std::normal_distribution<>;

template<class Manager>
using GeomorphologyTypes = ModelTypes<DataType<Manager>, Rain>;

/// A very simple geomorphology model
template<class Manager>
class Geomorphology:
    public Model<Geomorphology<Manager>, GeomorphologyTypes<Manager>>
{
public:

    /// Type helpers
    using Base = Model<Geomorphology<Manager>, GeomorphologyTypes<Manager>>;
    using BCType = typename Base::BCType;
    using Data = typename Base::Data;
    using DataSet = DataIO::HDFDataset<DataIO::HDFGroup>;

private:

    /// The grid manager
    Manager _manager;

    /// The boundary conditions (aka parameters) of the model
    BCType _bc;

    /// Dataset 
    std::shared_ptr<DataSet> _dset_water_content;

public:

    /// Construct the Geomorphology model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template<class ParentModel>
    Geomorphology (const std::string name,
                   const ParentModel& parent_model,
                   Manager manager)
    :
        // Construct the base class
        Base(name, parent_model),

        // Initialise the reference to the Manager object
        _manager(manager),

        // Open dataset for output of cell states 
        _dset_water_content(this->_hdfgrp->open_dataset("water_content")),

        // Initialize model parameters from config file
        _bc{this->_cfg["rain_mean"].template as<double>(), 
            this->_cfg["rain_var"].template as<double>()}
    {
        // Initialize altitude as an inclined plane (by making use of coordinates)
        auto set_inclined_plane = [this](const auto cell) {   
            auto state = cell->state();
            state[0] = cell->position()[1]; 
            return state;       
        };
        apply_rule(set_inclined_plane, manager.cells()); 

        // Add the model parameters as attributes
        this->_hdfgrp->add_attribute("rain_mean", 
                                    this->_cfg["rain_mean"].template as<double>());
        this->_hdfgrp->add_attribute("rain_var", 
                                    this->_cfg["rain_var"].template as<double>());

        // Write the cell coordinates
        auto coords = this->_hdfgrp->open_dataset("cell_positions",
                                                  {_manager.cells().size()});
        coords->write(_manager.cells().begin(),
                      _manager.cells().end(),
                      [](const auto& cell) {
                        return std::array<double,2>
                            {{cell->position()[0],
                              cell->position()[1]}};
                      }
        );

        // Write cell height 
        auto dsetH = this->_hdfgrp->open_dataset("height");
        dsetH->write(_manager.cells().begin(),
                    _manager.cells().end(), 
                    [](const auto& cell) {return cell->state()[0];});

        // Write initial state 
        write_data();
    }

    /// Iterate a single step
    void perform_step ()
    {
        // Let it rain
        auto rain = [this](const auto cell) {
            auto rain = _bc(*(_manager.rng()));
            auto state = cell->state();
            state[1] += rain; 
            return state;
        };
        apply_rule(rain, _manager.cells());

        // Reset state_new of cells
        auto& cells = _manager.cells();
        for (auto& cell : cells) {
            cell->state_new()[1] = 0;
        }

        // Waterflow  
        for (auto& cell : cells) {
            // Find lowest neighbour
            auto neighbors = Neighborhoods::NextNeighbor::neighbors(cell, _manager);
            auto l_neighbor = neighbors[0];
            for (auto neighbor : neighbors) {
                if (neighbor->state()[0] < l_neighbor->state()[0]) 
                    l_neighbor = neighbor;
            }
            // Put watercontent from cell to l_neighbor
            l_neighbor->state_new()[1] += cell->state()[1];
        }

        // Remove water from boundary cells (sinks)
        for (auto& cell : cells) {
            if (cell->is_boundary())
                cell->state_new()[1] = 0;
        }

        // Update cells
        for (auto& cell : cells) {
            cell->update();
        }
    }

    /// Write the cell states (aka water content)
    /** The cell height is currently not written out as in the current implementation 
     *  it does not change over time (erosion is not yet included).
     */
    void write_data () {

        _dset_water_content->write(_manager.cells().begin(),
                                   _manager.cells().end(),
                                   [](auto& cell) { return cell->state()[1]; }
                                  );
    }

    /// Return const reference to cell container
    const Data& data () const { return _manager.cells(); }

    /// Set model boundary condition
    void set_boundary_condition (const BCType& new_bc) { _bc = new_bc; }


    /// Set model initial condition
    void set_initial_condition (const Data& ic)
    {
        assert(check_ic_compatibility(ic));

        auto& cells = _manager.cells();
        for (size_t i = 0; i < cells.size(); ++i) {
            cells[i]->state_new() = ic[i]->state();
            cells[i]->update();
        }
    }

private:

    // Verify that the inserted initial condition can be used
    /** This function checks if
     *      - the inserted container has the appropriate size
     *      - the coordinates of all cells match individually
     *  \param ic Initial condition to be used
     */
    bool check_ic_compatibility (const Data& ic)
    {
        const auto& cells = _manager.cells();

        // check size
        if (ic.size() != cells.size()) {
            throw std::runtime_error("Container inserted as initial condition "
                "has incorrect size " + std::to_string(ic.size())
                + " (should be " + std::to_string(cells.size() + ")"));
        }

        // check coordinates
        for (size_t i = 0; i < cells.size(); ++i)
        {
            const auto& coord_1 = cells[i]->position();
            const auto& coord_2 = ic[i]->position();
            for (size_t j = 0; j < coord_1.size(); j++) {
                if (coord_1[j] != coord_2[j])
                    return false;
            }
        }
        return true;
    }
};


/// Setup the grid manager with an initial state
/** \param name          The name of the model instance
  * \param parent_model  The parent model the new model instance will reside in
  *
  * \tparam ParentModel  The parent model type
  */ 
template<typename ParentModel>
auto setup_manager(const std::string name, ParentModel& parent_model)
{
    // define data types (should this go into the namespace?)
    using State = std::array<double, 2>; //height, watercontent
    using Tag = Utopia::DefaultTag;
    constexpr bool sync = true;

    // define grid properties
    constexpr bool structured = true;
    constexpr bool periodic = false;

    // Get the logger... and use it :)
    auto log = parent_model.get_logger();
    log->info("Setting up '{}' model ...", name);

    // Get the configuration and the rng
    auto cfg = parent_model.get_cfg()[name];
    auto rng = parent_model.get_rng();

    // Extract grid size from config
    const auto grid_size = as_<unsigned int>(cfg["grid_size"]);

    // Inform about the size
    log->info("Creating 2-dimensional grid of size: {0} x {0} ...",
              grid_size);

    // Create grid of that size
    auto grid = Utopia::Setup::create_grid<2>(grid_size);

    // initial value: No plant density
    State initial_state = {0.0, 0.0};

    // Create cells on that grid, passing the initial state
    auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(grid,
                                                                       initial_state);

    // Create the grid manager, passing the template argument
    log->info("Initializing GridManager with {} boundary conditions ...",
              (periodic ? "periodic" : "fixed"));

    return Utopia::Setup::create_manager_cells<structured, periodic>(grid,
                                                                     cells,
                                                                     rng);
}

} // namespace Geomorphology

} // namespace Models

} // namespace Utopia

#endif // GEOMORPHOLOGY_HH

