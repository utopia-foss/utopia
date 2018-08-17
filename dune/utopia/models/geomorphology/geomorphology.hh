#ifndef GEOMORPHOLOGY_HH
#define GEOMORPHOLOGY_HH

#include <dune/utopia/base.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>

#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/hdfdataset.hh>

namespace Utopia {
namespace Models {
namespace Geomorphology {

/// State struct for Geomorphology model
struct State {
    double height;
    double watercontent;
};

/// Define data type and boundary condition type of geomorphology model

using Rain = std::normal_distribution<>;

template<class Manager>
using GeomorphologyTypes = ModelTypes<typename Manager::Container, Rain>;

/// A very simple geomorphology model
template<class Manager>
class Geomorphology:
    public Model<Geomorphology<Manager>, GeomorphologyTypes<Manager>>
{
public:

    /// Type helpers
    using Base = Model<Geomorphology<Manager>, GeomorphologyTypes<Manager>>;
    using BCType = typename Base::BCType;
    using Data = typename Base::Data;
    using DataSet = DataIO::HDFDataset<DataIO::HDFGroup>;

private:

    /// The grid manager
    Manager _manager;

    /// The boundary conditions (aka parameters) of the model
    BCType _bc;

    /// Datasets
    std::shared_ptr<DataSet> _dset_water_content;
    std::shared_ptr<DataSet> _dset_height;

public:

    /// Construct the Geomorphology model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template<class ParentModel>
    Geomorphology (const std::string name,
                   const ParentModel& parent_model,
                   Manager manager)
    :
        // Construct the base class
        Base(name, parent_model),

        // Initialise the reference to the Manager object
        _manager(manager),

        // Initialize model parameters from config file
        _bc{this->_cfg["rain_mean"].template as<double>(), 
            this->_cfg["rain_var"].template as<double>()},

        // Open datasets for output of cell states 
        _dset_water_content(this->_hdfgrp->open_dataset("water_content")),
        _dset_height(this->_hdfgrp->open_dataset("height"))
    {
        // Initialize altitude as an inclined plane (by making use of coordinates)
        auto set_inclined_plane = [](const auto cell) {   
            auto state = cell->state();
            state.height = cell->position()[1]; 
            return state;       
        };
        apply_rule(set_inclined_plane, manager.cells()); 

        // Add the model parameters as attributes
        this->_hdfgrp->add_attribute("rain_mean", 
                                    this->_cfg["rain_mean"].template as<double>());
        this->_hdfgrp->add_attribute("rain_var", 
                                    this->_cfg["rain_var"].template as<double>());

        // Write the cell coordinates
        auto coords = this->_hdfgrp->open_dataset("cell_positions",
                                                  {_manager.cells().size()});
        coords->write(_manager.cells().begin(),
                      _manager.cells().end(),
                      [](const auto& cell) {
                        return std::array<double,2>
                            {{cell->position()[0],
                              cell->position()[1]}};
                      }
        );

        // Set dataset capacities
        const hsize_t num_cells = std::distance(_manager.cells().begin(),
                                                _manager.cells().end());
        this->_log->debug("Setting dataset capacities to {} x {} ...",
                          this->get_time_max() + 1, num_cells);
        _dset_water_content->set_capacity({this->get_time_max() + 1, num_cells});
        _dset_height->set_capacity({this->get_time_max() + 1, num_cells});

        // Write initial state 
        write_data();
    }

    /// Iterate a single step
    void perform_step ()
    {
        // Let it rain
        auto rain = [this](const auto cell) {
            auto rain = _bc(*(_manager.rng()));
            auto state = cell->state();
            state.watercontent += rain; 
            return state;
        };
        apply_rule(rain, _manager.cells());

        // Reset state_new of cells
        auto& cells = _manager.cells();
        for (auto& cell : cells) {
            cell->state_new().watercontent = 0;
        }

        // Waterflow  
        for (auto& cell : cells) {
            // Find lowest neighbour
            auto neighbors = Neighborhoods::NextNeighbor::neighbors(cell, _manager);
            auto l_neighbor = neighbors[0];
            for (auto neighbor : neighbors) {
                if (neighbor->state().height < l_neighbor->state().height) 
                    l_neighbor = neighbor;
            }
            // Put watercontent from cell to l_neighbor
            l_neighbor->state_new().watercontent += cell->state().watercontent;
        }

        // Remove water from boundary cells (sinks)
        for (auto& cell : cells) {
            if (cell->is_boundary())
                cell->state_new().watercontent = 0;
        }

        // Update cells
        for (auto& cell : cells) {
            cell->update();
        }
    }

    /// Write the cell states (aka water content)
    /** The cell height is currently not written out as in the current implementation 
     *  it does not change over time (erosion is not yet included).
     */
    void write_data () {

        _dset_water_content->write(_manager.cells().begin(),
                                   _manager.cells().end(),
                                   [](auto& cell) { return cell->state().watercontent; }
                                  );

        _dset_height->write(_manager.cells().begin(),
                                   _manager.cells().end(),
                                   [](auto& cell) { return cell->state().height; }
                                  );
    }

    /// Return const reference to cell container
    const Data& data () const { return _manager.cells(); }

    /// Set model boundary condition
    void set_boundary_condition (const BCType& new_bc) { _bc = new_bc; }


    /// Set model initial condition
    void set_initial_condition (const Data& ic)
    {
        assert(check_ic_compatibility(ic));

        auto& cells = _manager.cells();
        for (size_t i = 0; i < cells.size(); ++i) {
            cells[i]->state_new() = ic[i]->state();
            cells[i]->update();
        }
    }

private:

    // Verify that the inserted initial condition can be used
    /** This function checks if
     *      - the inserted container has the appropriate size
     *      - the coordinates of all cells match individually
     *  \param ic Initial condition to be used
     */
    bool check_ic_compatibility (const Data& ic)
    {
        const auto& cells = _manager.cells();

        // check size
        if (ic.size() != cells.size()) {
            throw std::runtime_error("Container inserted as initial condition "
                "has incorrect size " + std::to_string(ic.size())
                + " (should be " + std::to_string(cells.size() + ")"));
        }

        // check coordinates
        for (size_t i = 0; i < cells.size(); ++i)
        {
            const auto& coord_1 = cells[i]->position();
            const auto& coord_2 = ic[i]->position();
            for (size_t j = 0; j < coord_1.size(); j++) {
                if (coord_1[j] != coord_2[j])
                    return false;
            }
        }
        return true;
    }
};


} // namespace Geomorphology
} // namespace Models
} // namespace Utopia

#endif // GEOMORPHOLOGY_HH

