#ifndef CORE_HH
#define CORE_HH

namespace Utopia {

/// Template declaration for model types extracted from Manager
template<class Manager>
using CoreModelTypes = Utopia::ModelTypes<
    typename Manager::Container,
    std::vector<bool>
>;

/// A very simple model implementing 
template<class Manager>
class CoreModel:
    public Utopia::Model<CoreModel<Manager>, CoreModelTypes<Manager>>
{
private:
    Manager _manager;

public:
    using Data = typename Manager::Container;
    using Base = Utopia::Model<CoreModel<Manager>, CoreModelTypes<Manager>>;
    using BCType = typename Base::BCType;

    /// Construct the model.
    /** \param manager Manager for this model
     */
    CoreModel (const Manager& manager):
        Base(),
        _manager(manager)
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

#endif // CORE_HH
