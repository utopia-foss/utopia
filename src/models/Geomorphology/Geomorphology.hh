#ifndef GEOMORPHOLOGY_HH
#define GEOMORPHOLOGY_HH

#include <cmath>
#include <functional>
#include <random>

#include <utopia/core/model.hh>
#include <utopia/core/apply.hh>
#include <utopia/core/cell_manager.hh>

namespace Utopia {
namespace Models {
namespace Geomorphology {

/// State struct for Geomorphology model
struct GeomorphologyCellState {
    double height;
    double watercontent;
};

/// The full cell struct for the Geomorphology model
template <typename IndexContainer>
struct GeomorphologyCell {
    // The actual cell state
    /// The cells topgraphic height
    double height;
    /// The cells watercontent
    double watercontent;

    /// Whether the cell is an outflow boundary cell
    bool is_outflow;
    
    /// Construct a cell from a configuration node and an RNG
    template<class RNG>
    GeomorphologyCell (const DataIO::Config& cfg, const std::shared_ptr<RNG>& rng)
    :
        height(0.),
        watercontent(0.), is_outflow(false)
    { 
        std::normal_distribution<> init_height{
            get_as<double>("initial_height_mean", cfg), 
            get_as<double>("initial_height_var", cfg)
        };
        height = init_height(*rng);
    }
};

/// Cell traits specialization using the state type
/** \detail The first template parameter specifies the type of the cell state,
  *         the second sets them to not be synchronously updated.
  *         See \ref Utopia::CellTraits
  * 
  * \note   This model relies on asynchronous update for calculation of the
  *         clusters and the percolation.
  */
using GeomorphologyCellTraits = Utopia::CellTraits<GeomorphologyCell<
                                                        Utopia::IndexContainer>, 
                                                   UpdateMode::async,
                                                   false>;

/// Typehelper to define data types of ForestFire model
using GeomorphologyTypes = ModelTypes<>;

/// A very simple geomorphology model
class Geomorphology:
    public Model<Geomorphology, GeomorphologyTypes>
{
public:
    /// The base model type
    using Base = Model<Geomorphology, GeomorphologyTypes>;
    
    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// The type of the cell manager
    using GeomorphologyCellManager = CellManager<GeomorphologyCellTraits,
                                                 Geomorphology>;

    /// A cell in the geomorphology model
    using GeomorphologyCellType = GeomorphologyCellManager::Cell;

    /// The index type of the Geomorphology cell manager
    using GeomorphologyCellIndexType = Utopia::IndexType;

    /// The type of a cell container in the Geomorphology model
    using GeomorphologyCellContainer = Utopia::CellContainer<GeomorphologyCellType>;

    /// Rule function type, extracted from CellManager
    using RuleFunc = typename GeomorphologyCellManager::RuleFunc;

private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor

    // -- Members -------------------------------------------------------------
    /// The cell manager for the forest fire model
    GeomorphologyCellManager _cm;
  
    // The boundary conditions (aka parameters) of the model
    /// 
    std::normal_distribution<> _rain;
    std::normal_distribution<> _uplift;
    double _erodibility;

    // A map of lowest neighbors
    std::map<GeomorphologyCellIndexType, 
             std::shared_ptr<GeomorphologyCellType>> _lowest_neighbors;

    /// Datasets
    std::shared_ptr<DataSet> _dset_height;
    std::shared_ptr<DataSet> _dset_water_content;


public:
    // -- Model Setup ---------------------------------------------------------
    /// Construct the Geomorphology model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     */
    template<class ParentModel>
    Geomorphology (const std::string name, ParentModel& parent)
    :
        // Construct the base class
        Base(name, parent),

        // Initialize the cell manager, binding it to this model
        _cm(*this),
        _rain{get_as<double>("rain_mean", this->_cfg), 
              get_as<double>("rain_var", this->_cfg)},
        _uplift{get_as<double>("uplift_mean", this->_cfg), 
                get_as<double>("uplift_var", this->_cfg)},
        _erodibility(get_as<double>("erodibility", this->_cfg)),

        // Create datasets using the helper functions for CellManager-data
        _dset_height(this->create_cm_dset("height", _cm)),
        _dset_water_content(this->create_cm_dset("water_content", _cm))

    {
        // Initialize altitude as an inclined plane (by making use of coordinates)
        RuleFunc set_inclined_plane = [this](const auto& cell) {   
            auto state = cell->state();
            auto pos = _cm.barycenter_of(cell);
            double slope = get_as<double>("initial_slope", 
                                this->_cfg["cell_manager"]["cell_params"]);
            state.height += slope*pos[1]; 
            return state;
        };
        apply_rule<false>(set_inclined_plane, _cm.cells());

        // set bottom cells as outflow
        apply_rule(
            // The rule to apply
            [](const auto& cell){
                auto state = cell->state();
                state.is_outflow = true;
                return state;
            },
            // The containers over which to iterate
            _cm.boundary_cells("bottom"),
            // The RNG needed for apply_rule calls with async update
            *this->_rng
        );

        // let it rain for the first time
        apply_rule<false>(
            // The rule to apply
            [this](const auto& cell){
                auto state = cell->state();
                state.watercontent += this->_rain(*(this->_rng)); 
                return state;
            },
            // The containers over which to iterate
            _cm.cells()
        );
        
        // update network
        _update_lowest_neighbors();
        this->_log->debug("Cells fully set up.");

        // Write initial state
        this->write_data();

        this->_log->debug("{} model all set up and initial state written.",
                          this->_name);
    }

    /// Perform step
    void perform_step () {
        // apply update rule on all cells, asynchronously and shuffled
        apply_rule(_erode, _cm.cells(), *this->_rng);

        // Uplift  
        apply_rule<false>(
            // The rule to apply
            [this](const auto& cell){
                auto state = cell->state();
                state.height += this->_uplift(*(this->_rng));
                return state;
            },
            // The containers over which to iterate
            _cm.cells()
        );

        // reset network
        apply_rule<false>(
            // The rule to apply
            [](const auto& cell){
                auto state = cell->state();
                state.watercontent = 0.;
                return state;
            },
            // The containers over which to iterate
            _cm.cells()
        );

        // let it rain
        apply_rule<false>(
            // The rule to apply
            [this](const auto& cell){
                auto state = cell->state();
                state.watercontent += this->_rain(*(this->_rng)); 
                return state;
            },
            // The containers over which to iterate
            _cm.cells()
        );

        // update network
        _update_lowest_neighbors();
    }

    /// Provide monitoring data: tree density and number of clusters
    void monitor () { return; }

    /// Write the cell states (aka water content)
    /** The cell height is currently not written out as in the current implementation 
     *  it does not change over time (erosion is not yet included).
     */
    void write_data () {
        _dset_height->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) { return cell->state().height; }
        );

