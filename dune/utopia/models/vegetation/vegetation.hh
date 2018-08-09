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
        _dset_plant_mass(this->_hdfgrp->open_dataset("plant_mass"))
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
        auto coords = this->_hdfgrp->open_dataset("cell_positions");
        coords->write(_manager.cells().begin(),
                      _manager.cells().end(),
                      [](const auto& cell) {
                        return std::array<double,2>
                            {{cell->position()[0],
                              cell->position()[1]}};
                      },
                      1,
                      {100},
                      {100}
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
        auto cells = _manager.cells();
        unsigned int num_cells = std::distance(cells.begin(), cells.end());

        _dset_plant_mass->write(cells.begin(), cells.end(),
                              [](auto& cell) { return cell->state(); },
                              2,              // rank
                              {1, num_cells}, // extend of this entry
                              {},             // max_size of the dataset
                              8               // chunksize, for extension
                              );
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

} // namespace Vegetation
} // namespace Models
} // namespace Utopia

#endif // VEGETATION_HH
