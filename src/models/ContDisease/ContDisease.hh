#ifndef UTOPIA_MODELS_CONTDISEASE_HH
#define UTOPIA_MODELS_CONTDISEASE_HH

#include <functional>
#include <random>

// Utopia-related includes
#include <utopia/core/model.hh>
#include <utopia/core/apply.hh>
#include <utopia/core/cell_manager.hh>

// ContDisease-realted includes
#include "params.hh"
#include "state.hh"


namespace Utopia::Models::ContDisease {

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Specialize the CellTraits type helper for this model
/** \detail Specifies the type of each cells' state as first template argument
  *         and the update mode as second.
  *
  * See \ref Utopia::CellTraits for more information.
  */
using CDCellTraits = Utopia::CellTraits<State, Update::manual>;


/// Typehelper to define data types of ContDisease model
using CDTypes = ModelTypes<>;


// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Contagious disease model on a grid
/** \detail In this model, we model the spread of a disease through a forest on
 *          a 2D grid. Each cell can have one of five different states: empty,
 *          tree, infected, source or empty.
 *          Each time step, cells update their state according to the update
 *          rules. Empty cells will convert with a certain probability
 *          to tress, while trees represent cells that can be infected.
 *          Infection can happen either through a neighboring cells, or
 *          through random point infection. An infected cells reverts back to
 *          empty after one time step.
 *          Stones represent cells that can not be infected, therefore
 *          represent a blockade for the spread of the infection.
 *          Infection sources are cells that continuously spread infection
 *          without dying themselves.
 *          Different starting conditions, and update mechanisms can be
 *          configured.
 */
class ContDisease:
    public Model<ContDisease, CDTypes>
{
public:
    /// The base model type
    using Base = Model<ContDisease, CDTypes>;

    /// Data type for a data group
    using DataGroup = typename Base::DataGroup;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Type of the CellManager to use
    using CellManager = Utopia::CellManager<CDCellTraits, ContDisease>;

    /// Rule function type
    using RuleFunc = typename CellManager::RuleFunc;

private:
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor, _space
    // ... but you should definitely check out the documentation ;)

    // -- Members -------------------------------------------------------------
    /// The cell manager
    CellManager _cm;

    /// Model parameters
    const Params _params;

    /// The range [0, 1] distribution to use for probability checks
    std::uniform_real_distribution<double> _prob_distr;

    /// The incremental cluster tag
    unsigned int _cluster_id_cnt;

      // .. Temporary objects .................................................
    /// Densities for all states
    /** \note   This array is used for temporary storage; it is not
      *         automatically updated!
      * \detail The array entries map to the CellState enum:
      *         0: empty
      *         1: tree
      *         2: infected
      *         3: source
      *         4: stone
      */
    std::array<double, 5> _densities;

    /// A temporary container for use in cluster identification
    std::vector<std::shared_ptr<CellManager::Cell>> _cluster_members;

    // .. Data groups .........................................................
    /// The data group where all density datasets are stored in
    std::shared_ptr<DataGroup> _dgrp_densities;

    // .. Datasets ............................................................
    /// 2D dataset (cell ID and time) of cell states
    std::shared_ptr<DataSet> _dset_state;

    /// 2D dataset (tree age and time) of cells
    std::shared_ptr<DataSet> _dset_age;

    /// 1D dataset of density of empty cells over time
    std::shared_ptr<DataSet> _dset_density_empty;

    /// 1D dataset of density of tree cells over time
    std::shared_ptr<DataSet> _dset_density_tree;

    /// 1D dataset of density of infected cells over time
    std::shared_ptr<DataSet> _dset_density_infected;

    /// 1D dataset of density of infected infection source cells over time
    std::shared_ptr<DataSet> _dset_density_source;

    /// 1D dataset of density of infected stone cells over time
    std::shared_ptr<DataSet> _dset_density_stone;

    /// The dataset for storing the cluster ID associated with each cell
    std::shared_ptr<DataSet> _dset_cluster_id;



public:
    /// Construct the ContDisease model
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     */
    template<class ParentModel>
    ContDisease (const std::string name, ParentModel &parent)
    :
        // Initialize first via base model
        Base(name, parent),

        // Initialize the cell manager, binding it to this model
        _cm(*this),

        // Carry over Parameters
        _params(this->_cfg),

        // Initialize remaining members
        _prob_distr(0., 1.),
        _densities{},  // undefined here, will be set in constructor body
        _cluster_members(),
        _cluster_id_cnt(),

        // Create a data group for the densities
        _dgrp_densities(this->_hdfgrp->open_group("densities")),

        // Create dataset for cell states
        _dset_state(this->create_cm_dset("state", _cm)),

        // Create dataset for tree age
        _dset_age(this->create_cm_dset("age", _cm)),

        // Create datasets for all densities
        _dset_density_empty(   this->create_dset("empty",
                                                 _dgrp_densities, {})),
        _dset_density_tree(    this->create_dset("tree",
                                                 _dgrp_densities, {})),
        _dset_density_infected(this->create_dset("infected",
                                                 _dgrp_densities, {})),
        _dset_density_source(  this->create_dset("source",
                                                 _dgrp_densities, {})),
        _dset_density_stone(   this->create_dset("stone",
                                                 _dgrp_densities, {})),

        // Create dataset for cluster id
        _dset_cluster_id(this->create_cm_dset("cluster_id", _cm))
    {
        // Make sure the densities are not undefined
        _densities.fill(std::numeric_limits<double>::quiet_NaN());

        // Remaining initialization steps regard macroscopic quantities

        // Stones
        if (_params.stones.on) {
            if (_params.stones.init.mode == "random") {
                this->_log->debug("Setting up random stones ...");

                /// Initialize stones randomly 
                RuleFunc _stone_init = [this](const auto& cell) {
                    // Cell will be a stone with probability p_random
                    auto state = cell->state;
                    if (this->_prob_distr(*this->_rng) 
                            < this->_params.stones.init.p_random)
                    {
                        state.kind = Kind::stone;
                        return state;
                    }
                    // else: stay in the same state
                    return state;
                };
                
                // Set random stones
                apply_rule<Update::async, Shuffle::on>
                    (_stone_init, _cm.cells(), *this->_rng);
            }
            
            // If cluster mode is selected, additionally add clustered stones
            if (_params.stones.init.mode == "cluster"){
                this->_log->debug("Setting up stone clusters ...");

                // Add a stone with probability stone_cluster to any empty
                // cell with a neighboring stone.

                RuleFunc _stone_cluster = [this](const auto& cell) {
                    auto state = cell->state;

                    // Add the clustered stones
                    // Iterate over all neighbors of the current cell
                    for (auto& nb: this->_cm.neighbors_of(cell)) {
                        auto nb_state = nb->state;

                        if (    state.kind == Kind::empty
                            and nb_state.kind == Kind::stone
                            and this->_prob_distr(*this->_rng) 
                                < this->_params.stones.init.p_cluster)
                        {
                            // Become a stone
                            state.kind = Kind::stone;
                        }
                        else {
                            break;
                        }
                    }
                    return state;
                };

                // Create stone clusters
                apply_rule<Update::async, Shuffle::on>
                    (_stone_cluster, _cm.cells(), *this->_rng);
            }
        } // end of stones setup


        // Infection source
        if (_params.infection_source) {
            this->_log->debug("Setting bottom boundary cells to be "
                              "permanently infected ...");

            RuleFunc _source_init = [this](const auto& cell) {
                auto state = cell->state;
                state.kind = Kind::source;
                return state;
            };

            apply_rule<Update::sync>(_source_init, _cm.boundary_cells("bottom"));
        }

        // Initialization should be finished here.
        this->_log->debug("{} model fully set up.", this->_name);

        // -- Write initial state
        // Write all other data that is written each write_data call, which
        // includes the remaining densities (indices 0, 1, and 2)
        this->write_data();

        // Now that all densities have been calculated (in write_data), write
        // those that do not change throughout the simulation (indices 3 and 4)
        _dset_density_stone->write(
            _densities[static_cast<unsigned short>(Kind::stone)]);
        _dset_density_source->write(
            _densities[static_cast<unsigned short>(Kind::source)]);

        this->_log->debug("Initial state written.");
    }

protected:
    // .. Helper functions ....................................................
    /// Update the densities array
    /** @details  Each density is calculated by counting the number of state
     *            occurrences and afterwards dividing by the total number of
     *            cells.
     * @attention It is possible that rounding errors occur due to the
     *            division, thus, it is not guaranteed that the densities
     *            exactly add up to 1. The errors should be negligible.
     */
    void update_densities() {
        // Temporarily overwrite every entry in the densities with zeroes
        _densities.fill(0);

        // Count the occurrence of each possible state. Use the _densities
        // member for that in order to not create a new array.
        for (const auto& cell : this->_cm.cells()) {
            // Cast enum to integer to arrive at the corresponding index
            ++_densities[static_cast<unsigned short int>(cell->state.kind)];
        }
        // The _densities array now contains the counts.

        // Calculate the actual densities by dividing the counts by the total
        // number of cells.
        for (auto&& d : _densities){
            d /= static_cast<double>(this->_cm.cells().size());
        }
    };


    /// Identify clusters
    /** \details This function identifies clusters and updates the cell
     *           specific cluster_id as well as the member variable 
     *           cluster_id_cnt that counts the number of ids
     */
    void identify_clusters(){
        // reset cluster counter
        _cluster_id_cnt = 0;
        apply_rule<Update::async, Shuffle::off>(_identify_cluster, 
                                                _cm.cells(), 
                                                *this->_rng);
    }

    // .. Rule functions ......................................................

    /// Define the update rule
    /** \details Update the given cell according to the following rules:
      *          - Empty cells grow trees with probability p_growth.
      *          - Tree cells in neighborhood of an infected cell get infected
      *            with the probability p_infect.
      *          - Infected cells die and become an empty cell.
      */
    RuleFunc _update = [this](const auto& cell){
        // Get the current state of the cell
        auto state = cell->state;
        state.cluster_id = 0;

        // Distinguish by current state
        if (state.kind == Kind::empty) {
            // With a probability of p_growth, set the cell's state to tree
            if (_prob_distr(*this->_rng) < _params.p_growth){
                state.kind = Kind::tree;
                return state;
            }
        }
        else if (state.kind == Kind::tree){
            // Increase the age of the tree
            ++state.age;

            // Tree can be infected by neighbor or by random-point-infection.

            // Determine whether there will be a point infection
            if (_prob_distr(*this->_rng) < _params.p_random_infect) {
                // Yes, point infection occurred.
                state.kind = Kind::infected;
                return state;
            }
            else {
                // Go through neighbor cells (according to Neighborhood type),
                // and check if they are infected (or an infection source).
                // If yes, infect cell with the probability p_infect.
                for (const auto& nb: this->_cm.neighbors_of(cell)) {
                    // Get the neighbor cell's state
                    auto nb_state = nb->state;

                    if (   nb_state.kind == Kind::infected
                        or nb_state.kind == Kind::source)
                    {
                        // With a certain probability, become infected
                        if (_prob_distr(*this->_rng) < _params.p_infect) {
                            state.kind = Kind::infected;
                            return state;
                        }
                    }
                }
            }
        }
        else if (state.kind == Kind::infected) {
            // Decease -> become an empty cell
            state.kind = Kind::empty;

            // Reset the age of the cell to 0
            state.age = 0;

            return state;
        }
        // else: other cell states need no update

        // Return the (potentially changed) cell state for the next round
        return state;
    };

    /// Identify each cluster of trees
    RuleFunc _identify_cluster = [this](const auto& cell){
        if (cell->state.cluster_id != 0 or cell->state.kind != Kind::tree) {
            // already labelled, nothing to do. Return current state
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
            for (const auto& nb : this->_cm.neighbors_of(cluster[i])) {
                // If it is a tree that is not yet in the cluster, add it.
                if (    nb->state.cluster_id == 0
                    and nb->state.kind == Kind::tree)
                {
                    nb->state.cluster_id = _cluster_id_cnt;
                    cluster.push_back(nb);
                    // This extends the outer for-loop...
                }
            }
        }

        return cell->state;
    };


public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Iterate a single time step
    /** \detail This updates all cells (synchronously) according to the
      *         _update rule. For specifics, see there.
      */
    void perform_step () {
        // Apply the update rule to all cells.
        apply_rule<Update::sync>(_update, _cm.cells());
        // NOTE The cell state is updated synchronously, i.e.: only after all
        //      cells have been visited and know their state for the next step
    }


    /// Monitor model information
    /** \detail Supplies the `densities` array to the monitor.
      */
    void monitor () {
        update_densities();
        this->_monitor.set_entry("densities", _densities);
    }


    /// Write data
    /** \detail Writes out the cell state and the densities of cells with the
      *         states empty, tree, or infected (i.e.: those that may change)
      */
    void write_data () {
        // Write the cell state
        _dset_state->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return static_cast<unsigned short int>(cell->state.kind);
            }
        );

        // Write the tree ages
        _dset_age->write(_cm.cells().begin(), _cm.cells().end(),
            [](const auto& cell) {
                return static_cast<unsigned short int>(cell->state.age);
            }
        );

        // And those densities that are changing (empty, tree, infected)
        update_densities();

        // Clusters are only identified for the last time step
        if (this->get_time_max() == this->get_time()) {
            // Identify clusters
            identify_clusters();

            _dset_cluster_id->write(_cm.cells().begin(), _cm.cells().end(),
               [](const auto& cell) {
                   return cell->state.cluster_id;
            });
        }

        // Write the densities
        _dset_density_empty->write(
            _densities[static_cast<unsigned short>(Kind::empty)]);
        _dset_density_tree->write(
            _densities[static_cast<unsigned short>(Kind::tree)]);
        _dset_density_infected->write(
            _densities[static_cast<unsigned short>(Kind::infected)]);
    }
};


} // namespace Utopia::Models::ContDisease

#endif // UTOPIA_MODELS_CONTDISEASE_HH
