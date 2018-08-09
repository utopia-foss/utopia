#ifndef VEGETATION_HH
#define VEGETATION_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>

#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/hdfdataset.hh>


namespace Utopia {

namespace Models {

namespace Vegetation {

/// Define data type and boundary condition type of vegetation model
template<class Manager>
using DataType = typename Manager::Container;

using Rain = std::normal_distribution<>;
using GrowthRate = double;
using SeedingRate = double;
using BoundaryConditionType = std::tuple<Rain, GrowthRate, SeedingRate>;

template<class Manager>
using VegetationTypes = ModelTypes<DataType<Manager>, BoundaryConditionType>;

/// A very simple vegetation model
template<class Manager>
class Vegetation:
    public Model<Vegetation<Manager>, VegetationTypes<Manager>>
{
public:

    /// Type helpers
    using Base = Model<Vegetation<Manager>, VegetationTypes<Manager>>;
    using BCType = typename Base::BCType;
    using Data = typename Base::Data;
    using DataSet = DataIO::HDFDataset<DataIO::HDFGroup>;

private:

    /// The grid manager
    Manager _manager;

    /// The boundary conditions (aka parameters) of the model
    BCType _bc;
    
    /// Dataset 
    std::shared_ptr<DataSet> _dset_plant_mass;

public:

    /// Construct the Vegetation model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template<class ParentModel>
    Vegetation (const std::string name,
                const ParentModel & parent_model,
                Manager manager) 
    :
        // Construct the base class
        Base(name, parent_model),

        // Initialise the reference to the Manager object
        _manager(manager),

        // Open dataset for output of cell states 
        _dset_plant_mass(this->_hdfgrp->open_dataset("plant_mass",
                                                     {this->get_time_max() + 1,
                                                      _manager.cells().size()}))
    {
        // Initialize model parameters from config file
        Rain rain{as_double(this->_cfg["rain_mean"]),
                  as_double(this->_cfg["rain_var"])};
        _bc = std::make_tuple(rain, 
                              as_double(this->_cfg["growth"]),
                              as_double(this->_cfg["seeding"]));

        // Add the model parameters as attributes
        this->_hdfgrp->add_attribute("rain_mean",
                                     as_double(this->_cfg["rain_mean"]));
        this->_hdfgrp->add_attribute("rain_var",
                                     as_double(this->_cfg["rain_var"]));
        this->_hdfgrp->add_attribute("growth",
                                     as_double(this->_cfg["growth"]));
        this->_hdfgrp->add_attribute("seeding",
                                     as_double(this->_cfg["seeding"]));

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

        // Write initial state
        write_data();
    }

    /// Iterate a single step
    /** For each cell, a random gauss-distributed number is drawn that 
     *  represents the rainfall onto that cell. If the plant bio-mass at that 
     *  cell is already non-zero, it is increased according to a logistic growth
     *  model. If it is zero, than the plant bio-mass is set proportional to the 
     *  seeding rate and the amount of rain.
     */
    void perform_step ()
    {
        // Apply logistic growth and seeding
        auto growth_seeding_rule = [this](const auto cell){
                const auto state = cell->state();
                auto rain  = std::get<Rain>(_bc)(*(_manager.rng()));

                // regular logistic growth
                if (state != 0) {
                    auto growth = std::get<1>(_bc);
                    return state + state*growth*(1 - state/rain);
                }

                // seeding
                auto seeding = std::get<2>(_bc);
                return seeding*rain;

        };

        // Set negative populations to zero
        auto sanitize_population = [this](const auto cell){
            const auto state = cell->state();
            if (state < 0.0 or std::isinf(state)) {
                return 0.0;
            }

            return state;
        };

        apply_rule(growth_seeding_rule, _manager.cells());
        apply_rule(sanitize_population, _manager.cells());
    }

    /// Write the cell states (aka plant bio-mass)
    void write_data () 
    {
        _dset_plant_mass->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) { return cell->state(); }
                               );
    }

    /// Return const reference to stored data
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
    using State = double;
    using Tag = Utopia::DefaultTag;
    constexpr bool sync = true;

    // define grid properties
    constexpr bool structured = true;
    constexpr bool periodic = true; // not relevant for this model

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
    State initial_state = 0.0;

    // Create cells on that grid, passing the initial state
    auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(grid,        initial_state);

    // Create the grid manager, passing the template argument
    log->info("Initializing GridManager with {} boundary conditions ...",
              (periodic ? "periodic" : "fixed"));

    return Utopia::Setup::create_manager_cells<structured, periodic>(grid,
                                                                     cells,
                                                                     rng);
}

} // namespace Vegetation
} // namespace Models
} // namespace Utopia

#endif // VEGETATION_HH
