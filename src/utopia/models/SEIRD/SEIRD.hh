#ifndef UTOPIA_MODELS_SEIRD_HH
#define UTOPIA_MODELS_SEIRD_HH

#include <functional>
#include <random>

// Utopia-related includes
#include <utopia/core/model.hh>
#include <utopia/core/apply.hh>
#include <utopia/core/cell_manager.hh>
#include <utopia/core/select.hh>

// SEIRD-realted includes
#include "params.hh"
#include "state.hh"
#include "utils.hh"


namespace Utopia::Models::SEIRD
{
// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

// Specialize the CellTraits type helper for this model
/** Specifies the type of each cells' state as first template argument
 * and the update mode as second.
 *
 * See \ref Utopia::CellTraits for more information.
 */
using CDCellTraits = Utopia::CellTraits<State, Update::manual>;

/// Typehelper to define data types of SEIRD model
using CDTypes = ModelTypes<>;

// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// SEIRD model on a grid
/** In this model, we model the spread of a disease using a SEIRD
 * (susceptible-exposed-infected-recovered-deceased) model on a 2D grid.
 */
class SEIRD : public Model<SEIRD, CDTypes>
{
  public:
    /// The base model type
    using Base = Model<SEIRD, CDTypes>;

    /// Data type for a data group
    using DataGroup = typename Base::DataGroup;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Type of the CellManager to use
    using CellManager = Utopia::CellManager<CDCellTraits, SEIRD>;

    /// Type of a cell
    using Cell = typename CellManager::Cell;

    /// Type of a container of shared pointers to cells
    using CellContainer = Utopia::CellContainer<Cell>;

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

    /// A temporary container for use in cluster identification
    std::vector<std::shared_ptr<CellManager::Cell>> _cluster_members;

    /// Densities for all states
    /** Array indices are linked to \ref Utopia::Models::SEIRD::Kind
     *
     * \warning This array is used for temporary storage; it is not
     *          automatically updated but only upon write operations.
     */
    std::array<double, static_cast<char>(Kind::COUNT)> _densities;

    /// Counters for state transitions and other global counters
    /** Is reset at the beginning of each step.
      */
    Counters<unsigned int> _counts;


    // .. Data-Output related members .........................................
    /// The compression level used for all datasets
    const int _compression;

    /// If false, writes only the non-spatial `densities` and `counts` data
    bool _write_ca_data;

    /// 2D dataset (densities array and time) of density values
    std::shared_ptr<DataSet> _dset_densities;

    /// 2D dataset (counts array and time) of state counters
    std::shared_ptr<DataSet> _dset_counts;

    /// 2D dataset (cell ID and time) of cell kinds
    std::shared_ptr<DataSet> _dset_kind;

    /// 2D dataset (cell ID and time) of cell's immunity
    std::shared_ptr<DataSet> _dset_immune;

    /// 2D dataset (cell ID and time) of cell's number of recoveries
    std::shared_ptr<DataSet> _dset_num_recoveries;

    /// 2D dataset (cell ID and time) of cell's age
    std::shared_ptr<DataSet> _dset_age;

    /// The dataset for storing the cluster ID associated with each cell
    std::shared_ptr<DataSet> _dset_cluster_id;


  public:
    /// Construct the SEIRD model
    /** \param name             Name of this model instance; is used to extract
     *                          the configuration from the parent model and
     *                          set up a HDFGroup for this instance
     *  \param parent_model     The parent model this model instance resides in
     *  \param custom_cfg       A custom configuration to use instead of the
     *                          one extracted from the parent model using the
     *                          instance name
     */
    template<class ParentModel>
    SEIRD(const std::string& name,
          ParentModel& parent_model,
          const DataIO::Config& custom_cfg = {})
    :
        // Initialize first via base model
        Base(name, parent_model, custom_cfg),

        // Initialize the cell manager, binding it to this model
        _cm(*this),

