#ifndef VEGETATION_HH
#define VEGETATION_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>

#include <dune/utopia/data_io/config.hh>
#include <dune/utopia/data_io/hdffile.hh>
#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/hdfdataset.hh>

#include <boost/lexical_cast.hpp>


namespace Utopia {

/// Template declaration for model types extracted from Manager
template<class Manager>
using VegetationModelTypes = Utopia::ModelTypes<
    typename Manager::Container, // unused
    std::tuple<std::normal_distribution<>, double, double> // probability distribution for rain, 
                                                           // growth rate
                                                           // seeding rate
                                                           // seeding and rain use the same probability distribution at the moment
                                                           // maybe change later...
>;

/// A very simple vegetation model
template<class Manager>
class VegetationModel:
    public Model<VegetationModel<Manager>, VegetationModelTypes<Manager>>
{

public:

    using Data = typename Manager::Container;
    using Base = Model<VegetationModel<Manager>, VegetationModelTypes<Manager>>;
    using Parameter = std::tuple<std::normal_distribution<>, double, double>;

private:

    const std::string _name;
    std::shared_ptr<Utopia::DataIO::HDFGroup> _group;
    Manager _manager;
    Parameter _par;

public:

    VegetationModel (const std::string name,
                     Utopia::DataIO::Config config,
                     std::shared_ptr<Utopia::DataIO::HDFGroup> group,
                     Manager manager) :
        Base(),
        _name(name),
        _group(group->open_group(_name)),
        _manager(manager)
    {

        // Initialize model parameters from config file
        std::normal_distribution<> rain{config["rain_mean"].as<double>(),
                                        config["rain_var"].as<double>()};
        _par = std::make_tuple(rain, 
                               config["growth"].as<double>(), 
                               config["seeding"].as<double>());

        // Write constant values (such as positions of cells)
        //file.get_basegroup()->add_attribute("Metadatum1", "HelloWorld"); 
        // TODO later, add info about parameter as metadata
        auto dsetX = _group->open_dataset("positionX");
        dsetX->write(_manager.cells().begin(),
                _manager.cells().end(), 
                [](const auto& cell) {return cell->position()[0];});
        auto dsetY = _group->open_dataset("positionY");
        dsetY->write(_manager.cells().begin(),
                _manager.cells().end(), 
                [](const auto& cell) {return cell->position()[1];});

        // Write initial values
        // TODO later, as iterate function of base needs to be adapted
        //write_data();
    }

    void perform_step ()
    {
        std::cout << "Performing step ... ";

        // Logistic growth + Seeding
        auto growth_seeding_rule = [this](const auto cell){
                auto state = cell->state();
                auto rain  = std::get<0>(_par)(*(_manager.rng()));
                if (state != 0) {
                    auto growth = std::get<1>(_par);
                    return state + state*growth*(1 - state/rain);
                }
                auto seeding = std::get<2>(_par);
                return seeding*rain;

        };
        apply_rule(growth_seeding_rule, _manager.cells());

        std::cout << "complete!\n";
    }

    void write_data () {
        std::cout << "Writing data @ _t = " << Base::time << " ... ";
        auto dset = _group->open_dataset("plants@t="+boost::lexical_cast<std::string>(Base::time));
        dset->write(_manager.cells().begin(),
                _manager.cells().end(), 
                [](const auto cell){ return cell->state(); });
        std::cout << "complete!\n";
    }

    const Data& data () const { return _manager.cells(); }

};

} // namespace Utopia

#endif // VEGETATION_HH
