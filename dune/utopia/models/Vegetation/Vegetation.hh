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
    using RuleFunc = typename std::function<State(const std::shared_ptr<CellType>)>;

private:

    /// The grid manager
    ManagerType _manager;

    ///
    std::normal_distribution<double> _rain_dist;

    // -- The parameters of the model -- //
    /// The growth rate (logistic growth model)
    double _growth_rate;

    /// The seeding rate
    double _seeding_rate;
    
    // Datasets -- //
    /// Plant mass dataset
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
        auto rain = _rain_dist(*(this->_rng));
        auto mass = state.plant_mass;

        if(rain < 1e-16){
            rain = 0.;
        }

        // Distinguish by mass whether to grow or to seed
        // if negative or smaller than machine epsilon at 1.0 for doubles, 
        // consider invalid and reseed
        if (not (mass < 1e-16)) {
            // regular logistic growth
            // state.plant_mass += mass * _growth_rate*(1. - mass/rain);

            // use Beverton-Holt to approximate discretized logistic growth.
            // however, be careful that when using the wikipedia version
            // [https://en.wikipedia.org/wiki/Beverton–Holt_model]
            // the R0 parameter therein is >= 1 -> proliferation rate!
            // this means that, as given therein, we have: 
            // n_{t+1} = (r*n_t)/(1 + (n_t*(r-1)/K)) with r >= 1, 
            // which has to be turned into 
            // n_{t+1} =((r+1.)*n_t)/(1 +(n_t*r)/K) when r is given  
            // as a growth rate proper
            state.plant_mass = ((_growth_rate+1.)*mass)/(1. + (mass*(_growth_rate))/(rain));
        }
        else {
            // seeding
            state.plant_mass = _seeding_rate * rain;
        }

        return state;
    };

    /// Calculate the mean plant mass
    double calc_mean_mass () {

        return std::accumulate(_manager.cells().begin(), _manager.cells().end(),
                               0.,
                               [&](double sum, const auto& cell) {
                                   return sum + cell->state().plant_mass;
                               }) /
               _manager.cells().size();
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

        // Initialize the reference to the Manager object
        _manager(manager),

        // Initialize the rain distribution
        _rain_dist{as_double(this->_cfg["rain_mean"]),
                   as_double(this->_cfg["rain_std"])},

        // Initialize model parameters from config file
        _growth_rate(as_double(this->_cfg["growth_rate"])),
        _seeding_rate(as_double(this->_cfg["seeding_rate"])),

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
