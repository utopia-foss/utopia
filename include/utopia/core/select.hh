#ifndef UTOPIA_CORE_SELECT_HH
#define UTOPIA_CORE_SELECT_HH

#include <map>
#include <set>
#include <unordered_set>
#include <random>
#include <algorithm>
#include <type_traits>

#include <armadillo>
#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>  // for fmt::

#include "types.hh"
#include "exceptions.hh"
#include "entity.hh"
#include "cell.hh"
#include "agent.hh"
#include "logging.hh"
#include "../data_io/cfg_utils.hh"

/**
 *  \addtogroup EntitySelection Entity Selection
 *  \{
 *  \ingroup Core
 *  \brief   An interface to select a subset of entities from a manager
 *
 *  The Utopia constructs \ref Utopia::Cell and \ref Utopia::Agent both have a
 *  common parent type: \ref Utopia::Entity.
 *  Additionally, there are corresponding manager types that provide an
 *  interface to work with these entities: \ref Utopia::CellManager and
 *  \ref Utopia::AgentManager.
 *
 *  Given this structure, Utopia is able to provide a common interface with
 *  which a subset of entities can be selected in a consistent and
 *  configurable fashion. This allows to re-use the selection algorithms and
 *  while also allowing specializations for certain entity types.
 *
 *  The interface requires managers of \ref Utopia::Entity -derived types;
 *  let's call them ``EntityManagers`` for now. (Side note: The interface is
 *  currently only *assumed*; the CellManager and AgentManager are yet to be
 *  based on a common base class).
 *  These managers provide the embedding of a container of entities, which is
 *  required as soon as more complicated selections are to be performed, e.g.
 *  selecting cells that are distributed in a certain pattern in space.
 *
 *  For the mode of selection, the \ref Utopia::SelectionMode is used. For each
 *  mode value, a \ref Utopia::select_entities method is specialized. That
 *  method defines the parameters required for the selection.
 *  The whole interface is made accessible via configuration by the
 *  specialization that accepts a \ref Utopia::DataIO::Config as argument.
 *  Furthermore, the entity managers can implement a method that makes use of
 *  the free functions defined here, for example
 *  \ref Utopia::CellManager::select_entities .
 *
 *  This selection interface has a similar motivation as the
 *  \ref Utopia::apply_rule interface; in fact, it cooperates nicely with that
 *  interface, as you can first select some entities and then apply an
 *  operation to them...
 */

namespace Utopia {

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


/*!
 * \addtogroup SelectionModes
 * \{
 */

/// Possible selection modes; availability depends on choice of manager
/** For further details, consult the actual implementations.
 *
 *  \warning  Associated integer values may be subject to change.
 */
enum class SelectionMode {
    // .. Working on entities . . . . . . . . . . . . . . . . . . . . . . . . .
    /// Select if a condition is fulfilled
    condition = 0,

    /// Select a random sample of entities with a known sample size
    sample = 1,

    /// Select an entity with a given probability
    probability = 2,

    // .. Clustering . . . . . . . . . . . . . . . . . . . . . . . . . . . . .
    // (Offset by 20 to accomodate different algorithms)
    /// Select entity clusters using a simple neighborhood-based algorithm
    /** Uses the "simple" algorithm: From a given start population, iterate
      * over neighbors and attach them with a certain probability.
      *
      * \note Currently only implemented for CellManager, but expandable to
      *       all managers that provide a neighborhood interface for the
      *       entities they manage.
      */
    clustered_simple = 20,

    // .. Only relevant for CellManager . . . . . . . . . . . . . . . . . . . .
    // (Offset by 100)
    /// (For CellManager only) Selects cells at given positions in space
    position = 100,

    /// (For CellManager only) Select the boundary cells of a grid
    boundary = 101,

    /// (For CellManager only) Selects horizontal or vertical lanes of cells
    lanes = 102

    // .. Only relevant for AgentManager . . . . . . . . . . . . . . . . . . .
    // (Offset by 200)

    // .. Only relevant for GraphManager . . . . . . . . . . . . . . . . . . .
    // (Offset by 300)

