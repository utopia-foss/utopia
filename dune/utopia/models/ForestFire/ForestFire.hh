#ifndef UTOPIA_MODELS_FORESTFIRE_HH
#define UTOPIA_MODELS_FORESTFIRE_HH

#include <functional>
#include <random>

#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/apply.hh>
#include <dune/utopia/core/cell_manager.hh>


namespace Utopia {
namespace Models {
namespace ForestFire {

/// The values a cell's state can take: empty and tree
enum FFMCellState { empty=0, tree=1 };


/// The full cell struct for the ForestFire model
struct FFMCell {
    /// The actual cell state
    FFMCellState state;

    /// An ID denoting to which cluster this cell belongs
    unsigned int cluster_tag;

    /// Whether the cell is permanently ignited
    bool permanently_ignited;

    /// Construct a cell from a configuration node and an RNG
    template<class RNG>
    FFMCell (const DataIO::Config& cfg, const std::shared_ptr<RNG>& rng)
    :
        state(empty),
        cluster_tag(0),
        permanently_ignited(false)
    {
        // Check if initial_density is available to set up cell state
        if (cfg["initial_density"]) {
            const auto rho = get_as<double>("initial_density", cfg);

            if (rho < 0. or rho > 1.) {
                throw std::invalid_argument("initial_density needs to be in "
                    "interval [0., 1.], but was not!");
            }

            // With this probability, the cell state is a tree
            if (std::uniform_real_distribution<double>(0., 1.)(*rng) < rho) {
                state = tree;
            }
            // NOTE Although the distribution object is created each time, this
            //      is not a significant slow-down compared to re-using an
            //      existing distribution object (<4%). When compiled with
            //      optimization flags, that slowdown is even smaller...
        }
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
using FFMCellTraits = Utopia::CellTraits<FFMCell, UpdateMode::async>;


/// ForestFire model parameter struct
struct Param {
    /// Rate of growth per cell
    const double growth_rate;

    /// Frequency of lightning occurring per cell
    const double lightning_frequency;

    /// Whether the bottom row should be constantly on fire
    const bool light_bottom_row;

    /// The resistance parameter
    const double resistance;

    /// Construct the parameters from the given configuration node
    Param(const DataIO::Config& cfg)
    :
        growth_rate(get_as<double>("growth_rate", cfg)),
        lightning_frequency(get_as<double>("lightning_frequency", cfg)),
        light_bottom_row(get_as<bool>("light_bottom_row", cfg)),
        resistance(get_as<double>("resistance", cfg))
    {
        if ((growth_rate > 1) or (growth_rate < 0)) {
            throw std::invalid_argument("Invalid growth_rate; need be a value "
                "in range [0, 1] and specify the probability per time step "
                "and cell with which an empty cell turns into a tree.");
        }
        if ((lightning_frequency > 1) or (lightning_frequency < 0)) {
            throw std::invalid_argument("Invalid lightning_frequency; need be "
                "in range [0, 1] and specify the probability per cell and "
                "time step for lightning to strike.");
        }
        if ((resistance > 1) or (resistance < 0)) {
            throw std::invalid_argument("Invalid resistance argument! "
                "Need be a value in range [0, 1] and specify the probability "
                "per neighbor with which that neighbor can resist fire");
        }
    }
};



/// Typehelper to define data types of ForestFire model
using FFMTypes = ModelTypes<>;


/// The ForestFire model
/** The ForestFire model simulates the development of a forest under influence
 *  of forest fires. 
 *  Trees grow randomly and fires lead to a whole cluster instantaneously
 *  burning down; thus being a so-called two state model.
 */
class ForestFire:
    public Model<ForestFire, FFMTypes>
{
public:
    /// The base model type
    using Base = Model<ForestFire, FFMTypes>;
    
    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// The type of the cell manager
    using FFMCellManager = CellManager<FFMCellTraits, ForestFire>;

    /// Rule function type, extracted from CellManager
    using RuleFunc = typename FFMCellManager::RuleFunc;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor

    // -- Members -------------------------------------------------------------
    /// The cell manager for the forest fire model
    FFMCellManager _cm;

    /// Model parameters
    const Param _param;

    /// A [0,1]-range uniform distribution used for evaluating probabilities
    std::uniform_real_distribution<double> _prob_distr;

    /// The incremental cluster tag
    unsigned int _cluster_tag_cnt;

    /// A temporary container for use in cluster identification
    std::vector<std::shared_ptr<FFMCellManager::Cell>> _cluster_members;

    // .. Datasets ............................................................
    /// The dataset for storing state values for each cell
    std::shared_ptr<DataSet> _dset_state;

    /// The dataset for storing the cluster ID associated with each cell
    std::shared_ptr<DataSet> _dset_cluster_id;


public:
    // -- Model Setup ---------------------------------------------------------
    /// Construct the ForestFire model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     */
    template<class ParentModel>
    ForestFire (const std::string name, ParentModel &parent)
    :
        // Initialize first via base model
        Base(name, parent),

        // Initialize the cell manager, binding it to this model
        _cm(*this),

        // Carry over parameters
        _param(this->_cfg),

        // Initialize remaining members
        _prob_distr(0., 1.),
        _cluster_tag_cnt(0),
        _cluster_members(),

        // Create datasets using the helper functions for CellManager-data
        _dset_state(this->create_cm_dset("state", _cm)),
        _dset_cluster_id(this->create_cm_dset("cluster_id", _cm))
    {
        // Cells are already set up in the CellManager
        // Still need to take care of the ignited bottom row
        if (_param.light_bottom_row) {
            this->_log->debug("Setting bottom boundary cells to be "
                              "permanently ignited ...");

            if (this->_space.periodic) {
                this->_log->warn("The parameter 'light_bottom_row' has no "
                    "effect with the space configured to be periodic!");
            }

            apply_rule(
                // The rule to apply
                [](const auto& cell){
                    // Get the state, change it, return
                    auto state = cell->state();
                    state.permanently_ignited = true;
                    return state;
                },
                // The containers over which to iterate
                _cm.boundary_cells("bottom"),
                // The RNG needed for apply_rule calls with async update
                *this->_rng
            );
        }
        this->_log->debug("Cells fully set up.");

        // Write initial state
        this->write_data();

        this->_log->debug("{} model all set up and initial state written.",
                          this->_name);
    }


private:
    // .. Setup functions .....................................................


    // .. Helper functions ....................................................


    // .. Rule functions ......................................................

    /// Update rule, called every step
    /** \detail The possible transitions are the following:
      *           - empty -> tree (p = growth_rate)
      *           - tree -> burning (p = lightning_frequency)
      *         A burning tree directly invokes the burning of the whole
      *         cluster of connected trees ("two-state FFM"). After that, all
      *         burned cells are in the empty state again.
      *
      * \note   This rule relies on an asynchronous cell update.
      */
    RuleFunc _update = [this](const auto& cell){
        // Get the current state of the cell and reset the cluster tag
        auto state = cell->state();
        state.cluster_tag = 0;
        
        // Permanently ignited cells always burn the cluster
        if (state.permanently_ignited) {
            state = _burn_cluster(cell);
        }

        // Empty cells can grow a tree
        else if (    state.state == empty
                 and this->_prob_distr(*this->_rng) < _param.growth_rate)
        {
            state.state = tree;
        }
        
        // Trees can be hit by lightning
        else if (this->_prob_distr(*this->_rng) < _param.lightning_frequency)
        {
            state = _burn_cluster(cell);
        }

        return state;
    };

    /// Rule to burn a cluster of trees around the given cell
    /** \note This function is never actually called via apply_rule, but only
      *       from the update method. It relies on an asynchronous cell update.
      */
    RuleFunc _burn_cluster = [this](const auto& cell) {
        // The current cell surely is empty now.
        cell->state().state = empty;
        
        // Use existing cluster member container, clear it, add current cell
        auto& cluster = _cluster_members;
        cluster.clear();
        cluster.push_back(cell);

        // Recursively go over all cluster members
        for (unsigned int i = 0; i < cluster.size(); ++i) {
            const auto& cluster_member = cluster[i];

            // Iterate over all potential cluster members
            for (const auto& c : this->_cm.neighbors_of(cluster_member)) {
                // If it is a tree, it will burn ...
                if (c->state().state == tree) {
                    // ... unless there is resistance > 0 ...
                    if (this->_param.resistance > 0.) {
                        // ... where there is a chance not to burn:
                        if (this->_prob_distr(*this->_rng) > _param.resistance)
                            continue;
                    }

                    // Bad luck. Burn.
                    c->state().state = empty;
                    cluster.push_back(c);
                    // This extends the outer for-loop
                }
            }
        }

        // Return the current cell's adjusted state.
        return cell->state();
    };

    /// Get the identity of each cluster of trees
    /* \detail Runs a percolation on a cell, that has ID 0. Then, give all
     *         cells of that percolation the same ID.
     *         The _cluster_tag_cnt member keeps track of already given IDs.
     */
    RuleFunc _identify_cluster = [this](const auto& cell){
        if (cell->state().cluster_tag != 0 or cell->state().state == empty) {
            // already labelled, nothing to do. Return current state
            return cell->state();
        }
        // else: need to label this cell

        // Increment the cluster ID counter and label the given cell
        _cluster_tag_cnt++;
        cell->state().cluster_tag = _cluster_tag_cnt;

        // Use existing cluster member container, clear it, add current cell
        auto& cluster = _cluster_members;
        cluster.clear();
        cluster.push_back(cell);

        // Perform the percolation
        for (unsigned int i = 0; i < cluster.size(); ++i) {
            // Iterate over all potential cluster members c, i.e. all
            // neighbors of cell cluster[i] that is already in the cluster
            for (const auto& c : this->_cm.neighbors_of(cluster[i])) {
                // If it is a tree that is not yet in the cluster, add it.
                if (    c->state().cluster_tag == 0
                    and c->state().state == tree)
                {
                    c->state().cluster_tag = _cluster_tag_cnt;
                    cluster.push_back(c);
                    // This extends the outer for-loop...
                }
            }
        }

        return cell->state();
    };


public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Perform step
    void perform_step () {
        /// apply update rule on all cells, asynchronously and shuffled
        apply_rule(_update, _cm.cells(), *this->_rng);
    }

    /// Provide monitoring data: tree density and number of clusters
    void monitor () {
        this->_monitor.set_entry("tree_density", calc_tree_density());
        this->_monitor.set_entry("num_clusters", identify_clusters());
    }

    /// Write data
    void write_data () {
        // Store all cells' state
        _dset_state->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return static_cast<unsigned short int>(cell->state().state);
        });

        // Identify the clusters (only needed when actually writing)
        identify_clusters();

        _dset_cluster_id->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return cell->state().cluster_tag;
        });
    }


    // Getters and setters ....................................................
    // Add getters and setters here to interface with other model

    /// Calculate the density of tree cells
    double calc_tree_density() const {
        double sum = 0.;

        for (const auto& cell : _cm.cells()) {
            if (cell->state().state == tree) {
                sum += 1.;
            }
        }
        return sum / _cm.cells().size();
    }

    /// Identifies clusters in the cells and labels them with corresponding IDs
    /** \return Number of clusters identified
     */
    unsigned int identify_clusters() {
        this->_log->debug("Identifying clusters...");

        _cluster_tag_cnt = 0; // reset tmp counter for cluster IDs
        apply_rule(_identify_cluster, _cm.cells(), *this->_rng);

        this->_log->debug("Identified {} clusters.", _cluster_tag_cnt);

        return _cluster_tag_cnt;
    }

};

} // namespace ForestFire
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_FORESTFIRE_HH
