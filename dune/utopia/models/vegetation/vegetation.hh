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
    Manager _manager;
    Utopia::DataIO::HDFFile _hdff;
    Parameter _par;
    std::size_t _t;

public:

    VegetationModel(Manager& manager, Utopia::DataIO::Config config):
        Base(),
        _manager(manager),
        _t(0),
        _hdff("vegetation-test.h5", "w")
    {
        std::cout << "Construction of vegetation ... ";
        double rain_mean = config["rain_mean"].as<double>();
        double rain_var = config["rain_var"].as<double>();
        double growth = config["growth"].as<double>();
        double seeding = config["seeding"].as<double>();
        std::normal_distribution<> rain{rain_mean, rain_var};
        _par = std::make_tuple(rain, growth, seeding);
        //file.get_basegroup()->add_attribute("Metadatum1", "HelloWorld"); TODO later, add info about parameter as metadata
        std::cout << "complete!\n";
    }

    void perform_step ()
    {
        ++_t;
        std::cout << "Performing step ... ";

        // Growth + Seeding
        auto growth_seeding_rule = [this](const auto cell){
                auto state = cell->state();
                auto rain  = std::get<0>(_par)(*(this->_manager.rng()));
                if (state != 0) {
                    auto growth = std::get<1>(_par);
                    return state + state*growth*(1 - state/rain); // Logistic growth
                }
                auto seeding = std::get<2>(_par);
                return seeding*rain;

        };
        apply_rule(growth_seeding_rule, _manager.cells());
        std::cout << "complete!\n";
        write_data();
    }

    void write_data () {
        std::cout << "Writing data @ _t = " << _t << " ... ";
        auto dset = _hdff.open_dataset("t="+std::to_string(_t));
        dset->write(_manager.cells().begin(),
                _manager.cells().end(), 
                [](const auto cell){ return cell->state(); });
        std::cout << "complete!\n";
    }

    const Data& data () const { return _manager.cells(); }

};

} // namespace Utopia

#endif // VEGETATION_HH
