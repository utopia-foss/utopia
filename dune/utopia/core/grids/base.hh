#ifndef UTOPIA_CORE_GRIDS_BASE_HH
#define UTOPIA_CORE_GRIDS_BASE_HH

#include "../types.hh"

namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

/// Type of the neighborhood calculating function
template<class Grid>
using NBFuncID = std::function<IndexContainer(const IndexType&)>;


/// Possible neighborhood types; availability depends on choice of grid
enum NBMode {
    /// Every entity is utterly alone in the world
    empty = 0,
    /// The vonNeumann neighborhood, i.e. only nearest neighbors
    vonNeumann = 1,
    /// The Moore neighborhood, i.e. nearest and next nearest neighbors
    Moore = 2
};
// NOTE When adding new neighborhood types, take care to update nb_mode_map!


/// A map from strings to neighborhood enum values
const std::map<std::string, NBMode> nb_mode_map {
    {"empty",       NBMode::empty},
    {"vonNeumann",  NBMode::vonNeumann},
    {"Moore",       NBMode::Moore}
};

/// Given an NBMode enum value, return the corresponding string key
/** \detail  This iterates over the nb_mode_map and returns the first key that
  *          matches the given enum value.
  */
std::string nb_mode_to_string(NBMode nb_mode) {
    for (const auto& m : nb_mode_map) {
        if (m.second == nb_mode) {
            return m.first;
        }
    }
    // Entry is missing; this should not happen, as the nb_mode_map is meant to
    // include all possible enum values. Inform about it ...
    throw std::invalid_argument("The given nb_mode was not available in the "
        "nb_mode_map! Are all NBMode values represented in the map?");
};


/// The base class for all grid discretizations used by the CellManager
template<class Space>
class Grid {
public:
    /// Type of this class, i.e. the base grid class
    using Self = Grid<Space>;

    /// The dimensionality of the space to be discretized (for easier access)
    static constexpr std::size_t dim = Space::dim;


protected:
    // -- Members -- //
    /// The space that is to be discretized
    const std::shared_ptr<Space> _space;

    /// How many cells to place per length unit of space
    /** \note The effective resolution might differ from this number, depending
      *       on the choice of resolution value and the physical extent of the
      *       space in each dimension.
      */
    const std::size_t _resolution;

    /// Neighborhood mode
    NBMode _nb_mode;

    /// Neighborhood function (working on cell IDs)
    NBFuncID<Self> _nb_func;

    /// The neighborhood distance (metric distance wrt. neighborhood mode)
    std::size_t _nbh_distance;


public:
    // -- Constructors and Destructors -- //
    /// Construct a grid discretization
    /** \param  space   The space to construct the discretization for
      * \param  cfg     Further configuration parameters
      */
    Grid (std::shared_ptr<Space> space, const DataIO::Config& cfg)
    :
        _space(space),
        _resolution([&](){
            if (not cfg["resolution"]) {
                throw std::invalid_argument("Missing grid configuration "
                    "parameter 'resolution'! Please supply an integer >= 1.");
            }
            auto res = as_<std::size_t>(cfg["resolution"]);

            if (res < 1) {
                throw std::invalid_argument("Grid resolution needs to be a "
                                            "positive integer!");
            }

            return res;
        }()),
        _nb_mode(NBMode::empty),
        _nbh_distance([&](){
            if (not cfg["nbh_distance"]) {
                return static_cast<std::size_t>(1);
            }
            else{
                return as_<std::size_t>(cfg["nbh_distance"]);
            }
        }())
    {
        // Set the neighborhood function to not return anything
        _nb_func = _nb_empty;
    }

    /// Virtual destructor to allow polymorphic destruction
    virtual ~Grid() = default;


    // -- Neighborhood interface -- //

    IndexContainer neighbors_of(const IndexType& id) const {
        return _nb_func(id);
    }

    void select_neighborhood(NBMode nb_mode) {
        try {
            _nb_func = get_nb_func(nb_mode);
        }
        catch (std::exception& e) {
            throw std::invalid_argument("Failed to select neighborhood: "
                                        + ((std::string) e.what()));
        }
        // Set the member to the new value
        _nb_mode = nb_mode;
    }


    // -- Getters -- //
    /// Get number of cells
    /** \detail This information is used by the CellManager to populate the
      *         cell container with the returned number of cells
      */
    virtual IndexType num_cells() const = 0;
    
    /// Get const reference to resolution of this grid
    std::size_t resolution() const {
        return _resolution;
    }

    /// Returns the effective resolution into each dimension of the grid
    /** \detail Depending on choice of resolution and extent of physical space,
      *         the resolution given at initialization might not represent the
      *         density of cells per unit of space fully accurately.
      *         The effective resolution accounts for the scaling that was
      *         required to map an integer number of cells onto the space.
      */
    virtual const std::array<double, dim> effective_resolution() const = 0;

    /// Get the shape of the grid discretization
    virtual const GridShapeType<Space::dim> shape() const = 0;

    /// Whether the grid is periodic
    bool is_periodic() const {
        return _space->periodic;
    }


protected:
    // -- Neighborhood interface -- //
    /// Retrieve the neighborhood function depending on the mode
    virtual NBFuncID<Self> get_nb_func(NBMode mode) const = 0;


    /// A neighborhood function for empty neighborhood
    NBFuncID<Self> _nb_empty = [](const IndexType&) {
        IndexContainer idcs{};
        idcs.reserve(0);
        return idcs;
    };
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRIDS_BASE_HH
