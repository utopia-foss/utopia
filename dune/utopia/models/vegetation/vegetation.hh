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
    std::tuple<std::normal_distribution<>, double> // probability distribution for rain, 
                                                   // reproduction rate
>;

/// A very simple model implementing 
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
    /** \param manager Manager for this model
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
        auto rule = [this](const auto cell){
                auto state = cell->state();
                auto rain  = std::get<0>(_bc)(*(this->_manager.rng()));
                auto birth = std::get<1>(_bc);
                return state + state*birth*(1 - state/rain);
        };
        apply_rule(rule, cells);
    }

    /// Do nothing
    void write_data () {}

    /// Return const reference to cell container
    const Data& data () const { return _manager.cells(); }

};

} // namespace Utopia

#endif // VEGETATION_HH
