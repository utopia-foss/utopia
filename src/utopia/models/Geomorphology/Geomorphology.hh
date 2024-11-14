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

/// The full cell struct for the Geomorphology model
struct GeomorphologyCell {
    /// The cells topgraphic height
    double rock;

    /// The cells watercontent
    double watercolumn;

    /// The height of the waterline
    double waterline() const { return rock + watercolumn; };

    /// The drainage area
    double drainage_area;

    /// Was called for drainage area calculation
    bool was_drained;

    /// Whether the cell is an outflow boundary cell
    bool is_outflow;

    /// Construct a cell from a configuration node and an RNG
    /** \details Initializes cell with normal_distributed(mean, var) rock height.
     *        The distribution is cut for values below 0, giving a minimal
     *        height of 0.
     */
    template<class RNG>
    GeomorphologyCell (const DataIO::Config& cfg,
                       const std::shared_ptr<RNG>& rng)
    :
        rock(0.), watercolumn(0.), drainage_area(1.), was_drained(false),
        is_outflow(false)
    {
        std::normal_distribution<> init_height{
            get_as<double>("initial_height_mean", cfg),
            get_as<double>("initial_height_var", cfg)
        };
        rock = init_height(*rng);
    }
};


/// Cell traits specialization using the state type
/** \details The first template parameter specifies the type of the cell state,
 *          the second sets them to not be synchronously updated.
 *          See \ref Utopia::CellTraits
 *
 *  \note   This model relies on asynchronous update for calculation of the
 *          clusters and the percolation.
 */
using CellTraits = Utopia::CellTraits<GeomorphologyCell, Update::manual>;

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
    using GeomorphologyCellManager = CellManager<CellTraits, Geomorphology>;

    /// A cell in the geomorphology model
    using GeomorphologyCellType = GeomorphologyCellManager::Cell;

    /// The index type of the Geomorphology cell manager
    using GeomorphologyCellIndexType = Utopia::IndexType;

    /// The type of a cell container in the Geomorphology model
    using GmorphCellContainer = Utopia::CellContainer<GeomorphologyCellType>;

    /// Rule function type, extracted from CellManager
    using RuleFunc = typename GeomorphologyCellManager::RuleFunc;

private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor

    // -- Members -------------------------------------------------------------
    
    /// The cell manager for the forest fire model
    GeomorphologyCellManager _cm;

    
    // -- The boundary conditions (aka parameters) of the model
    
    /// The random uplift as normal distribution
    std::normal_distribution<> _uplift;
     
    /// The stream power coefficient
    double _stream_power_coef;
    
    /// The frequency of possible toppling events per cell
    double _toppling_frequency;
     
    /// The critical height difference to trigget a toppling event
    /** The probability of a toppling event is given as 
     *  \f$ p = \Delta h / h_{crit} \f$, where \f$ \Delta h \f$ is the
     *  difference in height between a target cell and its highest neighbor.
     */
    double _toppling_critical_height;
    
    /// The factor by which to reduce the slope in a toppling event
    /** The difference in height between the source and target cell of the
     *  toppling event is divided by this factor.
     *  This amount of rock is removed from the source and added to the target
     *  site.
     * 
     *  default value: 3.
     */
    double _toppling_slope_reduction_factor;

    // A map of lowest neighbors
    std::unordered_map<GeomorphologyCellIndexType,
             std::shared_ptr<GeomorphologyCellType>> _lowest_neighbors;


    const double _float_precision; /// precision when comparing floats

    /// A re-usable uniform real distribution to evaluate probabilities
    std::uniform_real_distribution<> _prob_dist;

    // -- Datasets ------------------------------------------------------------
    std::shared_ptr<DataSet> _dset_height;          /// Dataset of rock height
    std::shared_ptr<DataSet> _dset_drainage_area;   /// Dataset of drainage area
    std::shared_ptr<DataSet> _dset_watercolumn;     /// Dataset of watercolumn


