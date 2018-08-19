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


/// State of a cell in the vegetation model
struct State {
    double plant_mass;
};

/// Define parameters of vegetation model
using Rain = std::normal_distribution<>;
struct VegetationParameters {

    // Constructor
    VegetationParameters(double _rain_mean, double _rain_var, 
            double _growth_rate, double _seeding_rate) : 
        rain{_rain_mean, _rain_var}, 
        growth_rate(_growth_rate), seeding_rate(_seeding_rate) {}

    Rain rain;
    double growth_rate;
    double seeding_rate;
};

template<class Manager>
using VegetationTypes = ModelTypes<typename Manager::Container, VegetationParameters>;


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

        // Initialize model parameters from config file
        _bc(this->_cfg["rain_mean"].template as<double>(), 
            this->_cfg["rain_var"].template as<double>(),
            this->_cfg["growth"].template as<double>(),
            this->_cfg["seeding"].template as<double>()),

        // Open dataset for output of cell states 
        _dset_plant_mass(this->_hdfgrp->open_dataset("plant_mass",
                                                     {this->get_time_max() + 1,
                                                      _manager.cells().size()}))
    {
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
                auto state = cell->state();
                auto rain = _bc.rain(*(this->_rng));
                auto mass = state.plant_mass;

                // regular logistic growth
                if (mass != 0) {
                    state.plant_mass += mass * _bc.growth_rate*(1 - mass/rain);
                }

                // seeding
                else {
                    state.plant_mass = _bc.seeding_rate*rain;
                }

                return state;
        };

        // Set negative populations to zero
        auto sanitize_population = [this](const auto cell){
            auto state = cell->state();
            if (state.plant_mass < 0. or std::isinf(state.plant_mass)) {
                state.plant_mass = 0.;
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
                                [](auto& cell) { return cell->state().plant_mass; }
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


} // namespace Vegetation
} // namespace Models
} // namespace Utopia

#endif // VEGETATION_HH
