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
    static constexpr DimType dim = Space::dim;

    /// The type of vectors that have a relation to physical space
    using SpaceVec = typename Space::SpaceVec;

    /// The type of multi-index like arrays, e.g. the grid shape
    using MultiIndex = MultiIndexType<dim>;


protected:
    // -- Members -------------------------------------------------------------
    /// The space that is to be discretized
    const std::shared_ptr<Space> _space;

    /// How many cells to place per length unit of space
    /** \note The effective resolution might differ from this number, depending
      *       on the choice of resolution value and the physical extent of the
      *       space in each dimension.
      */
    const DistType _resolution;

    /// Neighborhood mode
    NBMode _nb_mode;

    /// Neighborhood function (working on cell IDs)
    NBFuncID<Self> _nb_func;

    // .. Neighborhood parameters .............................................
    // These are parameters that are required by some neighborhood functions

    /// A distance parameter; interpretation depends on chosen neighborhood
    DistType _nbh_distance;

    // NOTE When adding new members here, make sure to update the reset method!


public:
    // -- Constructors and Destructors ----------------------------------------
    /// Construct a grid discretization
    /** \param  space   The space to construct the discretization for
      * \param  cfg     Further configuration parameters
      */
    Grid (std::shared_ptr<Space> space, const DataIO::Config& cfg)
    :
        _space(space),
        _resolution([&cfg](){
            if (not cfg["resolution"]) {
                throw std::invalid_argument("Missing grid configuration "
                    "parameter 'resolution'! Please supply an integer >= 1.");
            }

            // Read in as signed int (allows throwing error for negative value)
            const auto res = as_<long long>(cfg["resolution"]);

            if (res < 1) {
                throw std::invalid_argument("Grid resolution needs to be a "
                                            "positive integer, was < 1!");
            }
            return res;
        }()),
        _nb_mode(NBMode::empty)
    {
        // Set the neighborhood function to not return anything
        _nb_func = _nb_empty;

        // Set the neighborhood parameters to their "empty" values
        reset_nbh_params();
    }

    /// Construct a grid discretization
    /** \param  space   The space to construct the discretization for; will be
      *                 stored as shared pointer
      * \param  cfg     Further configuration parameters
      */
    Grid (Space& space, const DataIO::Config& cfg)
    :
        Grid(std::make_shared<Space>(space), cfg)
    {}

    /// Virtual destructor to allow polymorphic destruction
    virtual ~Grid() = default;


    // -- Public interface ----------------------------------------------------
    // .. Neighborhood interface ..............................................

    /// Returns the indices of the neighbors of the cell with the given ID
    IndexContainer neighbors_of(const IndexType& id) const {
        return _nb_func(id);
    }

    void select_neighborhood(NBMode nb_mode,
                             const DataIO::Config& nb_params = {})
    {
        try {
            _nb_func = get_nb_func(nb_mode, nb_params);
        }
        catch (std::exception& e) {
            throw std::invalid_argument("Failed to select neighborhood: "
                                        + ((std::string) e.what()));
        }
        
        _nb_mode = nb_mode;
    }

    /// Const reference to the currently selected neighborhood mode
    const NBMode& nb_mode() {
        return _nb_mode;
    }


    // .. Position-related methods ............................................
    /// Returns the multi-index of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    virtual MultiIndex midx_of(const IndexType&) const = 0;

    /// Returns the barycenter of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    virtual SpaceVec barycenter_of(const IndexType&) const = 0;

    /// Returns the extent of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    virtual SpaceVec extent_of(const IndexType&) const = 0;

    /// Returns the vertices of the cell with the given ID
    /** \detail Consult the derived class implementation's documentation on
      *         the order of the vertices in the returned container.
      * \note   This method does not perform bounds checking of the given ID!
      */
    virtual std::vector<SpaceVec> vertices_of(const IndexType&) const = 0;

    /// Return the ID of the cell covering the given point in physical space
    /** \detail Cells are interpreted as covering half-open intervals in space,
      *         i.e., including their low-value edges and excluding their high-
      *         value edges.
      *         The special case of points on high-value edges for non-periodic
      *         space behaves such that these points are associated with the
      *         cells at the boundary.
      *
      * \note   This function always returns IDs of cells that are inside
      *         physical space. For non-periodic space, a check is performed
      *         whether the given point is inside the physical space
      *         associated with this grid. For periodic space, the given
      *         position is mapped back into the physical space.
      */
    virtual IndexType cell_at(const SpaceVec&) const = 0;

    /// Retrieve a set of cell indices that are at a specified boundary
    /** \note   For a periodic space, an empty container is returned; no error
      *         or warning is emitted.
      *
      * \param  select  Which boundary to return the cell IDs of. If 'all',
      *         all boundary cells are returned. Other available values depend
      *         on the dimensionality of the grid:
      *                1D:  left, right
      *                2D:  bottom, top
      *                3D:  back, front
      */
    virtual std::set<IndexType> boundary_cells(std::string={}) const = 0;


    // .. Getters .............................................................
    /// Get number of cells
    /** \detail This information is used by the CellManager to populate the
      *         cell container with the returned number of cells
      */
    virtual IndexType num_cells() const = 0;
    
    /// Get scalar resolution value of this grid
    auto resolution() const {
        return _resolution;
    }

    /// Returns the effective resolution into each dimension of the grid
    /** \detail Depending on choice of resolution and extent of physical space,
      *         the resolution given at initialization might not represent the
      *         density of cells per unit of space fully accurately.
      *         The effective resolution accounts for the scaling that was
      *         required to map an integer number of cells onto the space.
      */
    virtual SpaceVec effective_resolution() const = 0;

    /// Get the shape of the grid discretization
    virtual MultiIndex shape() const = 0;

    /// Const reference to the space this grid maps to
    const std::shared_ptr<Space>& space() const {
        return _space;
    }

    /// Whether the space this grid maps to is periodic
    bool is_periodic() const {
        return _space->periodic;
    }


