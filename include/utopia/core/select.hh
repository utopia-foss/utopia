#ifndef UTOPIA_CORE_SELECT_HH
#define UTOPIA_CORE_SELECT_HH

#include <map>
#include <unordered_set>
#include <random>
#include <algorithm>
#include <type_traits>

#include "types.hh"
#include "exceptions.hh"
#include "entity.hh"
#include "cell.hh"
#include "agent.hh"

/**
 *  \addtogroup EntitySelection
 *  \{
 *  \ingroup Core 
 *  \brief   An interface to select a subset of entities from a manager
 */

namespace Utopia {

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Possible selection modes; availability depends on choice of grid
enum class SelectionMode {
    // .. Working on entities . . . . . . . . . . . . . . . . . . . . . . . . .
    /// Select if a condition is fulfilled
    condition = 0,
    /// Select a random sample of entities with a known sample size
    sample = 1,
    /// Select an entity with a given probability
    probability = 2,

    // .. Only relevant for CellManager . . . . . . . . . . . . . . . . . . . .
    // (Offset by 100)
    /// (For CellManager only) Select the boundary cells of a grid
    boundary = 100,

    // .. Clustering . . . . . . . . . . . . . . . . . . . . . . . . . . . . . 
    // (Offset by 200)
    /// Select entities that are spatially clustered (algorithm: simple)
    /** \note Currently only implemented for CellManager
      */
    clustered_simple = 200
};
// NOTE When adding new enum members, take care to update the select_key_map!


/// A map from strings to Select enum values
const std::map<std::string, SelectionMode> selection_mode_map {
    {"condition",           SelectionMode::condition},
    {"sample",              SelectionMode::sample},
    {"probability",         SelectionMode::probability},
    {"boundary",            SelectionMode::boundary},
    {"clustered_simple",    SelectionMode::clustered_simple}
};

/// Given an Select enum value, return the corresponding string key
/** This iterates over the select_cell_map and returns the first key that
  * matches the given enum value.
  */
std::string selection_mode_to_string(const SelectionMode& mode) {
    for (const auto& m : selection_mode_map) {
        if (m.second == mode) {
            return m.first;
        }
    }
    // Entry is missing; this should not happen, as the map is meant to
    // include all possible enum values. Inform about it ...
    throw std::invalid_argument("The given entity selection mode is not "
        "available! Are all Select enum values represented in the map?");
};


/// Metafunction to determine whether a Manager is a CellManager
/** \note Relies on export of the traits type by \ref Entity.
  */
template<class M>
struct is_cell_manager { 
    static constexpr bool value =
        std::is_same_v<Cell<typename M::Entity::Traits>,
                       typename M::Entity>;
};

/// Metafunction to determine whether a Manager is an AgentManager
/** \note Relies on export of the traits type by \ref Entity and the manager
  *       types exporting the \ref Utopia::Space type.
  */
template<class M>
struct is_agent_manager { 
    static constexpr bool value =
        std::is_same_v<Agent<typename M::Entity::Traits,
                             typename M::Space>,
                       typename M::Entity>;
};

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++ Convenience Wrappers ++++++++++++++++++++++++++++++++++++++++++++++++++++


/// Select entities according to parameters specified in a configuration
/** Via the ``mode`` key, one of the modes (see \ref Utopia::SelectionMode) can
  * be selected. Depending on that mode, the other parameters are extracted
  * from the configuration.
  *
  * \param  mngr     The manager to select the entities from
  * \param  sel_cfg  The configuration node containing the expected
  *                  key-value pairs specifying the selection.
  */
template<
    class Manager,
    class Container = EntityContainer<typename Manager::Entity>>
Container select_entities(const Manager& mngr, const DataIO::Config& sel_cfg)
{
    // Determine the selection mode
    if (not sel_cfg["mode"]) {
        throw KeyError("mode", sel_cfg, "Could not select entities!");
    }
    const auto mode_str = get_as<std::string>("mode", sel_cfg);

    if (not selection_mode_map.count(mode_str)) {
        throw std::invalid_argument("The given selection mode string ('"
            + mode_str +"') is invalid! For available modes, consult the "
            "EntitySelection group in the doxygen documentation.");
    }
    const SelectionMode mode = selection_mode_map.at(mode_str);

    // Depending on the mode, extract the required parameters and invoke
    // the mode-specific methods directly
    // .. Generally available .................................................
    if (mode == SelectionMode::sample) {
        const auto N = get_as<int>("num_cells", sel_cfg);
        return select_entities<SelectionMode::sample>(mngr, N);
    }

    if (mode == SelectionMode::probability) {
        const auto p = get_as<double>("probability", sel_cfg);
        return select_entities<SelectionMode::probability>(mngr, p);
    }

    // .. Only for CellManager ................................................
    if constexpr (is_cell_manager<Manager>::value) {
        if (mode == SelectionMode::boundary) {
            const auto b = get_as<std::string>("boundary", sel_cfg);
            return select_entities<SelectionMode::boundary>(mngr, b);
        }

        if (mode == SelectionMode::clustered_simple) {
            const auto p_seed = get_as<double>("p_seed", sel_cfg);
            const auto p_attach = get_as<double>("p_attach", sel_cfg);
            const auto num_passes = get_as<unsigned>("num_passes", sel_cfg);

            // TODO Also make the neighborhood mode configurable here

            return
                select_entities<SelectionMode::clustered_simple>(mngr,
                                                                 p_seed,
                                                                 p_attach,
                                                                 num_passes);
        }
    }

    // .. Only for AgentManager ...............................................
    if constexpr (is_agent_manager<Manager>::value) {
        // ...
    }

    // If this point is reached, we did not return, i.e.: no type was available
    throw std::invalid_argument("The selection mode '"
        + selection_mode_to_string(mode) + "' is not available for the given "
        "manager type!");
}


// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ++ General selection functions +++++++++++++++++++++++++++++++++++++++++++++

/// Return a container with entities that match the given condition
/** 
  * \param  mngr       The manager to select the entities from
  * \param  condition  A unary function working on a single entity and
  *                    returning a boolean.
  */
template<
    SelectionMode mode,
    class Manager,
    class Container = EntityContainer<typename Manager::Entity>,
    class Condition = std::function<bool(const std::shared_ptr<typename Manager::Entity>&)>,
    typename std::enable_if_t<mode == SelectionMode::condition, int> = 0
    >
Container select_entities(const Manager& mngr,
                          const Condition& condition)
{
    Container selected{};
    std::copy_if(mngr.entities().begin(), mngr.entities().end(),
                 std::back_inserter(selected),
                 condition);
    return selected;
}

/// Select a sample of entities randomly
/** Uses std::sample to select ``num_entities`` randomly.
  *
  * \note   The order of the entities in the returned container is the same as
  *         in the underlying container!
  *
  * \param  mngr         The manager to select the entities from
  * \param  num_entities The number of entities to sample from the given
  *                      container. Need be a value in [0, container size]
  */
template<
    SelectionMode mode,
    class Manager,
    class Container = EntityContainer<typename Manager::Entity>,
    typename std::enable_if_t<mode == SelectionMode::sample, int> = 0
    >
Container select_entities(const Manager& mngr,
                          const int num_entities)
{
    if (   num_entities < 0
        or static_cast<std::size_t>(num_entities) > mngr.entities().size())
    {
        throw std::invalid_argument("Argument num_entities need be in the "
            "interval [0, entity container size]!");
    }

    Container selected{};
    selected.reserve(num_entities);

    // Populate it with the sampled cells, then return
    std::sample(mngr.entities().begin(), mngr.entities().end(),
                std::back_inserter(selected),
                static_cast<std::size_t>(num_entities), *mngr.rng());
    return selected;
}

/// Select entities with a certain probability
/** Iterates over all entities and selects each entity with the given
  * probability.
  *
  * \note   The order of the entities in the returned container is the same as
  *         in the underlying container!
  *
  * \param  mngr         The manager to select the entities from
  * \param  probability  The probablity with which a single entity is selected
  */
template<
    SelectionMode mode,
    class Manager,
    class Container = EntityContainer<typename Manager::Entity>,
    typename std::enable_if_t<mode == SelectionMode::probability, int> = 0
    >
Container select_entities(const Manager& mngr,
                          const double probability)
{
    // Before drawing a huge amount of random numbers, check obvious cases
    if (probability == 0.) {
        return {};
    }
    else if (probability == 1.) {
        return mngr.entities();
    }
    else if (probability < 0. or probability > 1.) {
        throw std::invalid_argument("Entity selection in mode 'probability' "
            "failed due to probability argument outside of interval [0., 1.]");
    }

    // Build the distribution, selection condition lambda, and select ...
    std::uniform_real_distribution<> dist(0., 1.);

    return 
        select_entities<SelectionMode::condition>(
            mngr,
            // Declare a mutable lambda (needed for dist)
            [&, dist{std::move(dist)}](const auto&) mutable {
                return (dist(*mngr.rng()) < probability);
            }
        );
}

// ++ Cell-based selection functions ++++++++++++++++++++++++++++++++++++++++++

/// Selects cells on a boundary
/** Invokes \ref CellManager::boundary_cells
  *
  * \param  mngr      The manager to select the entities from. Needs to be
  *                   a Utopia::CellManager for this function.
  * \param  boundary  Which boundary to select.
  */
template<
    SelectionMode mode,
    class Manager,
    class Container = EntityContainer<typename Manager::Entity>,
    typename std::enable_if_t<mode == SelectionMode::boundary, int> = 0
    >
Container select_entities(const Manager& mngr,
                          const std::string& boundary)
{
    return mngr.boundary_cells(boundary);
}


/// Selects cells that are clustered using a simple clustering algorithm
/** This is done by first determining some "seed" cells and then attaching
  * their neighbors to them with a certain probability. 
  *
  * \param  mngr        The manager to select the entities from. Needs to be
  *                     a Utopia::CellManager for this function.
  * \param  p_seed      The probability with which a cell becomes a seed for a
  *                     cluster
  * \param  p_attach    The probability with which a neighbor of a cluster cell
  *                     is also added to the cluster
  * \param  num_passes  How often to invoke the attachment iteration
  *
  * \TODO Could generalize this to all kinds of entities that have neighbors.
  */
template<
    SelectionMode mode,
    class Manager,
    class Container = EntityContainer<typename Manager::Entity>,
    typename std::enable_if_t<mode == SelectionMode::clustered_simple, int> = 0
    >
Container select_entities(const Manager& mngr,
                          const double p_seed,
                          const double p_attach,
                          const unsigned int num_passes)
{
    if (p_attach < 0. or p_attach > 1.) {
        throw std::invalid_argument("Argument p_attach needs to be a "
            "probability, i.e. be in interval [0., 1.]!");
    }
    if (num_passes < 0) {
        throw std::invalid_argument("Argument num_passes needs to be a "
            "positive value or zero!");
    }

    mngr.log()->debug("Selecting cell clusters ... (p_seed: {}, p_attach: {}, "
                      "num_passes: {})", p_seed, p_attach, num_passes);

    // Get an initial selection of clustering "seeds"
    const auto seeds = select_entities<SelectionMode::probability>(mngr,
                                                                   p_seed);

    mngr.log()->debug("Selected {} clustering seeds.", seeds.size());

    // Need to work on a set and also need a temporary container for those
    // cells that are to be added after each pass.
    // To make this more efficient, work on cell IDs instead of shared_ptr.
    std::unordered_set<IndexType> selected_ids{};
    std::unordered_set<IndexType> ids_to_attach{};

    // Populate the set of selected IDs
    for (const auto& cell : seeds) {
        selected_ids.insert(cell->id());
    }

    // For probabilities, need a distribution object
    std::uniform_real_distribution<> dist(0., 1.);

    // Do multiple passes ...
    for (unsigned int i = 0; i < num_passes; i++) {
        // ... in which all already selected cells are iterated over and
        // each cell's neighbours are added with the given attachment
        // probability

        // Determine which cell IDs to attach
        ids_to_attach.clear();
        for (const auto cell_id : selected_ids) {
            for (const auto nb_id : mngr.grid()->neighbors_of(cell_id)) {
                if (dist(*mngr.rng()) < p_attach) {
                    ids_to_attach.insert(nb_id);
                }
            }
        }

        // Add them to the set of selected cells
        selected_ids.insert(ids_to_attach.begin(), ids_to_attach.end());
        
        mngr.log()->debug("Finished pass {}. Have {} cells selected now.",
                          i + 1, selected_ids.size());
    }

    // Convert into container of pointers to cells and return
    return 
        mngr.entity_pointers_from_ids(
            std::vector<IndexType>(selected_ids.begin(), selected_ids.end())
        );
}


// ++ Agent-based selection functions +++++++++++++++++++++++++++++++++++++++++
// ... nothing yet.

// end group EntitySelection
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_SELECT_HH