        // Carry over Parameters
        _params(this->_cfg),

        // Initialize remaining members
        _prob_distr(0., 1.),
        _cluster_id_cnt(),
        _cluster_members(),
        _densities{},  // undefined here, will be set in constructor body
        _counts{},

        // Data output . . . . . . . . . . . . . . . . . . . . . . . . . . . .
        // Get output-related parameters
        _compression(get_as<int>("compression", this->_cfg)),
        _write_ca_data(get_as<bool>("write_ca_data", this->_cfg)),

        // Create the dataset for the densities and counts; shape is known
        _dset_densities(
            this->create_dset("densities", {static_cast<char>(Kind::COUNT)},
                              _compression)
        ),
        _dset_counts(
            this->create_dset("counts", {_counts.counts().size()},
                              _compression)
        ),

        // Create CellManager-based datasets
        _dset_kind(
            this->create_cm_dset("kind", _cm, _compression)),
        _dset_immune(
            this->create_cm_dset("immune", _cm, _compression)),
        _dset_num_recoveries(
            this->create_cm_dset("num_recoveries", _cm, _compression)),
        _dset_age(
            this->create_cm_dset("age", _cm, _compression)),
        _dset_cluster_id(
            this->create_cm_dset("cluster_id", _cm, _compression))
    {
        // Make sure the densities are not undefined
        _densities.fill(std::numeric_limits<double>::quiet_NaN());

        // Cells are already set up by the CellManager.
        // Remaining initialization steps regard only macroscopic quantities,
        // e.g. the setup of heterogeneities: Inert cells and infection source.

        // Inert
        if (_cfg["inert_cells"] and
            get_as<bool>("enabled", _cfg["inert_cells"]))
        {
            this->_log->info("Setting cells to be inert ...");

            // Get the container
            auto make_inert = _cm.select_cells(_cfg["inert_cells"]);

            // Apply a rule to all cells of that container: turn to an inert cell
            apply_rule<Update::async, Shuffle::off>(
                [](const auto& cell) {
                    auto& state = cell->state;
                    state.kind  = Kind::inert;
                    return state;
                },
                make_inert);

            this->_log->info("Set {} cells to be inert using selection mode "
                             "'{}'.",
                             make_inert.size(),
                             get_as<std::string>("mode", _cfg["inert_cells"]));
        }

        // Ignite some cells permanently: infection sources
        if (_cfg["infection_source"] and
            get_as<bool>("enabled", _cfg["infection_source"]))
        {
            this->_log->info("Setting cells to be infection sources ...");
            auto source_cells = _cm.select_cells(_cfg["infection_source"]);

            apply_rule<Update::async, Shuffle::off>(
                [](const auto& cell) {
                    auto& state = cell->state;
                    state.kind  = Kind::source;
                    return state;
                },
                source_cells);

            this->_log->info(
                "Set {} cells to be infection sources using "
                "selection mode '{}'.",
                source_cells.size(),
                get_as<std::string>("mode", _cfg["infection_source"]));
        }

        // Store the kind names (see state.hh) in the dataset attributes.
        _dset_kind->add_attribute("kind_names", kind_names);
        this->_log->debug("Stored metadata in 'kind' dataset.");

        // Add coordinate attributes to density and counts datasets
        _dset_densities->add_attribute("dim_name__1", "kind");
        _dset_densities->add_attribute("coords_mode__kind", "values");
        _dset_densities->add_attribute("coords__kind", kind_names);

        _dset_counts->add_attribute("dim_name__1", "label");
        _dset_counts->add_attribute("coords_mode__label", "values");
        _dset_counts->add_attribute("coords__label", _counts.labels());

        this->_log->debug("Added coordinate labels to 'densities' and "
                          "'counts' datasets.");

        // Initialization should be finished here. Provide some info
        this->_log->info("{} model fully set up.", this->_name);
        this->_log->info("  Writing CA data?    {}",
                         _write_ca_data ? "Yes" : "No");
        this->_log->info("  Compression level:  {}", _compression);
    }