protected:
    // -- Neighborhood functions ----------------------------------------------
    /// Retrieve the neighborhood function depending on the mode
    /** \detail The configuration node that is passed along can be used to
      *         specify the neighborhood parameter members.
      */
    virtual NBFuncID<Self> get_nb_func(NBMode, const DataIO::Config&) = 0;

    /// Resets all neighborhood parameters to their default / "empty" value
    void reset_nbh_params() {
        _nbh_distance = 0;
    }

    /// Function to use to set neighborhood parameters
    /** \detail Provides understandable error messages if a parameter is
      *         missing or the conversion failed.
      *
      * \param  nbh_params  The configuration node to read the parameters from
      * \param  keys        A pair that specifies the key and whether that key
      *                     is required.
      *
      * \note   Resets all other neighborhood parameters! Thus, this method's
      *         exceptions should only be caught if it is taken care that the
      *         neighborhood parameters are in a well-defined state for the
      *         continued use of the grid.
      */
    template<class opt_pair=std::pair<std::string, bool>>
    void set_nbh_params(const DataIO::Config& nbh_params,
                        const std::vector<opt_pair> keys)
    {
        // First, reset all parameters
        reset_nbh_params();

        // Now go over the desired keys and store them in the associated
        // member. If the key is required, an error will be thrown.
        for (const auto& [key, required] : keys) {
            try {
                if (key == "distance") {
                    try {
                        _nbh_distance = get_<DistType>("distance", nbh_params);
                    }
                    catch (...) {
                        if (required) throw;
                    }

                    // Needs to fit into the shape of the grid
                    if (_nbh_distance * 2 + 1 > this->shape().min()) {
                        throw std::invalid_argument("Grid shape is too small "
                            "to accomodate a neighborhood with parameter "
                            "'distance' set to"
                            + as_str(nbh_params["distance"]) + "!");
                    }
                }
                // ... can add other parameter assignments here
            }
            catch (std::exception& e) {
                throw std::invalid_argument("Could not set the required "
                    "neighborhood parameter '" + key + "': "
                    + ((std::string) e.what()));
            }
        }
    }


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
