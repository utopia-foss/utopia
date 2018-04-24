#ifndef VEGETATION_HH
#define VEGETATION_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>

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
    using BCType = typename Base::BCType;

private:
    Manager _manager;
    BCType _bc;

public:
    /// Construct the model.
    /** \param manager Manager for the cells
     *  \param bc Parameters of the vegetation model
     */
    VegetationModel(Manager& manager, BCType bc):
        Base(),
        _manager(manager),
        _bc(bc)
    { }

    /// Iterate one time step
    void perform_step ()
    {
        auto& cells = _manager.cells();

        // Growth
        auto growth_rule = [this](const auto cell){
                auto state = cell->state();
                auto rain  = std::get<0>(_bc)(*(this->_manager.rng()));
                auto growth = std::get<1>(_bc);
                return state + state*growth*(1 - state/rain); // Logistic growth
        };
        apply_rule(growth_rule, cells);

        // Seeding
        auto seeding_rule = [this](const auto cell){
                auto state = cell->state();
                auto seeding = std::get<2>(_bc);
                return (state == 0) ? seeding*std::get<0>(_bc)(*(this->_manager.rng())) : state;
        };
        apply_rule(seeding_rule, cells);
    }

    /// Do nothing
    void write_data () {}

    /// Return const reference to cell container
    const Data& data () const { return _manager.cells(); }

};

} // namespace Utopia

#endif // VEGETATION_HH
