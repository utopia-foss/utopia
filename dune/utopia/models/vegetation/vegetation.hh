#ifndef VEGETATION_HH
#define VEGETATION_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>

#include <dune/utopia/data_io/config.hh>
#include <dune/utopia/data_io/hdffile.hh>
#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/hdfdataset.hh>


namespace Utopia {

namespace Models {

/// Define data types of vegetation model
template<class Manager>
using VegetationTypes = Utopia::ModelTypes<
    typename Manager::Container,           // unused
    std::tuple<std::normal_distribution<>, // probability distribution for rain, 
               double,                     // growth rate
               double>                     // seeding rate
    // seeding and rain use the same probability distribution at the moment
    // maybe change later...
>;

/// A very simple vegetation model
template<class Manager>
class Vegetation:
    public Model<Vegetation<Manager>, VegetationTypes<Manager>>
{
public:

    using Base = Model<Vegetation<Manager>, VegetationTypes<Manager>>;
    using BCType = typename Base::BCType;
    using Data = typename Base::Data;

    Manager manager;
    BCType bc;

    template<class ParentModel>
    Vegetation (const std::string name,
                const ParentModel & parent_model,
                Manager _manager) 
    :
        // Use the base constructor for the main parts
        Base(name, parent_model),
        // Initialise state and boundary condition members
        manager(_manager)
    {

        // Initialize model parameters from config file
        std::normal_distribution<> rain{this->cfg["rain_mean"].template as<double>(),
                                        this->cfg["rain_var"].template as<double>()};
        bc = std::make_tuple(rain, 
                             this->cfg["growth"].template as<double>(), 
                             this->cfg["seeding"].template as<double>());
        //std::normal_distribution<> rain{10.1,1.2};
        //bc = std::make_tuple(rain, 0.2, 0.2);

        // Write constant values (such as positions of cells)
        // TODO add info about parameter as metadata
        auto dsetX = this->hdfgrp->open_dataset("positionX");
        dsetX->write(manager.cells().begin(),
                    manager.cells().end(), 
                    [](const auto& cell) {return cell->position()[0];});
        auto dsetY = this->hdfgrp->open_dataset("positionY");
        dsetY->write(manager.cells().begin(),
                    manager.cells().end(), 
                    [](const auto& cell) {return cell->position()[1];});

        // Write initial state 
        write_data();
    }

    void perform_step ()
    {
        // Communicate which iteration step is performed
        std::cout << "  Performing step @ t = " << this->time << " ...";

        // Apply logistic growth and seeding
        auto growth_seeding_rule = [this](const auto cell){
                auto state = cell->state();
                auto rain  = std::get<0>(bc)(*(manager.rng()));
                if (state != 0) {
                    auto growth = std::get<1>(bc);
                    return state + state*growth*(1 - state/rain);
                }
                auto seeding = std::get<2>(bc);
                return seeding*rain;

        };
        apply_rule(growth_seeding_rule, manager.cells());
    }

    void write_data () 
    {
        std::cout << "Writing data @ t = " << this->time << " ... " << std::endl;
        auto dset = this->hdfgrp->open_dataset("plants@t="+std::to_string(this->time));
        dset->write(manager.cells().begin(),
                    manager.cells().end(), 
                    [](const auto cell){ return cell->state(); });
    }

    const Data& data () const { return manager.cells(); }

};

} // namespace Models

} // namespace Utopia

#endif // VEGETATION_HH
