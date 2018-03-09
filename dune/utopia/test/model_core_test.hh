#ifndef UTOPIA_MODEL_TEST_MODEL_CORE_HH
#define UTOPIA_MODEL_TEST_MODEL_CORE_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/setup.hh>
#include <dune/utopia/model.hh>

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

/// Build the model!
decltype(auto) setup_model_core(const unsigned int grid_size)
{
    // types for cells
    constexpr bool sync = false;
    using State = double;
    using Tag = Utopia::DefaultTag;

    auto grid = Utopia::Setup::create_grid(grid_size);
    auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(
        grid, 0.0);
    auto manager = Utopia::Setup::create_manager_cells<true, true>(grid, cells);

    // class template argument deduction YESSSSS
    return CoreModel(manager);
}

#endif // UTOPIA_MODEL_TEST_MODEL_CORE_HH