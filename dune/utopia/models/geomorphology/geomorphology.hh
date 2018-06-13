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

namespace Models {

/// Define data types of vegetation model
template<class Manager>
using GeomorphologyTypes = Utopia::ModelTypes<
    typename Manager::Container,
    std::normal_distribution<> // rain
>;

template<class Manager>
class Geomorphology:
    public Utopia::Model<Geomorphology<Manager>, GeomorphologyTypes<Manager>>
{
public:

    using Base = Utopia::Model<Geomorphology<Manager>, GeomorphologyTypes<Manager>>;
    using BCType = typename Base::BCType;
    using Data = typename Base::Data;

    Manager manager;
    BCType bc;

    /// Construct the model.
    /** \param manager Manager for this model
     */
    Geomorphology (const std::string name,
                const ParentModel & parent_model,
                Manager _manager)
    :
        // Use the base constructor for the main parts
        Base(name, parent_model),
        // Initialise the manager
        manager(_manager),
        bc{this->cfg["rain_mean"].as<double>(), this->cfg["rain_var"].as<double>()}
    {
        // Initialize altitude as an inclined plane
        /*auto set_inclined_plane = [this](const auto cell) {          
            return 1.0;
        };
        apply_rule(set_inclined_plane, manager.cells());*/ 
        //apply_rule doesnt really work here...
        for (auto cell : manager.cells()) {
            cell->state_new()[0] = cell->position()[1];
            cell->update();
        }

        // Write initial condition and position of cells
        auto dsetX = this->hdfgrp.open_dataset("positionX");
        dsetX->write(manager.cells().begin(),
                manager.cells().end(), 
                [](const auto& cell) {return cell->position()[0];});
        auto dsetY = this->hdfgrp.open_dataset("positionY");
        dsetY->write(manager.cells().begin(),
                manager.cells().end(), 
                [](const auto& cell) {return cell->position()[1];});
        write_data();
    }

    /// Iterate one time step
    void perform_step ()
    {

        // let it rain
        auto rain = [this](const auto cell) {
            auto rain = bc(*(manager.rng()));
            auto state = cell->state();
            state[1] += rain; 
            return state;
        };
        apply_rule(rain, manager.cells());

        // reset state_new of cells
        auto& cells = manager.cells();
        for (auto& cell : cells) {
            cell->state_new()[1] = 0;
        }

        // waterflow  
        for (auto& cell : cells) {
            //find lowest neighbour
            auto neighbors = Neighborhoods::NextNeighbor::neighbors(cell, manager);
            auto l_neighbor = neighbors[0];
            double min_height = l_neighbor->state()[0];
            for (auto n : neighbors) {
                if (n->state()[0] < min_height) {
                    l_neighbor = n;
                    min_height = l_neighbor->state()[0];
                }
            }
            // model boundary as a sink
            if (l_neighbor->is_boundary()) {
                l_neighbor->state_new()[1] = 0;
            }
            // put watercontent from cell to l_neighbor
            else {
                l_neighbor->state_new()[1] += cell->state()[1];
            }
        }

        // update cells
        for (auto& cell : cells) {
            cell->update();
        }
    }

    void write_data () {
        std::cout << "Writing data @ _t = " << this->time << " ... ";
        auto dsetH = this->hdfgrp.open_dataset("height@t="+std::to_string(this->time));
        dsetH->write(manager.cells().begin(),
                     manager.cells().end(), 
                     [](const auto cell){ return cell->state()[0]; });
        auto dsetW = this->hdfgrp.open_dataset("watercontent@t="+std::to_string(this->time));
        dsetW->write(manager.cells().begin(),
                     manager.cells().end(), 
                     [](const auto cell){ return cell->state()[1]; });
        std::cout << "complete!\n";
    }


    /// Return const reference to cell container
    const Data& data () const { return manager.cells(); }

    // Set model boundary condition
    void set_boundary_condition (const BCType& bc) { }

    /// Set model initial condition
    void set_initial_condition (const Data& container) { }
};


} // namespace Utopia

#endif // GEOMORPHOLOGY_HH

