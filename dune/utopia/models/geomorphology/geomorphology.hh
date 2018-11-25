#ifndef GEOMORPHOLOGY_HH
#define GEOMORPHOLOGY_HH

#include <cmath>

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

/// Define boundary condition type of geomorphology model
using Rain = std::normal_distribution<>;
struct GeomorphologyParameters {

    // Constructor
    GeomorphologyParameters(double _rain_mean, double _rain_var, 
            double _height, double _uplift, double _erodibility) : 
        rain{_rain_mean, _rain_var}, height(_height), 
        uplift(_uplift), erodibility(_erodibility) {}

    Rain rain;
    double height;
    double uplift;
    double erodibility;
};

/// Typehelper to define data types of Geomorphology model
using GeomorphologyTypes = ModelTypes<>;

/// A very simple geomorphology model
template<class ManagerType>
class Geomorphology:
    public Model<Geomorphology<ManagerType>, GeomorphologyTypes>
{
public:

    // -- Type helpers -- //
    using Base = Model<Geomorphology<ManagerType>, GeomorphologyTypes>;
    using CellType = typename ManagerType::Cell;
    using CellIndexType = typename CellType::Index;
    using DataSet = DataIO::HDFDataset<DataIO::HDFGroup>;

private:

    /// The grid manager
    ManagerType _manager;
  
    /// The boundary conditions (aka parameters) of the model
    GeomorphologyParameters _params;

    /// Datasets
    std::shared_ptr<DataSet> _dset_water_content;
    std::shared_ptr<DataSet> _dset_height;

    // A map of lowest neighbors
    std::map<CellIndexType, std::shared_ptr<CellType>> _lowest_neighbors;

    // Rule functions
    std::function<State(std::shared_ptr<CellType>)> _rain = [this](const auto cell){
        auto rain = _params.rain(*(this->_rng));
        auto state = cell->state();
        state.watercontent += rain; 
        return state;
    };


public:

    /// Construct the Geomorphology model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param manager  The externally setup manager to use for this model
     */
    template<class ParentModel>
    Geomorphology (const std::string name,
                   const ParentModel& parent_model,
                   ManagerType manager)
    :
        // Construct the base class
        Base(name, parent_model),

        // Initialise the reference to the ManagerType object
        _manager(manager),

        // Initialize model parameters from config file
        _params(this->_cfg["rain_mean"].template as<double>(), 
            this->_cfg["rain_var"].template as<double>(),
            this->_cfg["height"].template as<double>(),
            this->_cfg["uplift"].template as<double>(),
            this->_cfg["erodibility"].template as<double>()),

        // Open datasets for output of cell states 
        _dset_water_content(this->_hdfgrp->open_dataset("water_content")),
        _dset_height(this->_hdfgrp->open_dataset("height"))

    {
        // Initialize altitude as an inclined plane (by making use of coordinates)
        auto set_inclined_plane = [this](const auto cell) {   
            auto state = cell->state();
            state.height = this->_cfg["slope"].template as<double>()*cell->position()[1] + 
                this->_cfg["height"].template as<double>(); 
            return state;
        };
        apply_rule(set_inclined_plane, manager.cells()); 

        // Add the model parameters as attributes
        this->_hdfgrp->add_attribute("rain_mean", 
                                    this->_cfg["rain_mean"].template as<double>());
        this->_hdfgrp->add_attribute("rain_var", 
                                    this->_cfg["rain_var"].template as<double>());
        this->_hdfgrp->add_attribute("height", 
                                    this->_cfg["height"].template as<double>());
        this->_hdfgrp->add_attribute("slope", 
                                    this->_cfg["slope"].template as<double>());
        this->_hdfgrp->add_attribute("uplift", 
                                    this->_cfg["uplift"].template as<double>());
        this->_hdfgrp->add_attribute("erodibility", 
                                    this->_cfg["erodibility"].template as<double>());

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
        auto cells = _manager.cells();

        // Update lowest neighbors
        update_lowest_neighbors();

        // Let it rain
        apply_rule(_rain, cells);

        // Sediment flow
        for (auto& cell : cells) {
            //std::cout << cell->position()[0] << ' ' << cell->position()[1] << std::endl;

            double sediment_flow = _params.erodibility*std::sqrt(cell->state().watercontent);

            // Constant outflow for cells on the lower boundary
            if (is_on_lower_boundary(cell)) {
                cell->state_new().height -= sediment_flow;
            } 
            else {
                // Compute height difference to lowest neighbor
                auto lowest_neighbor = _lowest_neighbors[cell->id()];
                double delta_height = cell->state().height - lowest_neighbor->state().height;
                if (delta_height < 0 || cell->state().watercontent < 0) continue;

                // Compute sediment flow and substract from cell height
                cell->state_new().height -= sediment_flow * delta_height;
                   
            // Check if new height is nan or inf, in that case set to zero
            if (std::isnan(cell->state_new().height) || std::isinf(cell->state_new().height)) {
                this->_log->debug("Cell ID {}, delta_height {}, water {}, new height {}",
                              cell->id(), delta_height, cell->state().watercontent, cell->state_new().height);
                cell->state_new().height = 0;
            }

            // Check if new height is negative, in that case set to zero
            if (cell->state_new().height < 0)
                cell->state_new().height = 0;

            // Would be reasonable but gives not so nice results: Add sediment flow to lowest neighbor...
            //if (cell->id() >= 20) // Let sediments flow out of lower boundary, really badly hard-coded.
                //    _lowest_neighbors[cell->id()]->state_new().height += sediment_flow;
            }
        }

        // Prepare state_new for waterflow
        for (auto& cell : cells) {
            cell->state_new().watercontent = 0;
        }

        // Waterflow  
        for (auto& cell : cells) {
            auto lowest_neighbor = _lowest_neighbors[cell->id()];
            // Put water onto another cell only if I'm not a boundary cell
            if (not cell->is_boundary())
                lowest_neighbor->state_new().watercontent += cell->state().watercontent;
        }

        // Uplift  
        for (auto& cell : cells) {
            cell->state_new().height += _params.uplift;
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

    /// Monitor model information
    void monitor () { }

private:

    /// Check if the cell is on the "lower" boundary 
    /** The lower boundary is the boundary, where the cell heights are minimal
     * (at least in the initial configuration) 
     */
    bool is_on_lower_boundary(std::shared_ptr<CellType> cell) {
        return cell->position()[1] < 1;
    }

    /// Update the map of lowest neighbors
    void update_lowest_neighbors() {
    
        for (auto& cell : _manager.cells()) {

            // Find lowest neighbour
            auto neighbors = Neighborhoods::MooreNeighbor::neighbors(cell, _manager);
            CellContainer<CellType> lowest_neighbors;
            auto lowest_neighbor = cell; // _lowest_neighbors of a cell is itself, if it is a sink
            lowest_neighbors.push_back(lowest_neighbor);
            for (auto neighbor : neighbors) {
                auto height_diff = neighbor->state().height - lowest_neighbor->state().height;

                // Check if cells have approximately the same height.
                // For problems of float comparison see:
                // https://stackoverflow.com/questions/17333/what-is-the-most-effective-way-for-float-and-double-comparison
                if ((height_diff < 1e-6) && (-height_diff < 1e-6)) {
                    lowest_neighbors.push_back(neighbor);
                }

                // If neighbor is lower, update lowest_neighbor and list
                else if (height_diff < 0) {
                    lowest_neighbor = neighbor;
                    lowest_neighbors.clear();
                    lowest_neighbors.push_back(lowest_neighbor);
                }
            }

            // If there is more than one lowest neighbor, select one randomly.
            if (lowest_neighbors.size() > 1) {
                std::uniform_int_distribution<> dis(0, lowest_neighbors.size() - 1);
                lowest_neighbor = lowest_neighbors[dis(*(this->_rng))];
            }

            // Set lowest neighbor for cell
            _lowest_neighbors[cell->id()] = lowest_neighbor;
        }
    }

    // Verify that the inserted initial condition can be used
    /** This function checks if
     *      - the inserted container has the appropriate size
     *      - the coordinates of all cells match individually
     *  \param ic Initial condition to be used
     */
 //   bool check_ic_compatibility (const Data& ic)
 //   {
 //       const auto& cells = _manager.cells();
 //
 //       // check size
 //       if (ic.size() != cells.size()) {
 //           throw std::runtime_error("Container inserted as initial condition "
 //               "has incorrect size " + std::to_string(ic.size())
 //               + " (should be " + std::to_string(cells.size() + ")"));
 //       }
 //
 //       // check coordinates
 //       for (size_t i = 0; i < cells.size(); ++i)
 //       {
 //           const auto& coord_1 = cells[i]->position();
 //           const auto& coord_2 = ic[i]->position();
 //           for (size_t j = 0; j < coord_1.size(); j++) {
 //               if (coord_1[j] != coord_2[j])
 //                   return false;
 //           }
 //       }
 //       return true;
 //   }
};


} // namespace Geomorphology
} // namespace Models
} // namespace Utopia

#endif // GEOMORPHOLOGY_HH

