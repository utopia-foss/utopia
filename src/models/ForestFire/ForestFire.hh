#ifndef UTOPIA_MODELS_FORESTFIRE_HH
#define UTOPIA_MODELS_FORESTFIRE_HH

#include <functional>
#include <random>
#include <numeric>

#include <utopia/core/model.hh>
#include <utopia/core/apply.hh>
#include <utopia/core/cell_manager.hh>


namespace Utopia {
namespace Models {
namespace ForestFire {

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The values a cell's state can take: empty and tree
enum class Kind { empty=0, tree=1 };


/// The full cell struct for the ForestFire model
struct State {
    /// The actual cell state
    Kind kind;

    /// An ID denoting to which cluster this cell belongs
    unsigned int cluster_id;

    /// Whether the cell is permanently ignited
    bool permanently_ignited;

    /// Remove default constructor, for safety
    State () = delete;

    /// Construct a cell from a configuration node and an RNG
    template<class RNG>
    State (const DataIO::Config& cfg, const std::shared_ptr<RNG>& rng)
    :
        kind(Kind::empty),
        cluster_id(0),
        permanently_ignited(false)
    {
        // Get the desired probability to be a tree
        const auto p_tree = get_as<double>("p_tree", cfg);

        if (p_tree < 0. or p_tree > 1.) {
            throw std::invalid_argument("p_tree needs to be in interval "
                                        "[0., 1.], but was not!");
        }
        else if (p_tree == 0.) {
            // No need to draw a random number
            return;
        }

        // With this probability, the cell state is a tree
        if (std::uniform_real_distribution<double>(0., 1.)(*rng) < p_tree) {
            kind = Kind::tree;
        }
        // NOTE Although the distribution object is created each time, this
        //      is not a significant slow-down compared to re-using an
        //      existing distribution object (<4%). When compiled with
        //      optimization flags, that slowdown is even smaller...
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
using CellTraits = Utopia::CellTraits<State, Update::manual>;


/// ForestFire model parameter struct
struct Param {
    /// Rate of growth per cell
    const double p_growth;

    /// Frequency of lightning occurring per cell
    const double p_lightning;

    /// Whether the bottom row should be constantly on fire
    const bool light_bottom_row;

    /// The resistance probability
    const double p_resistance;

    /// Construct the parameters from the given configuration node
    Param(const DataIO::Config& cfg)
    :
        p_growth(get_as<double>("p_growth", cfg)),
        p_lightning(get_as<double>("p_lightning", cfg)),
        light_bottom_row(get_as<bool>("light_bottom_row", cfg)),
        p_resistance(get_as<double>("p_resistance", cfg))
    {
        if ((p_growth > 1) or (p_growth < 0)) {
            throw std::invalid_argument("Invalid p_growth; need be a value "
                "in range [0, 1] and specify the probability per time step "
                "and cell with which an empty cell turns into a tree.");
        }
        if ((p_lightning > 1) or (p_lightning < 0)) {
            throw std::invalid_argument("Invalid p_lightning; need be "
                "in range [0, 1] and specify the probability per cell and "
                "time step for lightning to strike.");
        }
        if ((p_resistance > 1) or (p_resistance < 0)) {
            throw std::invalid_argument("Invalid resistance argument! "
                "Need be a value in range [0, 1] and specify the probability "
                "per neighbor with which that neighbor can resist fire");
        }
    }
};


/// Typehelper to define data types of ForestFire model
using ModelTypes = Utopia::ModelTypes<>;


// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The ForestFire model
/** The ForestFire model simulates the development of a forest under influence
 *  of forest fires. 
 *  Trees grow randomly and fires lead to a whole cluster instantaneously
 *  burning down; thus being a so-called two state model.
 */
class ForestFire:
    public Model<ForestFire, ModelTypes>
{
public:
    /// The base model type
    using Base = Model<ForestFire, ModelTypes>;
    
    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// The type of the cell manager
    using CellManager = Utopia::CellManager<CellTraits, ForestFire>;

    /// The type of a cell
    using Cell = typename CellManager::Cell;

    /// Rule function type, extracted from CellManager
    using RuleFunc = typename CellManager::RuleFunc;


private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor, _log, _space
    // ... but you should definitely check out the documentation ;)

    // -- Members -------------------------------------------------------------
    /// The cell manager for the forest fire model
    CellManager _cm;

    /// Model parameters
    const Param _param;

    /// A [0,1]-range uniform distribution used for evaluating probabilities
    std::uniform_real_distribution<double> _prob_distr;

    /// The incremental cluster tag
    unsigned int _cluster_id_cnt;

    /// A temporary container for use in cluster identification
    std::vector<std::shared_ptr<CellManager::Cell>> _cluster_members;

    // .. Datasets ...........................................................
    /// The dataset that stores the kind for each cell, e.g. Kind::tree
    const std::shared_ptr<DataSet> _dset_kind;

    /// The dataset that stores the cluster id
    const std::shared_ptr<DataSet> _dset_cluster_id;

    /// The dataset that stores the mean density
    const std::shared_ptr<DataSet> _dset_tree_density;


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
        _cluster_id_cnt(0),
        _cluster_members(),

        // Create datasets using the helper functions for CellManager-data
        _dset_kind{this->create_cm_dset("kind", _cm)},
        _dset_cluster_id{this->create_cm_dset("cluster_id", _cm)},
        _dset_tree_density{this->create_dset("tree_density", {})}
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

            apply_rule<Update::async, Shuffle::off>(
                // The rule to apply
                [](const auto& cell){
                    auto& state = cell->state;
                    state.permanently_ignited = true;
                    return state;
                },
                // The container over which to iterate
                _cm.boundary_cells("bottom"),
                // The RNG (not used, but needs to be passed)
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
    /// Calculate and return the density of tree cells
    double calculate_tree_density() const {
        // NOTE If execution policies are implemented, this could be easily
        //      made parallel by replacing std::accumulate with std::reduce 
        //      and adding std::execution::par from the header
        //      <execution> as first argument.
        return
            std::accumulate(_cm.cells().begin(), _cm.cells().end(),
                0,
                [&](std::size_t sum, const std::shared_ptr<Cell>& cell) {
                    return sum + (cell->state.kind == Kind::tree);
                }
            )
            / static_cast<double>(_cm.cells().size());
    }

    /// Identifies clusters in the cells and labels them with corresponding IDs
    /** \details This function updates the cluster id of each cell
     * 
     * \return Number of clusters identified
     */
    unsigned int identify_clusters() {
        this->_log->debug("Identifying clusters...");

        // reset tmp counter for cluster IDs
        _cluster_id_cnt = 0; 
        
        // Identify clusters
        apply_rule<Update::async, Shuffle::off>(
            _identify_cluster, _cm.cells(), *this->_rng
        );

        this->_log->debug("Identified {} clusters.", _cluster_id_cnt);

        return _cluster_id_cnt;
    }

    // .. Rule functions ......................................................
    /// Update rule, called every step
    /** \detail The possible transitions are the following:
      *           - empty -> tree (with p_growth)
      *           - tree -> burning (with p_lightning)
      *         A burning tree directly invokes the burning of the whole
      *         cluster of connected trees ("two-state FFM"). After that, all
      *         burned cells are in the empty state again.
      *
      * \note   This rule relies on an asynchronous cell update.
      */
    RuleFunc _update = [this](const auto& cell){
        // Get the current state of the cell and reset the cluster tag
        auto& state = cell->state;
        state.cluster_id = 0;
        
        // Permanently ignited cells always burn the cluster
        if (state.permanently_ignited) {
            state = _burn_cluster(cell);
        }

        // Empty cells can grow a tree
        else if (    state.kind == Kind::empty
                 and this->_prob_distr(*this->_rng) < _param.p_growth)
        {
            state.kind = Kind::tree;
        }
        
        // Trees can be hit by lightning
        else if (this->_prob_distr(*this->_rng) < _param.p_lightning)
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
        cell->state.kind = Kind::empty;
        
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
                if (c->state.kind == Kind::tree) {
                    // ... unless there is p_resistance > 0 ...
                    if (this->_param.p_resistance > 0.) {
                        // ... where there is a chance not to burn:
                        if (  this->_prob_distr(*this->_rng)
                            > _param.p_resistance) {
                            continue;
                        }
                    }

                    // Bad luck. Burn.
                    c->state.kind = Kind::empty;
                    cluster.push_back(c);
                    // This extends the outer for-loop
                }
            }
        }

        // Return the current cell's adjusted state.
        return cell->state;
    };

    /// Get the identity of each cluster of trees
    /* \detail Runs a percolation on a cell, that has ID 0. Then, give all
     *         cells of that percolation the same ID.
     *         The _cluster_id_cnt member keeps track of already given IDs.
     */
    RuleFunc _identify_cluster = [this](const auto& cell){
        if (   cell->state.cluster_id != 0
            or cell->state.kind == Kind::empty)
        {
            // already labelled, nothing to do. Return unchanged state
            return cell->state;
        }
        // else: need to label this cell

        // Increment the cluster ID counter and label the given cell
        _cluster_id_cnt++;
        cell->state.cluster_id = _cluster_id_cnt;

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
                if (    c->state.cluster_id == 0
                    and c->state.kind == Kind::tree)
                {
                    c->state.cluster_id = _cluster_id_cnt;
                    cluster.push_back(c);
                    // This extends the outer for-loop...
                }
            }
        }

        return cell->state;
    };


public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Perform step
    void perform_step () {
        // Apply update rule on all cells, asynchronously and shuffled
        apply_rule<Update::async, Shuffle::on>(
            _update, _cm.cells(), *this->_rng
        );
    }

    /// Provide monitoring data: tree density and number of clusters
    /** \details The monitored data relies on tracking data variables
     *           that need not correspond exactly to the actual value at this 
     *           time. They are calculated before the writing them out.
     */ 
    void monitor () {
        this->_monitor.set_entry("tree_density", calculate_tree_density());
    }

    /// Write data
    void write_data () {
        // Store all cells' state
        _dset_kind->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return static_cast<char>(cell->state.kind);
        });

        // Identify the clusters (only needed when actually writing)
        identify_clusters();
        _dset_cluster_id->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return cell->state.cluster_id;
        });

        // Calculate and write the tree density
        _dset_tree_density->write(calculate_tree_density()); 
    }
};

} // namespace ForestFire
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_FORESTFIRE_HH