  protected:
    // .. Helper functions ....................................................
    /// Update the densities array
    /** Each density is calculated by counting the number of state
     *  occurrences and afterwards dividing by the total number of cells.
     *
     *  @attention It is possible that rounding errors occur due to the
     *             division, thus, it is not guaranteed that the densities
     *             exactly add up to 1. The errors should be negligible.
     */
    void update_densities()
    {
        // Temporarily overwrite every entry in the densities with zeroes
        _densities.fill(0);

        // Count the occurrence of each possible state. Use the _densities
        // member for that in order to not create a new array.
        for (const auto& cell : this->_cm.cells()) {
            // Cast enum to integer to arrive at the corresponding index
            ++_densities[static_cast<char>(cell->state.kind)];
        }
        // The _densities array now contains the counts.

        // Calculate the actual densities by dividing the counts by the total
        // number of cells.
        for (auto&& d : _densities) {
            d /= static_cast<double>(this->_cm.cells().size());
        }
    };

    /// Identify clusters
    /** This function identifies clusters and updates the cell
     *  specific cluster_id as well as the member variable
     *  cluster_id_cnt that counts the number of ids
     */
    void identify_clusters()
    {
        // reset cluster counter
        _cluster_id_cnt = 0;
        apply_rule<Update::async, Shuffle::off>(_identify_cluster, _cm.cells());
    }

    /// Apply exposure control
    /** Add exposures if the iteration step matches the ones specified in
     *  the configuration. There are two available modes of exposure control
     *  that are applied, if provided, in this order:
     *
     *  1.  At specified times (parameter: ``at_times``) a number of additional
     *      exposures is added (parameter: ``num_additional_exposures``)
     *  2.  The parameter ``p_exposed`` is changed to a new value at given
     *      times. This can happen multiple times.
     *      Parameter: ``change_p_exposed``
     */
    void exposure_control()
    {
        // Check that time matches the first element of the sorted queue of
        // time steps at which to apply the given number of exposures.
        if (not _params.exposure_control.at_times.empty()) {
            // Check whether time has come for exposures
            if (this->_time == _params.exposure_control.at_times.front()) {
                // Select cells that are susceptibles
                // (not empty, inert, infected, or source)
                const auto cells_pool =
                    _cm.select_cells<SelectionMode::condition>(
                        [&](const auto& cell) {
                            return (cell->state.kind == Kind::susceptible);
                        });

                // Sample cells from the pool and ...
                CellContainer sample {};
                std::sample(cells_pool.begin(),
                            cells_pool.end(),
                            std::back_inserter(sample),
                            _params.exposure_control.num_additional_exposures,
                            *this->_rng);

                // ... and expose the sampled cells
                for (const auto& cell : sample) {
                    cell->state.kind = Kind::exposed;
                    _counts.exposed_via_exposure_control()++;
                }

                // Done. Can now remove first element of the queue.
                _params.exposure_control.at_times.pop();
            }
        }

        // Change p_exposed if the iteration step matches the ones
        // specified in the configuration. This leads to constant time lookup.
        if (not _params.exposure_control.change_p_exposed.empty()) {
            const auto change_p_exposed =
                _params.exposure_control.change_p_exposed.front();

            if (this->_time == change_p_exposed.first) {
                _params.p_exposed = change_p_exposed.second;

                // Done. Can now remove the element from the queue.
                _params.exposure_control.change_p_exposed.pop();
            }
        }
    }

