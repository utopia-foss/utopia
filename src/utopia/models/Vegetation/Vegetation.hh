#ifndef VEGETATION_HH
#define VEGETATION_HH

#include <random>
#include <memory>
#include <algorithm>
#include <string>

#include <utopia/core/model.hh>
#include <utopia/core/apply.hh>
#include <utopia/core/types.hh>
#include <utopia/core/cell_manager.hh>

#include <utopia/data_io/hdfgroup.hh>
#include <utopia/data_io/hdfdataset.hh>
#include <utopia/data_io/cfg_utils.hh>


namespace Utopia {
namespace Models {
namespace Vegetation {

/// State of a cell in the Vegetation model, consisting only of plant mass
struct CellState {
    double plant_mass;
};

/// Typehelper to define data types of Vegetation model
using VegetationTypes = Utopia::ModelTypes<>;

/// Define the model cell traits: State type, sync update, use default ctor.
using CellTraits = Utopia::CellTraits<CellState, Update::sync, true>;


/// A very simple vegetation model
class Vegetation:
    public Model<Vegetation, VegetationTypes>
{
public:
    /// Type of the base class
    using Base = Model<Vegetation, VegetationTypes>;

    /// Type of the config tree
    using Config = typename Base::Config;

    /// Type of a data set
    using DataSet = typename Base::DataSet;

    /// Type of the cell manager used
    using CellManager = Utopia::CellManager<CellTraits, Vegetation>;

    /// Type of the update rule
    using RuleFunc = typename CellManager::RuleFunc;

private:
    /// The grid manager
    CellManager _cm;

    // -- The parameters of the model -- //
    // TODO Consider making these public or implementing setters & getters

    /// Normal distribution for drawing random rain values
    std::normal_distribution<double> _rain_dist;

    /// The growth rate (logistic growth model)
    double _growth_rate;

    /// The seeding rate
    double _seeding_rate;


    // Datasets -- //
    /// Plant mass dataset
    std::shared_ptr<DataSet> _dset_plant_mass;


    // -- Rule functions -- //
    /// Apply logistic growth and seeding
    /** @details For each cell, a random gauss-distributed number is drawn that 
     *          represents the rainfall onto that cell. If the plant bio-mass
     *          at that cell is already non-zero, it is increased according to
     *          a logistic growth model, modelled by the Beverton-Holt
     *          discretisation of the logistic function. If it is zero, then
     *          the plant bio-mass is set proportional to the seeding rate and
     *          the amount of rain.
     */
    RuleFunc _growth_seeding = [this](const auto& cell){
        auto state = cell->state();
        auto rain = _rain_dist(*(this->_rng));
        auto mass = state.plant_mass;

        if (rain < 1e-16) {
            rain = 0.;
        }

        // Distinguish by mass whether to grow or to seed
        // If negative or smaller than machine epsilon at 1.0 for doubles,
        // consider invalid and reseed
        if (not (mass < 1e-16)) {
            // Logistic Growth
            /* Use Beverton-Holt to approximate discretized logistic growth.
             * Be careful that when using the Wikipedia version
             *   [https://en.wikipedia.org/wiki/Beverton–Holt_model], the R0
             * parameter therein is >= 1 -> proliferation rate!
             * This means that, as given therein, we have:
             *   n_{t+1} = (r*n_t)/(1 + (n_t*(r-1)/K))
             *  with r >= 1, which has to be turned into
             *   n_{t+1} =((r+1.)*n_t)/(1 +(n_t*r)/K)
             * when r is given as a growth rate proper.
             */
            state.plant_mass = ((_growth_rate + 1.) * mass)/(1. + (mass * (_growth_rate))/rain);
        }
        else {
            // Seeding
            state.plant_mass = _seeding_rate * rain;
        }

        return state;
    };

    /// Calculate the mean plant mass
    double calc_mean_mass () const {
        return std::accumulate(_cm.cells().begin(),
                               _cm.cells().end(),
                               0.,
                               [](double sum, const auto& cell) {
                                   return sum + cell->state().plant_mass;
                               }) / _cm.cells().size();
    }

public:

    /// Construct the Vegetation model
    /** \param name          Name of this model instance
     *  \param parent_model  The parent model this model instance resides in
     */
    template<class ParentModel>
    Vegetation (const std::string name,
                const ParentModel& parent_model)
    :
        // Construct the base class
        Base(name, parent_model),

        // Initialize the manager, setting the initial state
        _cm(*this, CellState({0.0})),

        // Initialize the rain distribution
        _rain_dist{get_as<double>("rain_mean", this->_cfg),
                   get_as<double>("rain_std", this->_cfg)},

        // Initialize model parameters from config file
        _growth_rate(get_as<double>("growth_rate", this->_cfg)),
        _seeding_rate(get_as<double>("seeding_rate", this->_cfg)),

        // Open dataset for output of cell states
        _dset_plant_mass(this->create_cm_dset("plant_mass", _cm))
    {
        this->_log->info("'{}' model fully set up.", name);
    }

    /// Iterate a single step
    void perform_step ()
    {
        apply_rule(_growth_seeding, _cm.cells());
    }

    /// Write the cell states (aka plant bio-mass)
    void write_data ()
    {
        _dset_plant_mass->write(_cm.cells().begin(),
                                _cm.cells().end(),
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