    // NOTE When adding new enum members, take care to update the
    //      select_key_map!
};


/// A map from strings to Select enum values
const std::map<std::string, SelectionMode> selection_mode_map {
    // General
    {"condition",           SelectionMode::condition},
    {"sample",              SelectionMode::sample},
    {"probability",         SelectionMode::probability},

    // CellManager
    {"position",            SelectionMode::position},
    {"boundary",            SelectionMode::boundary},
    {"lanes",               SelectionMode::lanes},

    // Clustered
    {"clustered_simple",    SelectionMode::clustered_simple}
};

/// Given a SelectionMode enum value, return the corresponding string key
/** This iterates over the selection_mode_map and returns the first key that
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
        "available! Are all SelectionMode values represented in the map?");
};

// end group SelectionModes
/**
 *  \}
 */


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
  * Available keys for each mode:
  *     - `sample` mode: `num_cells`
  *     - `probability` mode: `probability`
  *     - `positions` mode: `positions` (a list of coordinate pairs)
  *     - `boundary` mode: `boundary` (a string, e.g. `left`, `right`, `top`,
  *         `bottom`)
  *     - `lanes` mode: `num_horizontal`, `num_vertical`, `permeability`
  *         (optional; can be given as scalar, pair, or mapping w/ keys
  *         `horizontal` and `vertical`), `gate_width` (optional, same as
  *         `permeability`)
  *     - `clustered_simple` mode: `p_seed`, `p_attach`, `num_passes`
  *
  * \note   The \ref Utopia::SelectionMode::condition is not available!
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

    mngr.log()->debug("Selecting entities using mode '{}' ...", mode_str);
    mngr.log()->debug("Parameters:\n{}", DataIO::to_string(sel_cfg));

    // Depending on the mode, extract the required parameters and invoke
    // the mode-specific methods directly
    // .. Generally available .................................................
    if (mode == SelectionMode::sample) {
        const auto N = get_as<int>("num_cells", sel_cfg); // FIXME not general!
        return select_entities<SelectionMode::sample>(mngr, N);
    }

    if (mode == SelectionMode::probability) {
        const auto p = get_as<double>("probability", sel_cfg);
        return select_entities<SelectionMode::probability>(mngr, p);
    }

