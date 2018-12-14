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

/// Typehelper to define data types of Vegetation model
using VegetationTypes = ModelTypes<>;

/// A very simple vegetation model
template<class ManagerType>
class Vegetation:
    public Model<Vegetation<ManagerType>, VegetationTypes>
{
public:

    /// Type helpers
    using Base = Model<Vegetation<ManagerType>, VegetationTypes>;
    using CellType = typename ManagerType::Cell;
    using CellIndexType = typename CellType::Index;
    using DataSet = DataIO::HDFDataset<DataIO::HDFGroup>;
    using RuleFunc = typename std::function<State(std::shared_ptr<CellType>)>;

private:

    /// The grid manager
    ManagerType _manager;

    /// The parameters of the model
    VegetationParameters _params;
    
    /// Dataset 
    std::shared_ptr<DataSet> _dset_plant_mass;

    // -- Rule functions -- //
    /// Apply logistic growth and seeding
    /** For each cell, a random gauss-distributed number is drawn that 
     *  represents the rainfall onto that cell. If the plant bio-mass at that 
     *  cell is already non-zero, it is increased according to a logistic
     *  growth model. If it is zero, than the plant bio-mass is set
     *  proportional to the seeding rate and the amount of rain.
     */
    RuleFunc _growth_seeding = [this](const auto& cell){
        auto state = cell->state();
        auto rain = _params.rain(*(this->_rng));
        auto mass = state.plant_mass;

        // Distinguish by mass whether to grow or to seed
        if (mass != 0.) {
            // regular logistic growth
            state.plant_mass += mass * _params.growth_rate*(1. - mass/rain);
            // TODO consider using Beverton-Holt model?
        }
        else {
            // seeding
            state.plant_mass = _params.seeding_rate * rain;
        }

        return state;
    };

    /// Set negative plant masses to zero
    RuleFunc _sanitize_states = [](const auto& cell){
        auto state = cell->state();

        if (state.plant_mass < 0. or std::isinf(state.plant_mass)) {
            state.plant_mass = 0.;
        }
        return state;
    };

    /// Calculate the mean plant mass
    double calc_mean_mass () {
        auto m_tot = std::accumulate(_manager.cells().begin(),
                                     _manager.cells().end(),
                                     0.,
                                     [&](double sum, const auto& cell) {
                                        return sum + cell->state().plant_mass; 
                                     });
        return m_tot/_manager.cells().size();
    }

public:

    /// Construct the Vegetation model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template<class ParentModel>
    Vegetation (const std::string name,
                const ParentModel & parent_model,
                ManagerType manager) 
    :
        // Construct the base class
        Base(name, parent_model),

        // Initialise the reference to the Manager object
        _manager(manager),

        // Initialize model parameters from config file
        _params(as_double(this->_cfg["rain_mean"]), 
                as_double(this->_cfg["rain_var"]),
                as_double(this->_cfg["growth"]),
                as_double(this->_cfg["seeding"])),

        // Open dataset for output of cell states
        _dset_plant_mass(this->create_dset("plant_mass",
                                           {_manager.cells().size()}))
    {
        // Write out the cell coordinates
        auto coords = this->_hdfgrp->open_dataset("cell_positions",
                                                  {_manager.cells().size()});

        coords->write(_manager.cells().begin(), _manager.cells().end(),
                      [](const auto& cell) { return cell->position();}
                      );

        // Write initial state
        write_data();
    }

    /// Iterate a single step
    void perform_step ()
    {
        apply_rule(_growth_seeding, _manager.cells());
        apply_rule(_sanitize_states, _manager.cells());
        // TODO consider doing this in one step!
    }

    /// Write the cell states (aka plant bio-mass)
    void write_data () 
    {
        _dset_plant_mass->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) {
                                    return cell->state().plant_mass; }
                                );
    }

    /// Monitor the current model state; supplies the mean plant mass
    void monitor () {
        this->_monitor.set_entry("mean_mass", calc_mean_mass());
    }

};


} // namespace Vegetation
} // namespace Models
} // namespace Utopia

#endif // VEGETATION_HH