public:
    // -- Model Setup ---------------------------------------------------------
    /// Construct the Geomorphology model
    /** \param name             Name of this model instance; is used to extract
     *                          the configuration from the parent model and
     *                          set up a HDFGroup for this instance
     *  \param parent_model     The parent model this model instance resides in
     *  \param custom_cfg       A custom configuration to use instead of the
     *                          one extracted from the parent model using the
     *                          instance name
     */
    template<class ParentModel>
    Geomorphology (
        const std::string& name,
        ParentModel& parent_model,
        const DataIO::Config& custom_cfg = {}
    )
    :
        // Construct the base class
        Base(name, parent_model, custom_cfg),

        // Initialize the cell manager, binding it to this model
        _cm(*this),

        _uplift{get_as<double>("uplift_mean", this->_cfg),
                get_as<double>("uplift_var", this->_cfg)},
        _stream_power_coef(get_as<double>("stream_power_coef", this->_cfg)),
        _toppling_frequency(get_as<double>("toppling_frequency", this->_cfg)),
        _toppling_critical_height(get_as<double>("toppling_critical_height",
                                  this->_cfg)),
        _toppling_slope_reduction_factor(
            get_as<double>("toppling_slope_reduction_factor", this->_cfg, 3.)),

        _float_precision(1e-10),
        _prob_dist(0., 1.),

        // Create datasets using the helper functions for CellManager-data
        _dset_height(this->create_cm_dset("height", _cm)),
        _dset_drainage_area(this->create_cm_dset("drainage_area", _cm)),
        _dset_watercolumn(this->create_cm_dset("watercolumn", _cm))
    {
        if (get_as<double>("uplift_var", this->_cfg) <= 1e-10) {
            throw std::invalid_argument("Invalid argument: uplift_var must be "
                "> _float_precision (1e-10)!");
        }
        // Update initial cell states
        _initialize_cells();


        this->_log->debug("{} model fully set up.", this->_name);
    }

    /// The set of seperately applied rules to build the drainage network
    /** 1. Reset network
     *  2. Connect cells to basic netork
     *  3. Fill sinks with water, completing the network
     *  4. Calculate the drainage area on every cell
     */
    void build_network () {
        // reset network
        apply_rule<Update::sync>(reset_network, _cm.cells());

        // connect cells to drainage network
        apply_rule<Update::async, Shuffle::off>(connect_cells, _cm.cells());

        // fill sinks with water
        apply_rule<Update::async, Shuffle::off>(build_lake, _cm.cells());

        // get drainage area
        apply_rule<Update::async, Shuffle::off>(pass_drainage_area,
                                                _cm.cells());
    }

    /// Perform step
    void perform_step () {
        // Erode
        apply_rule<Update::sync>(erode, _cm.cells());

        // Uplift
        apply_rule<Update::sync>(uplift_rule, _cm.cells());

        // Topple
        apply_rule<Update::async, Shuffle::on>(toppling, _cm.cells(),
                                               *this->_rng);

        // Build drainage network
        build_network();
    }

    /// Provide monitoring data: tree density and number of clusters
    void monitor () { return; }

    /// Write the cell states (aka water content)
    /** The cell height is currently not written out as in the current
     *  implementation it does not change over time
     *  (erosion is not yet included).
     */
    void write_data () {
        _dset_height->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) { return cell->state.rock; }
        );

        _dset_drainage_area->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) { return cell->state.drainage_area; }
        );

        _dset_watercolumn->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) { return cell->state.watercolumn; }
        );
    }

