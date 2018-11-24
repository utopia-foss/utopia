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

private:

    /// The grid manager
    ManagerType _manager;

    /// The parameters of the model
    VegetationParameters _params;
    
    /// Dataset 
    std::shared_ptr<DataSet> _dset_plant_mass;

    /// Rule functions

    // Apply logistic growth and seeding
    /** For each cell, a random gauss-distributed number is drawn that 
     *  represents the rainfall onto that cell. If the plant bio-mass at that 
     *  cell is already non-zero, it is increased according to a logistic growth
     *  model. If it is zero, than the plant bio-mass is set proportional to the 
     *  seeding rate and the amount of rain.
     */
    std::function<State(std::shared_ptr<CellType>)> _growth_seeding = [this](const auto cell){
        auto state = cell->state();
        auto rain = _params.rain(*(this->_rng));
        auto mass = state.plant_mass;

        // regular logistic growth
        if (mass != 0) { state.plant_mass += mass * _params.growth_rate*(1 - mass/rain); }

        // seeding
        else { state.plant_mass = _params.seeding_rate*rain; }

        return state;
    };

    // Set negative plant masses to zero
    std::function<State(std::shared_ptr<CellType>)> _sanitize_states = [](const auto cell){
        auto state = cell->state();
        if (state.plant_mass < 0. or std::isinf(state.plant_mass)) {
            state.plant_mass = 0.;
        }
        return state;
    };

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
        _params(this->_cfg["rain_mean"].template as<double>(), 
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
    void perform_step ()
    {
        apply_rule(_growth_seeding, _manager.cells());
        apply_rule(_sanitize_states, _manager.cells());
    }

    /// Write the cell states (aka plant bio-mass)
    void write_data () 
    {
        _dset_plant_mass->write(_manager.cells().begin(),
                                _manager.cells().end(),
                                [](auto& cell) { return cell->state().plant_mass; }
                               );
    }

    /// Monitor model information
    void monitor () { }

};


} // namespace Vegetation
} // namespace Models
} // namespace Utopia

#endif // VEGETATION_HH