    /// Apply immunity control
    /** Add cell immunities if the iteration step matches the ones specified in
     *  the configuration. There are two available modes of immunity control
     *  that are applied, if provided, in this order:
     *
     *  1.  At specified times (parameter: ``at_times``) a number of additional
     *      immmunities is added (parameter: ``num_additional_immmunities``)
     *  2.  The parameter ``immune`` is changed to a new value at given
     *      times. This can happen multiple times.
     *      Parameter: ``change_immune``
     */
    void immunity_control()
    {
        // Check that time matches the first element of the sorted queue of
        // time steps at which to apply the given number of immmunities.
        if (not _params.immunity_control.at_times.empty()) {
            // Check whether time has come for immmunities
            if (this->_time == _params.immunity_control.at_times.front()) {
                // Select cells that are susceptibles
                // (not empty, exposed, infected, inert, infected, or source)
                const auto cells_pool =
                    _cm.select_cells<SelectionMode::condition>(
                        [&](const auto& cell) {
                            return (cell->state.kind == Kind::susceptible);
                        });

                // Sample cells from the pool and ...
                CellContainer sample {};
                std::sample(cells_pool.begin(),
                            cells_pool.end(),
                            std::back_inserter(sample),
                            _params.immunity_control.num_additional_immunities,
                            *this->_rng);

                // ... and add immunity to the sampled cells
                for (const auto& cell : sample) {
                    cell->state.immune = true;
                }

                // Done. Can now remove first element of the queue.
                _params.immunity_control.at_times.pop();
            }
        }

        // Change immune if the iteration step matches the ones
        // specified in the configuration. This leads to constant time lookup.
        if (not _params.immunity_control.change_p_immune.empty()) {
            const auto change_p_immune =
                _params.immunity_control.change_p_immune.front();

            if (this->_time == change_p_immune.first) {
                _params.p_immune = change_p_immune.second;

                // Done. Can now remove the element from the queue.
                _params.immunity_control.change_p_immune.pop();
            }
        }
    }

    /// Apply transmission control
    /** Change the transmitting probability p_transmit for a subset of
     *  cells of a specified kind if the iteration step matches the ones
     *  specified in the configuration. The parameter ``p_transmit`` is changed
     *  to a new value at given times for a number of randomly chosen cells of
     *  specified kins. This can happen multiple times. Parameter:
     *  ``change_p_transmit``
     */
    void transmission_control()
    {
        // Change p_transmit if the iteration step matches the ones
        // specified in the configuration. This leads to constant time
        // lookup.
        if (not _params.transmission_control.change_p_transmit.empty()) {
            // Extract parameters
            const auto change_p_transmit =
                _params.transmission_control.change_p_transmit.front();

            const auto iteration_step = std::get<0>(change_p_transmit);
            const auto num_cells      = std::get<1>(change_p_transmit);
            const auto cell_kind      = std::get<2>(change_p_transmit);
            const auto p_transmit     = std::get<3>(change_p_transmit);

            if (this->_time == iteration_step) {
                // Select cells that are of desired kind
                const auto cells_pool =
                    _cm.select_cells<SelectionMode::condition>(
                        [&](const auto& cell) {
                            return (cell->state.kind == cell_kind);
                        });

                // Sample cells from the pool and ...
                CellContainer sample {};
                std::sample(cells_pool.begin(),
                            cells_pool.end(),
                            std::back_inserter(sample),
                            num_cells,
                            *this->_rng);

                // ... and change p_transmit for the sampled cells
                for (const auto& cell : sample) {
                    cell->state.p_transmit = p_transmit;
                }

                // Done. Can now remove the element from the queue.
                _params.transmission_control.change_p_transmit.pop();
            }
        }
    }

    // .. Rule functions ......................................................