    // .. Only for CellManager ................................................
    if constexpr (is_cell_manager<Manager>::value) {
        if (mode == SelectionMode::position) {
            // Populate a vector with SpaceVec objects
            std::vector<typename Manager::SpaceVec> positions{};

            if (not sel_cfg["positions"]) {
               throw KeyError("positions", sel_cfg,
                              "Could not select cells by positions!");
            }

            for (const auto& pos_node : sel_cfg["positions"]) {
                using SpaceVec = typename Manager::SpaceVec;
                using ET = typename Manager::SpaceVec::elem_type;

                const auto vec = pos_node.as<std::array<ET, Manager::dim>>();

                // Construct the SpaceVec from the array
                // NOTE Can be outsourced by making get_as directly accept
                //      nodes instead of (key, parent node) pairs.
                SpaceVec pos;
                for (DimType i = 0; i < Manager::dim; i++) {
                    pos[i] = vec[i];
                }
                positions.push_back(pos);
            }

            return select_entities<SelectionMode::position>(mngr, positions);
        }

        if (mode == SelectionMode::boundary) {
            const auto b = get_as<std::string>("boundary", sel_cfg);
            return select_entities<SelectionMode::boundary>(mngr, b);
        }

        if (mode == SelectionMode::lanes) {
            const auto num_v = get_as<unsigned>("num_vertical", sel_cfg);
            const auto num_h = get_as<unsigned>("num_horizontal", sel_cfg);

            // Handle optional arguments
            // Permeability
            auto perm = std::pair<double, double>{0., 0.};
            auto perm_cfg = sel_cfg["permeability"];
            if (perm_cfg and perm_cfg.IsSequence()) {
                perm = perm_cfg.as<std::pair<double, double>>();
            }
            else if (perm_cfg and perm_cfg.IsMap()) {
                perm.first = get_as<double>("horizontal", perm_cfg);
                perm.second = get_as<double>("vertical", perm_cfg);
            }
            else if (perm_cfg and perm_cfg.IsScalar()) {
                perm.first = perm_cfg.as<double>();
                perm.second = perm_cfg.as<double>();
            }

            // Gate width
            auto gate_width = std::pair<unsigned, unsigned>{0, 0};
            auto gate_cfg = sel_cfg["gate_width"];
            if (gate_cfg and gate_cfg.IsSequence()) {
                gate_width = gate_cfg.as<std::pair<unsigned, unsigned>>();
            }
            else if (gate_cfg and gate_cfg.IsMap()) {
                gate_width.first = get_as<unsigned>("horizontal", gate_cfg);
                gate_width.second = get_as<unsigned>("vertical", gate_cfg);
            }
            else if (gate_cfg and gate_cfg.IsScalar()) {
                gate_width.first = gate_cfg.as<unsigned>();
                gate_width.second = gate_cfg.as<unsigned>();
            }

            return select_entities<SelectionMode::lanes>(mngr, num_v, num_h,
                                                         perm, gate_width);
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
        "manager type or via the configuration!");
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

/// Selects cells at given positions in space
/** Invokes \ref CellManager::cell_at to populate a container of cells that
  * are at the given absolute positions. Which cells are selected depends on
  * the chosen discretization, see \ref Utopia::Grid and the derived classes.
  *
  * \param  mngr      The manager to select the entities from. Needs to be
  *                   a \ref Utopia::CellManager for this work.
  * \param  positions Absolute positions; the cells under these positions are
  *                   then selected.
  */
template<
    SelectionMode mode,
    class Manager,
    class Container = EntityContainer<typename Manager::Entity>,
    typename std::enable_if_t<mode == SelectionMode::position, int> = 0
    >
Container select_entities(const Manager& mngr,
                          const std::vector<typename Manager::SpaceVec>& positions)
{
    Container selected{};
    for (const auto& pos : positions) {
        selected.push_back(mngr.cell_at(pos));
    }
    return selected;
}

/// Selects cells on a boundary
/** Invokes \ref CellManager::boundary_cells
  *
  * \param  mngr      The manager to select the entities from. Needs to be
  *                   a \ref Utopia::CellManager for this work.
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


/// Selects horizontal or vertical lanes of cells
/** The lanes are spaced such that the domain is divided into N equally large
  * parts for periodic space and N+1 parts for non-periodic space in each
  * dimension.
  *
  * For example:
  *
  *    - In non-periodic space, two vertical lanes will be set at 1/3 and 2/3
  *      relative position of the space, thus dividing the domain into three
  *      parts in x-direction
  *    - In periodic space, one needs to take the wraparound into account. Two
  *      vertical lanes would then be set at the lower-value boundary and at
  *      the center of the grid and would divide the domain into _two_ parts!
  *      For three lanes, they would be at 0/3, 1/3, and 2/3 of the relative
  *      space extent along the x-dimension.
  *
  * Calculation occurs by first determining the relative position along the
  * corresponding dimension at which a lane is to occur. From that, the multi-
  * multi-index is computed, and then all cells that match the desired multi
  * index component are selected to become part of the lane.
  * This is done on the grid level, i.e.: on the level of multi indices. As all
  * grid discretizations can operate on multi-indices, this approach is valid
  * among all types of grid discretizations.
  *
  * \param  mngr            The manager to select the entities from. Needs to
  *                         be a \ref Utopia::CellManager .
  * \param  num_vertical    Number of vertical lanes
  * \param  num_horizontal  Number of horizontal lanes
  * \param  permeability    The permeability of the horizontal and vertical
  *                         lanes, respectively. The probability that a cell is
  *                         spared out. Optional.
  * \param  gate_width      The width of gates in the horizontal and vertical
  *                         lanes, respectively. Given in number of cells per
  *                         compartment, i.e. between two lanes. Optional.
  */
template<
    SelectionMode mode,
    class Manager,
    class Container = EntityContainer<typename Manager::Entity>,
    typename std::enable_if_t<mode == SelectionMode::lanes, int> = 0
    >
Container select_entities(
    const Manager& mngr,
    const unsigned int num_vertical,
    const unsigned int num_horizontal,
    const std::pair<double, double> permeability = {0., 0.},
    const std::pair<unsigned int, unsigned int> gate_width = {0, 0}
)
{
    static_assert(Manager::Space::dim == 2, "Only 2D space is supported.");
    using SpaceVec = typename Manager::SpaceVec;
    using MultiIndex = typename Manager::MultiIndex;

    // Get the shared pointers to the grid and some further information
    const auto grid = mngr.grid();
    const MultiIndex shape = grid->shape();
    const auto num_cells = grid->num_cells();
    const SpaceVec extent = grid->space()->extent;
    const auto eff_resolution = grid->effective_resolution();

    // The number of lanes should not exceed the number of cells
    if (num_vertical >= shape[0] or num_horizontal >= shape[1]) {
        throw std::invalid_argument("Given number of vertical and/or "
            "horizontal lanes is equal or larger to the number of cells along "
            "that dimension! Choose a smaller value.");
    }

    // Check permeability
    if (permeability.first < 0. or permeability.first > 1.) {
        throw std::invalid_argument(fmt::format(
            "Permeability in horizontal lanes needs to be in interval "
            "[0., 1.], but was: {}", permeability.first)
        );
    }
    if (permeability.second < 0. or permeability.second > 1.) {
        throw std::invalid_argument(fmt::format(
            "Permeability in vertical lanes needs to be in interval "
            "[0., 1.], but was: {}", permeability.second)
        );
    }

    // Emit information
    mngr.log()->debug(
        "Selecting cells for lanes ...\n"
        "   num:            {} horizontal, \t{} vertical\n"
        "   permeability:   {} horizontal, \t{} vertical\n"
        "   gate width:     {} horizontal, \t{} vertical\n",
        num_horizontal, num_vertical,
        permeability.first, permeability.second,
        gate_width.first, gate_width.second
    );

    // .. Lanes ...............................................................
    // Define the required variables for vertical and horizontal lanes. It is
    // important to work on absolute positions such that rounding errors are
    // not propagated along the grid ...
    // Do all this vector based to be able to use armadillo
    const auto num_lanes = MultiIndex{num_vertical, num_horizontal};
    SpaceVec lane_start, lane_step;

    if (grid->is_periodic()) {
        lane_start.fill(0.);
        lane_step = extent / num_lanes;
    }
    else {
        lane_start = extent / (num_lanes + 1);
        lane_step = lane_start;
    }

    // Determine x- and y-indices for all the lanes that can be reached with
    // these positions. To avoid rounding errors, use the absolute position to
    // find the first cells of each lane: construct a proxy position and then
    // ask the grid what the corresponding multi index is. The respective
    // component can then be used to select the lanes.
    // Using sets to have faster lookups
    std::set<IndexType> indices_x, indices_y;

    for (unsigned int i = 0; i < num_vertical; i++) {
        const SpaceVec proxy_pos = {lane_start[0] + i * lane_step[0], 0.};
        indices_x.insert(grid->midx_of(grid->cell_at(proxy_pos))[0]);
    }

    for (unsigned int i = 0; i < num_horizontal; i++) {
        const SpaceVec proxy_pos = {0., lane_start[1] + i * lane_step[1]};
        indices_y.insert(grid->midx_of(grid->cell_at(proxy_pos))[1]);
    }

    // .. Gates in lanes ......................................................
    // Gates are centered between the lanes
    auto num_gates = num_lanes;
    SpaceVec gate_start, gate_step;
    const SpaceVec grid_step = 1./eff_resolution;

    if (grid->is_periodic()) {
        gate_step = extent / num_gates;
    }
    else {
        num_gates = num_gates + 1;
        gate_step = extent / num_gates;
    }

    // center of gate at gate_step / 2.
    // but, want lower edge of the gate and iterate from there
    // hence, distinguish pair and impair gates width
    if (gate_width.first % 2 == 0) {
        gate_start = gate_step / 2. - SpaceVec(
                        {grid_step[0] * (gate_width.first - 1) / 2.,
                         grid_step[1] * (gate_width.second - 1) / 2.});
    }
    else {
        gate_start = gate_step / 2. - SpaceVec(
                        {grid_step[0] * (gate_width.first) / 2.,
                         grid_step[1] * (gate_width.second) / 2.});
    }

    // Determine x- and y-indices for every gate
    // Need error handling here because with a large gate width in non-periodic
    // space, the grid->cell_at lookup might throw std::invalid_argument
    std::set<IndexType> gates_indices_x, gates_indices_y;
    SpaceVec proxy_pos;

    try {
        for (unsigned int i = 0; i < num_gates[0]; i++) {
            for (unsigned int j = 0; j < gate_width.first; j++) {
                proxy_pos = {gate_start[0] + i*gate_step[0] + j*grid_step[0],
                             0.};
                gates_indices_x.insert(
                    grid->midx_of(grid->cell_at(proxy_pos))[0]
                );
            }
        }
        for (unsigned int i = 0; i < num_gates[1]; i++) {
            for (unsigned int j = 0; j < gate_width.second; j++) {
                proxy_pos = {0.,
                             gate_start[1] + i*gate_step[1] + j*grid_step[1]};
                gates_indices_y.insert(
                    grid->midx_of(grid->cell_at(proxy_pos))[1]
                );
            }
        }
    }
    catch (std::invalid_argument& exc) {
        throw std::invalid_argument(fmt::format(
            "Failed to determine gate cells for lane selection, presumably "
            "because the gate width was chosen larger than the compartment "
            "size. Check that the gate width (h: {:d}, v: {:d}) fits into the "
            "compartment. Grid shape: ({:d} x {:d}, {}). "
            "Number of lanes: (h: {:d}, v: {:d}).",
            gate_width.first, gate_width.second,
            shape[0], shape[1],
            grid->is_periodic() ? "periodic" : "non-periodic",
            num_horizontal, num_vertical
        ));
    }

    // .. ID selection ........................................................
    // Now need a container for selected cell IDs
    std::vector<IndexType> selected_ids{};

    // Build the distribution, selection condition lambda, and select ...
    std::uniform_real_distribution<> dist(0., 1.);

    // Populate it by iterating over all grid cell IDs, determining their
    // multi index, and then checking it against the containers of cells.
    // NOTE There is hardly a way around std::set::find if one wants to
    //      ascertain that the lanes are distributed evenly on the grid, which
    //      requires to determine the desired multi index components explicitly
    //      rather than calculating them via modulo operations (which adds
    //      rounding errors that are propagated over the grid). Still, the
    //      std::set::find method is rather efficient (logarithmic complexity)
    //      as it operates on a tree and the indices can be easily sorted.
    for (IndexType cell_id = 0; cell_id < num_cells; cell_id++) {
        const auto midx = grid->midx_of(cell_id);

        // Check whether this cell belongs to a horizontal lane and
        // if so, add its index to the container of indices.
        if (indices_y.find(midx[1]) != indices_y.end()) {
            // skip because this cell is a gate
            if (gates_indices_x.find(midx[0]) != gates_indices_x.end()) {
                continue;
            }
            // skip because of permeability
            else if (permeability.first > 0. and
                     dist(*mngr.rng()) < permeability.first)
            {
                continue;
            }

            // else: not skipped. Add the cell ID
            selected_ids.push_back(cell_id);
        }
        // Do the same for cells belonging to vertical lanes
        else if (indices_x.find(midx[0]) != indices_x.end()) {
            // skip because this cell is a gate
            if (gates_indices_y.find(midx[1]) != gates_indices_y.end()) {
                continue;
            }
            // skip because of permeability
            else if (permeability.second > 0. and
                     dist(*mngr.rng()) < permeability.second)
            {
                continue;
            }

            // else: not skipped. Add the cell ID
            selected_ids.push_back(cell_id);
        }
    }
    // Alternative implementation: For each of the entries in the indices_*
    // containers, construct a full multi index and then compute the cell ID
    // from it. However, the Grid does not currently supply a method to
    // calculate the ID from a given multi index.

    mngr.log()->debug("Selected {:d} / {:d} cells using mode 'lanes'.",
                    selected_ids.size(), mngr.cells().size());

    // Return as container of shared pointers to cell objects
    return mngr.entity_pointers_from_ids(selected_ids);
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
  * \todo TODO Could generalize this to all kinds of entities with neighbors.
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
