#ifndef GEOMORPHOLOGY_HH
#define GEOMORPHOLOGY_HH

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
using GeomorphologyTypes = Utopia::ModelTypes<
    typename Manager::Container,
    std::normal_distribution<> // rain
>;

/// A very simple model implementing 
template<class Manager>
class GeomorphologyModel:
    public Utopia::Model<GeomorphologyModel<Manager>, GeomorphologyTypes<Manager>>
{
public:
    using Data = typename Manager::Container;
    using Base = Utopia::Model<GeomorphologyModel<Manager>, GeomorphologyTypes<Manager>>;
    using BCType = typename Base::BCType;

private:
    Manager _manager;
    BCType _bc;
    Utopia::DataIO::HDFFile _hdff;
    std::size_t _t;

public:
    /// Construct the model.
    /** \param manager Manager for this model
     */
    GeomorphologyModel (const Manager& manager, BCType bc):
        Base(),
        _manager(manager),
        _bc(bc),
        _t(0),
        _hdff("geomorphology-test.h5", "w")
    {
        //initialize with an inclined plane
        /*auto set_inclined_plane = [this](const auto cell) {          
            return 1.0;
        };
        apply_rule(set_inclined_plane, _manager.cells());*/ 
        //apply_rule doesnt really work here...
        auto& cells = _manager.cells();
        for (std::size_t i = 0; i < cells.size(); ++i) {
            cells[i]->state_new()[0] = i % cells.size(); //create an inclined plane
            cells[i]->update();
        }
        auto dsetX = _hdff.open_dataset("positionX");
        dsetX->write(_manager.cells().begin(),
                _manager.cells().end(), 
                [](const auto& cell) {return cell->position()[0];});
        auto dsetY = _hdff.open_dataset("positionY");
        dsetY->write(_manager.cells().begin(),
                _manager.cells().end(), 
                [](const auto& cell) {return cell->position()[1];});
        write_data();
    }

    /// Iterate one time step
    void perform_step ()
    {

        ++_t;
        // let it rain
        auto rain = [this](const auto cell) {
            auto rain = _bc(*(_manager.rng()));
            auto state = cell->state();
            state[1] += rain; 
            return state;
        };
        apply_rule(rain, _manager.cells());

        // reset state_new of cells
        auto& cells = _manager.cells();
        for (auto& cell : cells) {
            cell->state_new()[1] = 0;
        }

        // waterflow, i.e.  
        for (auto& cell : cells) {
            //find lowest neighbour
            auto neighbors = Neighborhoods::MooreNeighbor::neighbors(cell, _manager);
            auto l_neighbor = neighbors[0];
            double min_height = l_neighbor->state()[0];
            for (auto n : neighbors) {
                if (n->state()[0] < min_height) {
                    l_neighbor = n;
                    min_height = l_neighbor->state()[0];
                }
            }
            // put watercontent from cell to l_neighbor
            l_neighbor->state_new()[1] += cell->state()[1];
        }

        // update cells
        for (auto& cell : cells) {
            cell->update();
        }

        write_data();
    }

    void write_data () {
        std::cout << "Writing data @ _t = " << _t << " ... ";
        auto dsetH = _hdff.open_dataset("height@t="+std::to_string(_t));
        dsetH->write(_manager.cells().begin(),
                _manager.cells().end(), 
                [](const auto cell){ return cell->state()[0]; });
        auto dsetW = _hdff.open_dataset("watercontent@t="+std::to_string(_t));
        dsetW->write(_manager.cells().begin(),
                _manager.cells().end(), 
                [](const auto cell){ return cell->state()[0]; });
        std::cout << "complete!\n";
    }


    /// Return const reference to cell container
    const Data& data () const { return _manager.cells(); }

    // Set model boundary condition
    void set_boundary_condition (const BCType& bc) { }

    /// Set model initial condition
    void set_initial_condition (const Data& container) { }
};


/*
namespace Setup {

GeomorphologyModel geomorphology(const unsigned int grid_size)
{
    constexpr bool sync = true;
    using State = std::array<double, 2>; //height, watercontent
    using Tag = Utopia::DefaultTag;
    State initial_state = {0.0, 0.0};

    auto grid  = Utopia::Setup::create_grid(grid_size);
    auto cells = Utopia::Setup::create_cells_on_grid<sync, State, Tag>(grid, initial_state);
    auto mngr  = Utopia::Setup::create_manager_cells<true, true>(grid, cells);
    std::normal_distribution<> rain{10,2};

    return GeomorphologyModel(mngr, rain);
}

} // namespace Setup
*/
} // namespace Utopia

#endif // GEOMORPHOLOGY_HH