    /// Define the update rule
    /** Update the given cell according to the following rules:
     * - Empty cells grow susceptible cells with probability p_susceptible.
     * - Cells in neighborhood of an infected cell do not get exposed
     *   with the probability p_random_immunity.
     * - Exposed cells become infected cells after a certain incubation period.
     * - Infected cells die with probability p_deceased and become an empty
     *   cell or they become recovered cells with 1-p_deceased.
     */
    RuleFunc _update = [this](const auto& cell) {
        // Get the current state of the cell and reset its cluster ID
        auto state       = cell->state;
        state.cluster_id = 0;

        // With probability p_empty transition any kind of cell to be empty
        // Note that this allows for empty cells to become susceptible directly
        // in the same update step. Having it at the end would, however, create
        // a similar issue with just appearing agents.
        // Be explicit in the kinds to assure that inert and sources do not
        // convert to empty cells
        // Note that this could probably be made more efficient through
        // sampling if more performance is needed.
        if ((state.kind == Kind::susceptible) or
            (state.kind == Kind::exposed) or
            (state.kind == Kind::infected) or
            (state.kind == Kind::recovered))
        {
            if (_prob_distr(*this->_rng) < _params.p_empty) {
                state.kind           = Kind::empty;
                state.num_recoveries = 0;
                state.immune         = false;
                _counts.living_to_empty()++;
            }
        }

        // Distinguish by current state
        if (state.kind == Kind::empty) {
            // With a probability of p_susceptible, set the cell's state to
            // susceptible
            if (_prob_distr(*this->_rng) < _params.p_susceptible) {
                state.kind = Kind::susceptible;
                _counts.empty_to_susceptible()++;

                // If a new susceptible cell appears it is immune with the
                // probability p_immune.
                if (_prob_distr(*this->_rng) < _params.p_immune) {
                    state.immune = true;
                }
                else {
                    state.immune = false;
                }

                // Initialize the probability p_transmit of the cell from
                // the configuration node
                state.p_transmit = State::initialize_p_transmit(
                    get_as<DataIO::Config>("p_transmit", this->_cfg),
                    this->_rng);

                // The new susceptible cell does not yet have recoveries
                state.num_recoveries = 0;

                return state;
            }
        }
        else if (state.kind == Kind::susceptible) {
            // Increase the age of the susceptible cell
            ++state.age;

            // If the cell is immune nothing happens :)
            // It can neither be infected through point infection nor through
            // contact with exposed or infected cells
            if (state.immune) {
                return state;
            }
            else {
                // Susceptible cell can be exposed by neighbor
                // or by random-point-infection.

                // Determine whether there will be a point infection resulting
                // in an exposed agent.
                if (_prob_distr(*this->_rng) < _params.p_exposed) {
                    // Yes, point infection occurred.
                    state.kind = Kind::exposed;
                    _counts.susceptible_to_exposed_random()++;
                    return state;
                }
                else {
                    // Go through neighbor cells (according to Neighborhood
                    // type) and check if they are infected
                    // (or an infection source).
                    // If yes, expose the cell with the probability p_transmit
                    // given by the neighbor's cell state if no random immunity
                    // given by p_random_immunity occurs.
                    for (const auto& nb : this->_cm.neighbors_of(cell)) {
                        // Get the neighbor cell's state
                        const auto& nb_state = nb->state;

                        if (nb_state.kind == Kind::infected or
                            nb_state.kind == Kind::exposed or
                            nb_state.kind == Kind::source)
                        {
                            // With the probability given by the neighbors cell
                            // p_transmit become exposed if no random immunity
                            // occurs
                            if (_prob_distr(*this->_rng) <
                                ((1. - _params.p_random_immunity) *
                                 nb_state.p_transmit))
                            {
                                state.kind = Kind::exposed;
                                _counts.susceptible_to_exposed_local()++;
                                return state;
                            }
                        }
                    }
                }
            }
        }
        else if (state.kind == Kind::exposed) {
            // Increase the age of the exposed cell
            ++state.age;

            // Transition from the exposed state to infected with probability
            // p_infected
            if (_prob_distr(*this->_rng) < _params.p_infected) {
                state.kind = Kind::infected;
                _counts.exposed_to_infected()++;
                return state;
            }

            return state;
        }
        else if (state.kind == Kind::infected) {
            // Increase the age of the infected cell
            ++state.age;

            // Increment the exposed time because if the cell is infected
            // the infected time adds to the total exposed time.
            ++state.exposed_time;

            if (_prob_distr(*this->_rng) < _params.p_recovered) {
                state.kind   = Kind::recovered;
                state.immune = true;
                ++state.num_recoveries;
                _counts.infected_to_recovered()++;
            }
            else if (_prob_distr(*this->_rng) < _params.p_deceased) {
                state.kind = Kind::deceased;
                _counts.infected_to_deceased()++;
                // Do not reset the cell states here to keep them for
                // write out and analysis. Resetting happens in the deceased
                // case below.
            }
            // else nothing happens and the cell stays infected the next time

            return state;
        }
        else if (state.kind == Kind::recovered) {
            // Increase the age of the recovered cell
            ++state.age;

            // Cells can lose their immunity and get susceptible again
            if (_prob_distr(*this->_rng) < _params.p_lose_immunity) {
                state.immune = false;
                state.kind   = Kind::susceptible;
                _counts.recovered_to_susceptible()++;
            }

            return state;
        }
        else if (state.kind == Kind::deceased) {
            // A former deceased cell gets empty
            state.kind           = Kind::empty;
            state.immune         = false;
            state.num_recoveries = 0;
            _counts.deceased_to_empty()++;  // ... for completeness

            // Reset the age for the next susceptible cell
            state.age = 0;

            return state;
        }
        // else: other cell states need no update

        // Return the (potentially changed) cell state for the next round
        return state;
    };

