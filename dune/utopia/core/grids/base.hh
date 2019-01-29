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

    /// The type of the grid shape array
    using GridShape = GridShapeType<dim>;

protected:
    // -- Members -- //
    /// The space that is to be discretized
    const std::shared_ptr<Space> _space;

    /// The rectangular (multi-index) shape of the discretization
    const GridShape _shape;

    /// Neighborhood mode
    NBMode _nb_mode;

    /// Neighborhood function (working on cell IDs)
    NBFuncID<Self> _nb_func;

public:
    // -- Constructors and Destructors -- //
    /// Construct a grid discretization
    /** \param  space     The space to construct the discretization for
      * \param  shape     The shape of the grid
      */
    Grid (std::shared_ptr<Space> space,
          const GridShape& shape)
    :
        _space(space),
        _shape(shape),
        _nb_mode(NBMode::empty)
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
    IndexType num_cells() {
        return calc_num_cells();
    }
    
    /// Get const reference to grid shape
    const GridShapeType<Space::dim>& shape() const {
        return _shape;
    }

    /// Whether the grid is periodic
    bool is_periodic() const {
        return _space->periodic;
    }



protected:
    // -- Helper functions -- //
    /// Calculate the number of cells required to fill the current grid shape
    virtual IndexType calc_num_cells() = 0;


    // -- Neighborhood interface -- //
    /// Retrieve the neighborhood function depending on the mode
    virtual NBFuncID<Self> get_nb_func(NBMode mode) = 0;


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
