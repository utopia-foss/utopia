#ifndef VEGETATION_HH
#define VEGETATION_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>

#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/hdfdataset.hh>


namespace Utopia {

namespace Models {

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
        _dset_plant_mass(this->_hdfgrp->open_dataset("plant_mass"))
    {
        // Initialize model parameters from config file
        std::normal_distribution<> rain{this->_cfg["rain_mean"].template as<double>(),
                                        this->_cfg["rain_var"].template as<double>()};
        _bc = std::make_tuple(rain, 
                             this->_cfg["growth"].template as<double>(), 
                             this->_cfg["seeding"].template as<double>());

        // Add the model parameters as attributes
        this->_hdfgrp->add_attribute("rain_mean", 
                                    this->_cfg["rain_mean"].template as<double>());
        this->_hdfgrp->add_attribute("rain_var", 
                                    this->_cfg["rain_var"].template as<double>());
        this->_hdfgrp->add_attribute("growth", 
                                    this->_cfg["growth"].template as<double>());
        this->_hdfgrp->add_attribute("seeding", 
                                    this->_cfg["seeding"].template as<double>());

        // Write the cell coordinates
        auto dsetX = this->_hdfgrp->open_dataset("coordinates_x");
        dsetX->write(_manager.cells().begin(),
                    _manager.cells().end(), 
                    [](const auto& cell) {return cell->position()[0];});
        auto dsetY = this->_hdfgrp->open_dataset("coordinates_y");
        dsetY->write(_manager.cells().begin(),
                    _manager.cells().end(), 
                    [](const auto& cell) {return cell->position()[1];});

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
        // Communicate which iteration step is performed
        std::cout << "  Performing step @ t = " << this->_time << " ...";

        // Apply logistic growth and seeding
        auto growth_seeding_rule = [this](const auto cell){
                auto state = cell->state();
                auto rain  = std::get<0>(_bc)(*(_manager.rng()));
                if (state != 0) {
                    auto growth = std::get<1>(_bc);
                    return state + state*growth*(1 - state/rain);
                }
                auto seeding = std::get<2>(_bc);
                return seeding*rain;

        };
        apply_rule(growth_seeding_rule, _manager.cells());
    }

    /// Write the cell states (aka plant bio-mass)
    void write_data () 
    {
        std::cout << "Writing data @ t = " << this->_time << " ... " << std::endl;

        auto cells = _manager.cells();
        unsigned int num_cells = std::distance(cells.begin(), cells.end());

        _dset_plant_mass->write(cells.begin(), cells.end(), 
                                [](auto& cell) { return cell->state(); } );
    }

    /// Return const reference to stored data
    const Data& data () const { return _manager.cells(); }
    
    /// Set model boundary condition
    void set_boundary_condition (const BCType& new_bc) { _bc = new_bc; }

    /// Set model initial condition
    void set_initial_condition (const Data& ic) {
        auto cells = _manager.cells();
        for (auto ci : cells) {
            // Find cell in ic with same coordinates
            for (auto cj : ic) {
                if (ci->position() == cj->position()) {
                    // Copy cell content
                    ci->state_new() = cj->state();
                }
                std::cout << std::endl;
            }
        }
        for_each(cells.begin(), cells.end(),
            [](const auto& cell){ cell->update(); }
        );
    }
};

} // namespace Models

} // namespace Utopia

#endif // VEGETATION_HH