    /// Identify each cluster of susceptibles
    RuleFunc _identify_cluster = [this](const auto& cell) {
        if (cell->state.cluster_id != 0 or
            cell->state.kind != Kind::susceptible)
        {
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
            // Iterate over all potential cluster members nb, i.e. all
            // neighbors of cell cluster[i] that is already in the cluster
            for (const auto& nb : this->_cm.neighbors_of(cluster[i])) {
                // If it is susceptible and not yet in the cluster, add it.
                if (nb->state.cluster_id == 0 and
                    nb->state.kind == Kind::susceptible)
                {
                    nb->state.cluster_id = _cluster_id_cnt;
                    cluster.push_back(nb);
                    // This extends the outer for-loop...
                }
            }
        }

        return cell->state;
    };

    /// Move the agent on the cell away from an infected neighboring cell
    /** Check whether there is an infected cell in the neighborhood.
     *  If there is an empty cell in the neighborhood, move to it. If there
     *  is no empty spot in the neighborhood do nothing.
     */
    RuleFunc _move_away_from_infected = [this](const auto& cell) {
        // Get the reference to the state because it is directly manipulated
        auto& state = cell->state;

        auto neighbors = _cm.neighbors_of(cell);

        for (const auto& nb : neighbors) {
            if (nb->state.kind == Kind::infected) {
                // Shuffle the neighbors container ...
                std::shuffle(neighbors.begin(), neighbors.end(), *this->_rng);
                // ... go through it, and move to the first appearing empty
                // cell
                for (const auto& _nb : neighbors) {
                    if (_nb->state.kind == Kind::empty) {
                        // Swap the states and leave the loop
                        std::swap(state, _nb->state);
                        _counts.move_away_from_infected()++;
                        break;
                    }
                }
                // NOTE It is important to leave the outer loop because the
                //      neighbors container over which it is iterated was
                //      shuffled. This could result in undefined behavior and
                //      breaking code
                break;
            }
        }

        return state;
    };