private:
    // -- Helper functions ----------------------------------------------------
    /// Compare two floats for equality
    bool check_eq(const double& a, const double& b) {
        return (a-b < _float_precision) && (b-a < _float_precision);
    }

    /// Compare the waterline of two cells
    template <typename Cell>
    bool check_eq_waterline(const Cell& a, const Cell& b) {
        return check_eq(a->state.waterline(), b->state.waterline());
    }

    // -- Initialization function ---------------------------------------------
    /// The initialization of the cells
    /** Adds the inclination to the cells initial rock height.
     *
     *  Defines the outflow boundary.
     *
     *  Initializes the drainage network and lakes
     */
    void _initialize_cells() {
        this->_log->debug("Initializing cells ..");

        // Initialize altitude as an inclined plane
        // (by making use of coordinates)
        RuleFunc set_inclined_plane = [this](const auto& cell) {
            auto state = cell->state;
            auto pos = _cm.barycenter_of(cell);
            double slope = get_as<double>("initial_slope",
                                this->_cfg["cell_manager"]["cell_params"]);
            state.rock += slope*pos[1];
            if (state.rock < _float_precision) {
                std::uniform_real_distribution<> dist(0.,1e-5);
                state.rock = dist(*this->_rng);
                this->_log->warn("Received negative initial height. Was set "
                    "in [0.,1e-5]. Better chose the initial_height "
                    "distribution such that no negative values occur.");
            }
            return state;
        };
        apply_rule<Update::sync>(set_inclined_plane, _cm.cells());

        // set bottom cells as outflow
        apply_rule<Update::sync>(
            // The rule to apply
            [](const auto& cell){
                auto state = cell->state;
                state.is_outflow = true;
                return state;
            },
            // The containers over which to iterate
            _cm.boundary_cells("bottom")
        );

        // Build drainage network
        this->_log->debug(" Initializing drainage network ...");
        build_network();

        this->_log->debug("Cells fully set up.");
    }


    // -- Helper functions ----------------------------------------------------

    /// Return the lowest cell of the grid-Neighborhood, including cell itself
    template <typename Cell>
    Cell _lowest_grid_neighbor(const Cell& cell) {
        GmorphCellContainer lowest_neighbors = {cell};
        auto lowest_neighbor = cell;
        for (const auto& n : this->_cm.neighbors_of(cell)) {
            double height_diff = n->state.waterline() -
                                 lowest_neighbor->state.waterline();

            if (check_eq_waterline(n, lowest_neighbor)) {
                lowest_neighbors.push_back(n);
            }

            // If neighbor is lower, update lowest_neighbor and list
            else if (height_diff < 0) {
                lowest_neighbor = n;
                lowest_neighbors.clear();
                lowest_neighbors.push_back(lowest_neighbor);
            }
        }

        // If there is more than one lowest neighbor, select one randomly.
        if (lowest_neighbors.size() > 1) {
            std::uniform_int_distribution<> dist(0, lowest_neighbors.size() -1);
            lowest_neighbor = lowest_neighbors[dist(*(this->_rng))];
        }
        return lowest_neighbor;
    }

    /// Update the CellContainer of lake and shore cells
    /** updates the shore Container to match the new lake
     *
     *  \param lake Reference to a Container of lake cells. Lake: cells of
     *              equal waterline.
     *  \param shore Reference to the Cells neighboring the lake
     *
     *  \returns CellContainer of all cells connected and of same waterline as
     *           the given lake Container.
     */
    GmorphCellContainer update_lakesites(GmorphCellContainer& lake,
                                         GmorphCellContainer& shore)
    {
        auto it = std::begin(shore);
        while(it != std::end(shore)) {
            if (check_eq_waterline(*it, lake[0])) {
                lake.push_back(*it);
                auto nb = _cm.neighbors_of(*it);
                for (auto it_lake : lake) {
                    nb.erase(std::remove(nb.begin(), nb.end(), it_lake),
                             nb.end());
                }
                for (auto it_shore : shore) {
                    nb.erase(std::remove(nb.begin(), nb.end(), it_shore),
                             nb.end());
                }

                auto next_it = it;
                auto next = *next_it;
                if (next_it != shore.end())
                    next_it++;
                if (next_it != shore.end())
                    next = *next_it;
                it = shore.erase(it);
                if (!nb.empty()) {
                    if (it == shore.end())
                        next = nb[0];
                    shore.insert(shore.end(), nb.begin(), nb.end());
                    it = std::find(shore.begin(), shore.end(), next);
                }
            }
            else
                ++it;
        }

        return lake;
    };


    // -- Rule functions ------------------------------------------------------
    /// The rule how to uplift
    RuleFunc uplift_rule = [this](const auto& cell) {
        auto state = cell->state;
        state.rock += this->_uplift(*(this->_rng));
        return state;
    };

    /// The rule how to erode with stream power
    RuleFunc erode = [this](const auto& cell) {
        auto state = cell->state;

        double slope = state.waterline();
        if (not state.is_outflow) {
            // slope = state->waterline - lowest_neighbor->waterline
            slope -= _lowest_neighbors[cell->id()]->state.waterline();
        }
        // else: slope = state->waterline - 0.

        double stream_power = (  _stream_power_coef * slope
                               * std::sqrt(state.drainage_area));
        state.rock -= std::min(stream_power, state.rock);

        return state;
    };

    /// The rule how to topple / landslide
    /** \note Only evaluated with _toppling_frequency per cell
     *
     *  Failure of slope occurs with p = (h_i - h_j) / h_c
     *
     *  On failure slope is reduced to 1/3. of its initial value.
     */
    RuleFunc toppling = [this](const auto& cell) {
        auto state = cell->state;
        if (_toppling_frequency == 0. or
            _toppling_frequency < this->_prob_dist(*(this->_rng))
           ) {
            // Done here.
            return state;
        }

        auto nbrs = this->_cm.neighbors_of(cell);
        auto heighest_neighbor = cell;
        for (auto& nb : nbrs) {
            if (nb->state.waterline() > heighest_neighbor->state.waterline())
            {
                heighest_neighbor = nb;
            }
        }
        double relief = (  heighest_neighbor->state.waterline()
                         - state.waterline());
        double failure_prob = relief / _toppling_critical_height;
        if (this->_prob_dist(*(this->_rng)) < failure_prob) {
            double factor = _toppling_slope_reduction_factor;
            heighest_neighbor->state.rock -= relief / factor;
            state.rock += relief / factor;
        }

        return state;
    };

    /// The rule to reset the drainage network
    RuleFunc reset_network = [](const auto& cell) {
        auto state = cell->state;
        state.drainage_area = 1.;
        state.was_drained = false;
        state.watercolumn = 0.;
        return state;
    };

    /// Build a rudimentary network
    /** Create map: cells to their lowest_neighbor (random choice if multiple).
     *  Sinks (no lower neighbor or outflow) map to themselfes.
     */
    RuleFunc connect_cells = [this](const auto& cell) {
        if (cell->state.is_outflow) {
            _lowest_neighbors[cell->id()] = cell; // map to itself
            return cell->state;
        }

        // Set lowest neighbor for cell, is itself is sink
        _lowest_neighbors[cell->id()] = this->_lowest_grid_neighbor(cell);

        return cell->state;
    };

    /// Fill a sink with water
    /** such that there is a shore cell to the resulting lake that is either
     *  outflow or lower than the lakes waterline.
     *
     *  Updates the _lowest_neighbors map for all lake cells to point to the
     *  outflow alias a Cell within lake neighboring the lower shore cell.
     *
     *  \note Must be updated in an asynchronous way. No shuffle needed.
     */
    RuleFunc build_lake = [this](auto& cell) {
        // return if cell has lower neighbor
        if (cell->state.is_outflow or _lowest_neighbors[cell->id()] != cell) {
            return cell->state;
        }

        GmorphCellContainer lake = {cell};
        GmorphCellContainer shore = this->_cm.neighbors_of(cell);
        lake = update_lakesites(lake, shore);

        double waterline = lake[0]->state.waterline();
        bool no_sink = true;

        // check for sink in new lake sites
        for (const auto& lc : lake) {
            if (lc->state.is_outflow) {
                no_sink=false;
                break;
            }
        }

        // lowest shore cell is candidate for outflow
        auto lowest_shore_cell = shore[0];
        for (const auto& sc : shore) {
            if (sc->state.waterline() < lowest_shore_cell->state.waterline())
            {
                lowest_shore_cell = sc;
            }
        }

        // raise waterline while no outflow to lake
        while (lowest_shore_cell->state.waterline() > waterline and no_sink)
        {
            // raise watercolumn to new waterline
            waterline = lowest_shore_cell->state.waterline();
            for (auto& lc : lake) {
                lc->state.watercolumn = waterline - lc->state.rock;
            }

            // update lake and shore
		    lake = update_lakesites(lake, shore);

            for (const auto& lc : lake) {
                if (lc->state.is_outflow) {
                    no_sink=false;
                    break;
                }
            }

            // update lowest shore cell
            lowest_shore_cell = shore[0];
            for (const auto& sc : shore) {
                if (sc->state.waterline() <
                    lowest_shore_cell->state.waterline())
                {
                    lowest_shore_cell = sc;
                }
            }
        }

        auto outflow_cell = lowest_shore_cell;
        if (not no_sink) {
            for (const auto& lc : lake) {
                if (lc->state.is_outflow) {
                    outflow_cell = lc;
                    break;
                }
            }
        }
        else {
            auto nbs = this->_cm.neighbors_of(lowest_shore_cell);
            auto it_nb = nbs.begin();
            while(it_nb != nbs.end()) {
                if (not check_eq((*it_nb)->state.waterline(), waterline)) {
                    it_nb = nbs.erase(it_nb);
                }
                else if (lake.end() == std::find(lake.begin(), lake.end(),
                                                 *it_nb))
                {
                    it_nb = nbs.erase(it_nb);
                }
                else { ++it_nb; }
            }

            if (nbs.size() > 1) {
                std::uniform_int_distribution<> dist(0, nbs.size() - 1);
                outflow_cell = nbs[dist(*(this->_rng))];
            }
            else { outflow_cell = nbs[0]; }
        }

        for (const auto& lc : lake) {
            if (lc->state.is_outflow) {
                _lowest_neighbors[lc->id()] = lc;
            }
            else {
                _lowest_neighbors[lc->id()] = outflow_cell;
            }
        }
        if (not outflow_cell->state.is_outflow) {
            _lowest_neighbors[outflow_cell->id()] = lowest_shore_cell;
        }

        return cell->state;
    };

    /// Make a drainage process from this cell
    /** Pass the cells drainage area through the already drained downstream
     *  cells up to an outflow or not already drained cell.
     *
     *  \note Must be updated in an asynchronous way. No shuffle needed.
     */
    RuleFunc pass_drainage_area = [this](auto& cell) {
        auto state = cell->state;
        state.was_drained = true;

        if (not state.is_outflow and _lowest_neighbors[cell->id()] == cell) {
            throw std::runtime_error("No recipient assigned to a cell!");
        }
        // nothing to do here
        if (state.is_outflow) {
            return state;
        }

        // get downstream neighbor
        auto downstream_cell = _lowest_neighbors[cell->id()];
        downstream_cell->state.drainage_area += state.drainage_area;

        // pass drainage to all already drained cells
        while(not downstream_cell->state.is_outflow and
              downstream_cell->state.was_drained)
        {
            downstream_cell = _lowest_neighbors[downstream_cell->id()];
            downstream_cell->state.drainage_area += state.drainage_area;
            if (downstream_cell->state.drainage_area > size(this->_cm.cells()))
            {
                throw std::runtime_error("Drainage network has loop!");
            }
        }

        return state;
    };
};


} // namespace Geomorphology
} // namespace Models
} // namespace Utopia

#endif // GEOMORPHOLOGY_HH