        _dset_water_content->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) { return cell->state().watercontent; }
        );
    }

private:
    /// Update the map of lowest neighbors
    void _update_lowest_neighbors() {    
        for (auto& cell : _cm.cells()) {
            auto state = cell->state();

            // Find lowest neighbour
            GeomorphologyCellContainer lowest_neighbors;
            auto lowest_neighbor = cell; // _lowest_neighbors of a cell is itself, if it is a sink
            lowest_neighbors.push_back(lowest_neighbor);
            for (auto neighbor : this->_cm.neighbors_of(cell)) {
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

    RuleFunc _erode = [this](const auto& cell) {
        auto state = cell->state();

        double sediment_flow = _erodibility*std::sqrt(state.watercontent);

        // Constant outflow for cells on the lower boundary
        if (state.is_outflow) {
            state.height -= sediment_flow;
        } 
        else {
            // Compute height difference to lowest neighbor
            auto lowest_neighbor = _lowest_neighbors[cell->id()];
            double delta_height = state.height - lowest_neighbor->state().height;
            if (delta_height < 0) {
                delta_height = 0;
            }

            // Compute sediment flow and substract from cell height
            state.height -= sediment_flow * delta_height;
                
            // Check if new height is nan or inf, in that case set to zero
            if (std::isnan(state.height) || std::isinf(state.height)) {
                this->_log->debug("Cell ID {}, delta_height {}, water {}, new height {}",
                                cell->id(), delta_height, state.watercontent, state.height);
                state.height = 0;
            }

            // Check if new height is negative, in that case set to zero
            if (state.height < 0) {
                state.height = 0;
            }
        }

        return state;
    };
};


} // namespace Geomorphology
} // namespace Models
} // namespace Utopia

#endif // GEOMORPHOLOGY_HH

