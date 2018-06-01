#ifndef UTOPIA_MODEL_TEST_MODEL_WITH_MANAGER_HH
#define UTOPIA_MODEL_TEST_MODEL_WITH_MANAGER_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>

#include <dune/utopia/data_io/config.hh>
#include <dune/utopia/data_io/hdffile.hh>
#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/hdfdataset.hh>


/// Template declaration for model types extracted from Manager
template<class Manager>
using MngrModelTypes = Utopia::ModelTypes<
    typename Manager::Container,
    std::vector<bool>
>;

/// A model that has a custom (and templated) Manager as as member
template<class Manager>
class MngrModel:
    public Utopia::Model<MngrModel<Manager>, MngrModelTypes<Manager>>
{
private:
    Manager _manager;

public:
    using Base = Utopia::Model<MngrModel<Manager>, MngrModelTypes<Manager>>;
    using Data = typename Manager::Container;
    using BCType = typename Base::BCType;
    using Config = typename Base::Config;
    using DataGroup = typename Base::DataGroup;
    using RNG = typename Base::RNG;

    /// Construct the model.
    /** \param manager Manager for this model
     */
    MngrModel (const std::string name,
               Config &parent_cfg,
               std::shared_ptr<DataGroup> parent_group,
               std::shared_ptr<RNG> shared_rng,
               const Manager& manager)
    :
        Base(name, parent_cfg, parent_group, shared_rng),
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

/// Setup the manager to be used with the model class
/** It is this setup method that allows type deduction while instantiating
 *  the templated model class.
 */
decltype(auto) setup_manager(const unsigned int grid_size)
{
    // types for cells
    constexpr bool sync = false;
    using State = double;
    using Tag = Utopia::DefaultTag;

    // create a grid, cells on it
    auto grid = Utopia::Setup::create_grid(grid_size);
    auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(
        grid, 0.0);

    // can now create and return a GridManager
    return Utopia::Setup::create_manager_cells<true, true>(grid, cells);
}

#endif // UTOPIA_MODEL_TEST_MODEL_WITH_MANAGER_HH
