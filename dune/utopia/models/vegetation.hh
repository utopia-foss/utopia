#ifndef VEGETATION_HH
#define VEGETATION_HH

namespace Utopia {

/// Template declaration for model types extracted from Manager
template<class Manager>
using VegetationModelTypes = Utopia::ModelTypes<
    typename Manager::Container, //?
    //std::tuple<double, double> //rand_dist, equ.wateruptake
    std::tuple<std::normal_distribution<>, double> //rand_dist, equ.wateruptake
>;

/// A very simple model implementing 
template<class Manager>
class Vegetation:
    public Utopia::Model<Vegetation<Manager>, VegetationModelTypes<Manager>>
{
public:
    using Data = typename Manager::Container;
    using Base = Utopia::Model<Vegetation<Manager>, VegetationModelTypes<Manager>>;
    using BCType = typename Base::BCType;

private:
    Manager _manager;
    BCType _bc;

public:
    /// Construct the model.
    /** \param manager Manager for this model
     */
    Vegetation (const Manager& manager, BCType bc):
        Base(),
        _manager(manager),
        _bc(bc)
    { }

    /// Iterate one time step
    void perform_step ()
    {
        auto& cells = _manager.cells();
        std::for_each(cells.begin(), cells.end(),
            [this](const auto cell){
                auto nb = Utopia::Neighborhoods::NextNeighbor::neighbors(
                    cell, this->_manager);
                cell->state() = nb.size();
                if (cell->is_boundary())
                    cell->is_tagged = true;
        });
    }

    /// Do nothing
    void write_data () {}

    /// Return const reference to cell container
    const Data& data () const { return _manager.cells(); }

    // Set model boundary condition
    void set_boundary_condition (const BCType& bc) { }

    /// Set model initial condition
    void set_initial_condition (const Data& container) {
        // check container size
        const auto& cells = _manager.cells();
        assert(container.size() == cells.size());
        for(std::size_t i = 0; i<container.size(); ++i){
            cells[i]->state() = container[i]->state();
            cells[i]->is_tagged = container[i]->is_tagged;
        }
    }
};

} // namespace Utopia

#endif // VEGETATION_HH