    /// Move randomly to a neighboring cell if that cell is empty
    /** If the cell is not empty, an inert cell, or a source: move to
     *  neighboring empty location with probability `p_move_randomly`. If no
     *  neighboring cell is empty, do nothing.
     */
    RuleFunc _move_randomly = [this](const auto& cell) {
        // Get the state reference to directly manipulate the state
        auto& state = cell->state;

        // Directly return the state if the cell is of kind empty, source,
        // or inert.
        if ((state.kind == Kind::empty) or
            (state.kind == Kind::source) or
            (state.kind == Kind::inert))
        {
            return state;
        }

        if (_prob_distr(*this->_rng) < _params.p_move_randomly) {
            auto neighbors = _cm.neighbors_of(cell);

            // Shuffle the neighbors container ...
            std::shuffle(neighbors.begin(), neighbors.end(), *this->_rng);
            // ... go through it, and move to the first appearing empty cell
            for (const auto& _nb : neighbors) {
                if (_nb->state.kind == Kind::empty) {
                    // Swap the states and leave the loop
                    std::swap(state, _nb->state);
                    _counts.move_randomly()++;
                    break;
                }
            }
        }

        return state;
    };

  public:
    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Iterate a single time step
    /** This updates all cells (synchronously) according to the _update
     * rule. For specifics, see there.
     *
     * If exposure control is activated, the cells are first modified
     * according to the specific exposure control parameters.
     */
    void perform_step()
    {
        // Reset the state transition counters
        _counts.reset();

        // Apply exposure control if enabled
        if (_params.exposure_control.enabled) {
            exposure_control();
        }

        // Apply immunity control if enabled
        if (_params.immunity_control.enabled) {
            immunity_control();
        }

        // Apply transmit control if enabled
        if (_params.transmission_control.enabled) {
            transmission_control();
        }

        // Apply the update rule to all cells.
        apply_rule<Update::sync>(_update, _cm.cells());
        // NOTE The cell state is updated synchronously, i.e.: only after
        // all cells have been visited and know their state for the next step

        // Move cells randomly with probability p_move_randomly
        apply_rule<Update::async, Shuffle::on>(_move_randomly,
                                               _cm.cells(),
                                               *this->_rng);

        // Move the agents living on cells randomly
        if (_params.move_away_from_infected) {
            apply_rule<Update::async, Shuffle::on>(_move_away_from_infected,
                                                   _cm.cells(),
                                                   *this->_rng);
        }
    }

    /// Monitor model information
    /** Supplies the `densities` and `counts` arrays to the monitor.
     */
    void monitor()
    {
        update_densities();
        this->_monitor.set_entry("densities", _densities);
        this->_monitor.set_entry("counts", _counts.counts());
    }

    /// Write data
    void write_data()
    {
        // Update densities and write them
        update_densities();
        _dset_densities->write(_densities);

        // Store the counts
        _dset_counts->write(_counts.counts());

        // If CA data is not to be written, can return here
        if (not _write_ca_data) {
            return;
        }

        // Write the cell state
        _dset_kind->write(_cm.cells().begin(),
                          _cm.cells().end(),
                          [](const auto& cell) {
                              return static_cast<char>(cell->state.kind);
                          });

        // Write the immune state
        _dset_immune->write(_cm.cells().begin(),
                            _cm.cells().end(),
                            [](const auto& cell) {
                                return static_cast<char>(cell->state.immune);
                            });

        // Write the number of recoveries state
        _dset_num_recoveries->write(_cm.cells().begin(),
                                    _cm.cells().end(),
                                    [](const auto& cell) {
                                        return static_cast<unsigned short>(
                                            cell->state.num_recoveries);
                                    });

        // Write the susceptible ages
        _dset_age->write(_cm.cells().begin(),
                         _cm.cells().end(),
                         [](const auto& cell) {
                             return cell->state.age;
                         });

        // Identify clusters and write them out
        identify_clusters();
        _dset_cluster_id->write(_cm.cells().begin(),
                                _cm.cells().end(),
                                [](const auto& cell) {
                                    return cell->state.cluster_id;
                                });
    }
};

}  // namespace Utopia::Models::SEIRD

#endif  // UTOPIA_MODELS_SEIRD_HH
