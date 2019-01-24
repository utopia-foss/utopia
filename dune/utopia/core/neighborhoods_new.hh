#ifndef UTOPIA_CORE_NEIGHBORHOOD_HH
#define UTOPIA_CORE_NEIGHBORHOOD_HH

// NOTE This file will replace neighborhoods.hh

#include "types.hh"


namespace Utopia {
namespace Neighborhoods {

/// Type of the neighborhood calculating function
template<class Grid>
using NBFuncID = std::function<IndexContainer(const IndexType&, const Grid&)>;


// -- General helper functions -----------------------------------------------


// ---------------------------------------------------------------------------
/**
 *  \addtogroup Neighborhoods
 *  \{
 */

// TODO Categorize correctly in doxygen and write a few sentences here
// TODO Write about the required interface here
// TODO Ensure the returned index container is moved, not copied!
// TODO Consider using free functions (instead of lambdas)
// TODO Check if Space dimensionality can be used for constexpr statements


/// Always returns an empty neighborhood
template<class Grid>
NBFuncID<Grid> AllAlone = [](const IndexType&, const Grid&)
{
    IndexContainer idcs{};
    idcs.reserve(0);
    return idcs;
};

// ---------------------------------------------------------------------------
// -- Rectangular ------------------------------------------------------------
// ---------------------------------------------------------------------------

namespace Rectangular {

// TODO Consider moving the helper functions _into_ RectangularGrid; this would
//      require to have the interface determined by the base class and that
//      instead of template arguments + if constexpr multiple functions are
//      defined...

/// Return i-dimensional shift in cell indices, depending on grid shape
template<DimType shift_dim,
         class GridShape,
         class return_t=typename GridShape::value_type>
constexpr return_t id_shift_in_dim_ (const GridShape& shape) {
    if constexpr (shift_dim == 0) {
        return 1;
    }
    else {
        return shape[shift_dim-1] * id_shift_in_dim_<shift_dim-1>(shape);
    } 
}

/// Fill an index container with neighbors in different directions
/** This function takes an index container and populates it with the indices of
 *  neighboring cells in different dimensions, specified by template
 *  parameter `dim`.
 *  This only works on structured grids!
 * 
 *  The algorithm first calculates whether the given root cell index has a
 *  front or back boundary in the chosen dimension. If so, the neighboring cell
 *  is only added if the grid is periodic.
 * 
 * \param root_id      Which cell to find the agents of
 * \param neighbor_ids The container to populate with the indices
 * \param grid         The grid discretization to determine shape and
 *                     periodicity from
 * 
 * \tparam dim_no      The dimensions in which to add neighbors
 * \tparam Grid        The grid type to work on
 * 
 * \return void
 */
template<DimType dim, class Grid>
void add_neighbors_in_dim_ (const IndexType& root_id,
                            IndexContainer& neighbor_ids,
                            const Grid& grid)
{
    // Assure the number of dimesions is supported
    static_assert(Grid::dim >= 1 and Grid::dim <= 3,
                  "Unsupported dimensionality! Need be 1, 2, or 3.");

    // Gather the grid information needed
    const bool periodic = grid.is_periodic();
    const auto& shape = grid.shape();

    // Distinguish by dimension parameter
    if constexpr (dim == 1) {
        // check if at front boundary
        if (root_id % shape[0] == 0) {
            if (periodic) {
                neighbor_ids.push_back(root_id
                                       - id_shift_in_dim_<0>(shape)
                                       + id_shift_in_dim_<1>(shape));
            }
        }
        else {
            neighbor_ids.push_back(root_id - id_shift_in_dim_<0>(shape));
        }

        // check if at back boundary
        if (root_id % shape[0] == shape[0] - 1) {
            if (periodic) {
                neighbor_ids.push_back(root_id
                                       + id_shift_in_dim_<0>(shape)
                                       - id_shift_in_dim_<1>(shape));
            }
        }
        else {
            neighbor_ids.push_back(root_id + id_shift_in_dim_<0>(shape));
        }
    }


    else if constexpr (dim == 2) {
        // 'normalize' id to lowest height (if 3D)
        const auto root_id_nrm = root_id % id_shift_in_dim_<2>(shape);

        // check if at front boundary
        // TODO Check what the cast is needed for and why this is not the same as with the other dimensions
        if ((long) root_id_nrm / shape[0] == 0) {
            if (periodic) {
                neighbor_ids.push_back(root_id
                                       - id_shift_in_dim_<1>(shape)
                                       + id_shift_in_dim_<2>(shape));
            }
        }
        else {
            neighbor_ids.push_back(root_id - id_shift_in_dim_<1>(shape));
        }

        // check if at back boundary
        if ((long) root_id_nrm / shape[0] == shape[1] - 1) {
            if (periodic) {
                neighbor_ids.push_back(root_id
                                       + id_shift_in_dim_<1>(shape)
                                       - id_shift_in_dim_<2>(shape));
            }
        }
        else {
            neighbor_ids.push_back(root_id + id_shift_in_dim_<1>(shape));
        }
    }


    else if constexpr (dim == 3) {
        const auto id_max = id_shift_in_dim_<3>(shape) - 1;

        // check if at front boundary
        if (root_id - id_shift_in_dim_<2>(shape) < 0) {
            if (periodic) {
                neighbor_ids.push_back(root_id
                                       - id_shift_in_dim_<2>(shape)
                                       + id_shift_in_dim_<3>(shape));
            }
        }
        else {
            neighbor_ids.push_back(root_id - id_shift_in_dim_<2>(shape));
        }

        // check if at back boundary
        if (root_id + id_shift_in_dim_<2>(shape) > id_max) {
            if (periodic) {
                neighbor_ids.push_back(root_id
                                       + id_shift_in_dim_<2>(shape)
                                       - id_shift_in_dim_<3>(shape));
            }
        }
        else {
            neighbor_ids.push_back(root_id + id_shift_in_dim_<2>(shape));
        }
    }
}


template<class Grid>
NBFuncID<Grid> Nearest = [](const IndexType& root_id, const Grid& grid)
{
    // Instantiate container in which to store the neighboring cell IDs
    IndexContainer neighbor_ids;

    // The number if neighbors is known; pre-allocating space brings a speed
    // improvemet of about factor 2
    neighbor_ids.reserve(2 * Grid::dim);

    // Depending on the number of dimensions, add the IDs of neighboring cells
    // in those dimensions
    add_neighbors_in_dim_<1>(root_id, neighbor_ids, grid);

    if constexpr (Grid::dim >= 2) {
        add_neighbors_in_dim_<2>(root_id, neighbor_ids, grid);
    }

    if constexpr (Grid::dim >= 3) {
        add_neighbors_in_dim_<3>(root_id, neighbor_ids, grid);
    }
    // ... _could_ continue here

    // Return the container of cell indices
    return neighbor_ids;
};
    
} // namespace Rectangular

// ---------------------------------------------------------------------------
// -- Hexagonal --------------------------------------------------------------
// ---------------------------------------------------------------------------

namespace Hexagonal {

} // namespace Hexagonal

// ---------------------------------------------------------------------------
// -- Triangular -------------------------------------------------------------
// ---------------------------------------------------------------------------

namespace Triangular {

} // namespace Triangular

// end group Neighborhoods
/**
 *  \}
 */

} // namespace Neighborhoods
} // namespace Utopia

#endif // UTOPIA_CORE_NEIGHBORHOOD_HH
