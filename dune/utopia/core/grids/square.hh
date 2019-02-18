#ifndef UTOPIA_CORE_GRIDS_SQUARE_HH
#define UTOPIA_CORE_GRIDS_SQUARE_HH

#include <cmath>
#include <sstream>

#include "base.hh"

namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

/// A grid discretization using square cells
/** \detail This is a grid discretization where the cells are vector spaces
  *         that are spanned by orthogonal basis vectors and each cell has the
  *         same physical extent in each dimension. In the 2D case, this refers
  *         to perfectly square cells; in 3D these would be perfect cubes, etc.
  */
template<class Space>
class SquareGrid
    : public Grid<Space>
{
public:
    /// Base class type
    using Base = Grid<Space>;

    /// The dimensionality of the space to be discretized (for easier access)
    static constexpr DimType dim = Space::dim;

    /// The type of vectors that have a relation to physical space
    using SpaceVec = typename Space::SpaceVec;

    /// The type of multi-index like arrays, e.g. the grid shape
    using MultiIndex = MultiIndexType<dim>;


private:
    // -- SquareGrid-specific members -----------------------------------------
    /// The (multi-index) shape of the grid, resulting from resolution
    const MultiIndex _shape;

    /// The extent of each cell of this square discretization (same for all)
    const SpaceVec _cell_extent;


public:
    // -- Constructors --------------------------------------------------------
    /// Construct a rectangular grid discretization
    /** \param  space   The space to construct the discretization for
      * \param  cfg     Further configuration parameters
      */
    SquareGrid (std::shared_ptr<Space> space, const DataIO::Config& cfg)
    :
        Base(space, cfg),
        _shape(determine_shape()),
        _cell_extent(1./effective_resolution())
    {
        if constexpr (dim > 1) {
            // Make sure the cells really are square
            const auto eff_res = effective_resolution();

            if (eff_res.min() != eff_res.max()) {
                // Format effective resolution to show it in error message
                std::stringstream efr_ss;
                efr_ss << eff_res;

                throw std::invalid_argument("Given the extent of the physical "
                    "space and the specified resolution, a mapping with "
                    "exactly square cells could not be found! Either adjust "
                    "the physical space, the resolution of the grid, or "
                    "choose another grid. Effective resolution was:\n"
                    + efr_ss.str() + ", but should be the same in all "
                    "dimensions!");
            }
        }
    }
    
    /// Construct a rectangular grid discretization
    /** \param  space   The space to construct the discretization for; will be
      *                 stored as shared pointer
      * \param  cfg     Further configuration parameters
      */
    SquareGrid (Space& space, const DataIO::Config& cfg)
    :
        SquareGrid(std::make_shared<Space>(space), cfg)
    {}


    // -- Implementations of virtual base class functions ---------------------
    // .. Number of cells & shape .............................................

    /// Number of square cells required to fill the physical space
    /** \detail Is calculated simply from the _shape member
      */
    IndexType num_cells() const override {
        return std::accumulate(this->_shape.begin(), this->_shape.end(),
                               1, std::multiplies<IndexType>());
    };

    /// The effective cell resolution into each physical space dimension
    /** \detail For a square lattice, this is just the quotient of grid shape
      *         and extent of physical space, separately in each dimension
      */
    const SpaceVec effective_resolution() const override {
        // Use element-wise division by the physical extent (double)
        return _shape / this->_space->extent;
    }

    /// Get shape of the square grid
    const MultiIndex shape() const override {
        // Can just return the calculated member here
        return _shape;
    }


    // .. Position-related methods ............................................
    /// Returns the multi-index of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    const MultiIndex midx_of(const IndexType& id) const override {
        static_assert(dim <= 2, "MultiIndex only implemented for 1D and 2D!");

        if constexpr (dim == 1) {
            return MultiIndex({id % _shape[0]});
        }
        else {
            return MultiIndex({id % _shape[0],
                               id / _shape[0]});
        }
    }

    /// Returns the barycenter of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    const SpaceVec barycenter_of(const IndexType& id) const override {
        // Offset on grid + offset within cell
        return (midx_of(id) % _cell_extent) + (_cell_extent/2.);
        // NOTE The %-operator performs element-wise multiplication
    }

    /// Returns the extent of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    const SpaceVec extent_of(const IndexType&) const override {
        return _cell_extent;
    }

    /// Returns the vertices of the cell with the given ID
    /** \detail Only available for 2D currently; the vertices are given in
      *         counter-clockwise order, starting with the position of the
      *         bottom left-hand vertex of the cell.
      * \note   This method does not perform bounds checking of the given ID!
      */
    const std::vector<SpaceVec>
        vertices_of(const IndexType& id) const override
    {
        static_assert(dim == 2,
                      "SquareGrid::vertices_of is only implemented for 2D!");

        std::vector<SpaceVec> vertices{};
        vertices.reserve(4);

        // NOTE The %-operator performs element-wise multiplication
        // Counter-clockwise, starting bottom left ...
        vertices.push_back(midx_of(id) % _cell_extent);
        vertices.push_back(vertices[0] + _cell_extent % SpaceVec({1., 0.}));
        vertices.push_back(vertices[0] + _cell_extent);
        vertices.push_back(vertices[0] + _cell_extent % SpaceVec({0., 1.}));

        return vertices;
    }

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
    IndexType cell_at(const SpaceVec& pos) const override {
        static_assert(dim == 2,
                      "SquareGrid::cell_at only implemented for 2D!");
        
        // The multi-index element type to use for static casts
        using midx_et = typename MultiIndex::elem_type;

        // The multi-index to be calculated
        MultiIndex midx;

        // Distinguish periodic and non-periodic case
        // NOTE While there is some duplicate code, this is the configuration
        //      with the least amount of unnecessary / duplicate value checks.
        if (this->is_periodic()) {
            // Calculate the real-valued position in units of cell extents,
            // using the position mapped back into space. That function takes
            // care to map the high-value boundary to the low-value boundary.
            const SpaceVec ridx = (  this->_space->map_into_space(pos)
                                   / _cell_extent);

            // Can now calculate the integer multi index, rounding down
            midx = {static_cast<midx_et>(ridx[0]),
                    static_cast<midx_et>(ridx[1])};
        }
        else {
            // Make sure the given coordinate is actually within the space
            if (not this->_space->contains(pos)) {
                throw std::invalid_argument("The given position is outside "
                    "the non-periodic space associated with this grid!");
            }

            // Calculate the real-valued position in units of cell extents
            const SpaceVec ridx = pos / _cell_extent;

            // Calculate the integer multi index, rounding down
            midx = {static_cast<midx_et>(ridx[0]),
                    static_cast<midx_et>(ridx[1])};

            // Associate points on high-value boundaries with boundary cells
            if (midx[0] == _shape[0]) {
                midx[0]--;
            }
            if (midx[1] == _shape[1]) {
                midx[1]--;
            }
            // Note that with _shape having only elements >= 1, the decrement
            // does not cause any issues.
        }

        // From the multi index, calculate the corresponding ID
        return midx[0] + (midx[1] * _shape[0]);
        // Equivalent to:
        //     midx[0] * id_shift_in_dim_<0>()
        //   + midx[1] * id_shift_in_dim_<1>()
    }

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
    const std::set<IndexType>
        boundary_cells(std::string select="all") const override
    {
        static_assert(dim <= 2,
            "SquareGrid::boundary_cells only implemented for 1D and 2D!");

        // For periodic space, this is easy:
        if (this->is_periodic()) {
            return {};
        }
        // else: non-periodic space

        // The target set all IDs are to be emplaced in
        std::set<IndexType> bc_ids;

        // Distinguish by dimensionality of the space
        if constexpr (dim == 1) {
            if (select != "all" and select != "left" and select != "right") {
                throw std::invalid_argument("Invalid value for argument "
                    "`select` in call to method SquareGrid::boundary_cells! "
                    "Available arguments (for currently selected "
                    "dimensionality) are: "
                    "'all', 'left', 'right'. Given value: '" + select + "'");
            }

            // Left boundary (consists only of one cell)
            if (select == "all" or select == "left") {
                bc_ids.emplace(0);
            }

            // Right boundary (also consists only of one cell)
            if (select == "all" or select == "right") {
                bc_ids.emplace_hint(bc_ids.end(), _shape[0] - 1);
            }
        }
        else if constexpr (dim == 2) {
            if (    select != "all"
                and select != "left" and select != "right"
                and select != "bottom" and select != "top")
            {
                throw std::invalid_argument("Invalid value for argument "
                    "`select` in call to method SquareGrid::boundary_cells! "
                    "Available arguments (for currently selected "
                    "dimensionality) are: "
                    "'all', 'left', 'right', 'bottom', 'top'. Given value: '"
                    + select + "'");
            }

            // NOTE It is important to use the hinting features of std::set
            //      here, which allow to run the following in amortized
            //      constant time instead of logarithmic with set size.
            //      Below, it always makes sense to hint at inserting right
            //      before the end.
            // Hint for the first element needs to be the beginning
            auto hint = bc_ids.begin();

            // Bottom boundary (lowest IDs)
            if (select == "all" or select == "bottom") {
                // 0, ..., _shape[0] - 1    
                for (std::size_t id = 0; id < _shape[0]; id++) {
                    bc_ids.emplace_hint(hint, id);
                    hint = bc_ids.end();
                }
            }

            // Left boundary
            if (select == "left") {
                // First IDs in _shape[1] rows:  0, _shape[0], 2*_shape[0], ...
                for (std::size_t row = 0; row < _shape[1]; row++) {
                    bc_ids.emplace_hint(hint, row * _shape[0]);
                    hint = bc_ids.end();
                }
            }

            // Right boundary
            if (select == "right") {
                // Last IDs in _shape[1] rows
                const auto offset = _shape[0] - 1;

                for (std::size_t row = 0; row < _shape[1]; row++) {
                    bc_ids.emplace_hint(hint, offset + row * _shape[0]);
                    hint = bc_ids.end();
                }
            }

            // Left AND right (only for 'all' case, allows better hints)
            if (select == "all") {
                // First and last IDs in _shape[1] rows
                const auto offset = _shape[0] - 1;

                for (std::size_t row = 0; row < _shape[1]; row++) {
                    // Left boundary cell
                    bc_ids.emplace_hint(hint, row * _shape[0]);

                    // Right boundary cell (higher than left cell ID)
                    bc_ids.emplace_hint(bc_ids.end(),
                                        offset + row * _shape[0]);

                    hint = bc_ids.end();
                }   
            }

            // Top boundary (highest IDs)
            if (select == "all" or select == "top") {
                // _shape[0] * (_shape[1]-1), ..., _shape[0] * _shape[1] - 1
                for (std::size_t id = _shape[0] * (_shape[1]-1);
                     id < _shape[0] * _shape[1]; id++)
                {
                    bc_ids.emplace_hint(hint, id);
                    hint = bc_ids.end();
                }
            }
        }

        return bc_ids;
    }


private:
    // -- Helper functions ----------------------------------------------------
    /// Given the resolution, return the grid shape required to fill the space
    /** \detail Integer rounding takes place here. A physical space of extents
      *         of 2.1 length units in each dimension and a resolution of two
      *         cells per unit length will result in 4 cells in each dimension,
      *         each cell's size scaled up slightly and the effective
      *         effective resolution thus slightly smaller than the specified
      *         resolution.
      */
    MultiIndex determine_shape() const {
        MultiIndex shape;

        for (DimType i = 0; i < dim; i++) {
            shape[i] = this->_space->extent[i] * this->_resolution;
        }

        return shape;
    }


protected:
    // -- Neighborhood interface ----------------------------------------------
    /// Retrieve the neighborhood function depending on the mode
    NBFuncID<Base> get_nb_func(NBMode nb_mode) const override {
        if (nb_mode == NBMode::empty) {
            return this->_nb_empty;
        }
        else if (nb_mode == NBMode::vonNeumann) {
            if (this->is_periodic()) {
                return _nb_vonNeumann_periodic;
            }
            else {
                return _nb_vonNeumann_nonperiodic;
            }
        }
        else if (nb_mode == NBMode::Moore) {
            if (this->is_periodic()) {
                return _nb_Moore_periodic;
            }
            else {
                return _nb_Moore_nonperiodic;
            }
        }
        else {
            throw std::invalid_argument("No '" + nb_mode_to_string(nb_mode)
                + "' available for rectangular grid discretization!");
        }
    }


    // .. Neighborhood implementations ........................................
    // NOTE With C++20, the below lambdas would allow template arguments

    /// The Von-Neumann neighborhood for periodic grids
    NBFuncID<Base> _nb_vonNeumann_periodic = [this](const IndexType& root_id){
        static_assert((dim >= 1 and dim <= 3),
            "VonNeumann neighborhood is only implemented in 1-3 dimensions!");

        // Instantiate container in which to store the neighboring cell IDs
        IndexContainer neighbor_ids{};

        // The number of neighbors is known; pre-allocating space brings a
        // speed improvemet of about factor 2
        neighbor_ids.reserve(2 * dim);

        // Depending on the number of dimensions, add the IDs of neighboring
        // cells in those dimensions
        add_neighbors_in_dim_<1, true>(root_id, neighbor_ids);

        if constexpr (dim >= 2) {
            add_neighbors_in_dim_<2, true>(root_id, neighbor_ids);
        }

        if constexpr (dim >= 3) {
            add_neighbors_in_dim_<3, true>(root_id, neighbor_ids);
        }

        // Return the container of cell indices
        return neighbor_ids;
    };

    /// The Von-Neumann neighborhood for non-periodic grids
    NBFuncID<Base> _nb_vonNeumann_nonperiodic = [this](const IndexType& root_id){
        static_assert((dim >= 1 and dim <= 3),
            "VonNeumann neighborhood is only implemented in 1-3 dimensions!");

        // Instantiate container in which to store the neighboring cell IDs
        IndexContainer neighbor_ids{};

        // The number of neighbors is known; pre-allocating space brings a
        // speed improvemet of about factor 2
        neighbor_ids.reserve(2 * dim);

        // Depending on the number of dimensions, add the IDs of neighboring
        // cells in those dimensions
        add_neighbors_in_dim_<1, false>(root_id, neighbor_ids);

        if constexpr (dim >= 2) {
            add_neighbors_in_dim_<2, false>(root_id, neighbor_ids);
        }
        
        if constexpr (dim >= 3) {
            add_neighbors_in_dim_<3, false>(root_id, neighbor_ids);
        }

        // Return the container of cell indices
        return neighbor_ids;
    };


    /// Moore neighbors for periodic 2D grid
    NBFuncID<Base> _nb_Moore_periodic = [this](const IndexType& root_id){
        static_assert(dim == 2, "Moore neighborhood only available in 2D!");

        // Generate vector in which to store the neighbors and allocate space
        IndexContainer neighbor_ids{};
        neighbor_ids.reserve(8);

        // Get the neighbors in the second dimension
        add_neighbors_in_dim_<2, true>(root_id, neighbor_ids);
        // ...have these neighbors at indices 0 and 1 now.

        // For these neighbors, add _their_ neighbors in the first dimension
        add_neighbors_in_dim_<1, true>(neighbor_ids[0], neighbor_ids);
        add_neighbors_in_dim_<1, true>(neighbor_ids[1], neighbor_ids);

        // And finally, add the root cell's neighbors in the first dimension
        add_neighbors_in_dim_<1, true>(root_id, neighbor_ids);

        return neighbor_ids;
    };

    /// Moore neighbors for non-periodic 2D grid
    NBFuncID<Base> _nb_Moore_nonperiodic = [this](const IndexType& root_id){
        static_assert(dim == 2, "Moore neighborhood only available in 2D!");

        // Generate vector in which to store the neighbors and allocate space
        IndexContainer neighbor_ids{};
        neighbor_ids.reserve(8);

        // Get the neighbors in the second dimension
        add_neighbors_in_dim_<2, false>(root_id, neighbor_ids);
        // root not at border: have them at indices 0 and 1 now
        // root at border: less than two neighbors were added

        // Distinguish these two cases
        if (neighbor_ids.size() == 2) {
            // Was not at a boundary.
            add_neighbors_in_dim_<1, false>(neighbor_ids[0], neighbor_ids);
            add_neighbors_in_dim_<1, false>(neighbor_ids[1], neighbor_ids);
        }
        else if (neighbor_ids.size() == 1) {
            // Was at a front XOR back boundary in dimension 2
            add_neighbors_in_dim_<1, false>(neighbor_ids[0], neighbor_ids);
        }
        // else: was at front AND back boundary (single row of cells in dim 2)

        // Finally, add the root's neighbors in the first dimension
        add_neighbors_in_dim_<1, false>(root_id, neighbor_ids);

        return neighbor_ids;
    };


    // .. Neighborhood helper functions .......................................

    /// Return i-dimensional shift in cell indices, depending on grid shape
    template<DimType shift_dim>
    constexpr typename MultiIndexType<dim>::value_type id_shift_in_dim_ () {
        if constexpr (shift_dim == 0) {
            return 1;
        }
        else {
            return this->_shape[shift_dim-1] * id_shift_in_dim_<shift_dim-1>();
        } 
    }

    /// Fill an index container with neighbors in different directions
    /** This function takes an index container and populates it with the
     *  indices of neighboring cells in different dimensions, specified by
     *  template parameter `dim`.
     * 
     *  The algorithm first calculates whether the given root cell index has a
     *  front or back boundary in the chosen dimension. If so, the neighboring
     *  cell is only added if the grid is periodic.
     * 
     * \param root_id      Which cell to find the agents of
     * \param neighbor_ids The container to populate with the indices
     * 
     * \tparam dim_no      The dimensions in which to add neighbors
     * \tparam periodic    Whether the grid is periodic
     * 
     * \return void
     */
    template<DimType dim, bool periodic>
    void add_neighbors_in_dim_ (const IndexType& root_id,
                                IndexContainer& neighbor_ids)
    {
        // Assure the number of dimesions is supported
        static_assert(dim >= 1 and dim <= 3,
                      "Unsupported dimensionality! Need be 1, 2, or 3.");

        // Gather the required grid information
        const auto& shape = this->shape();

        // Distinguish by dimension parameter
        if constexpr (dim == 1) {
            // check if at front boundary
            if (root_id % shape[0] == 0) {
                if constexpr (periodic) {
                    neighbor_ids.push_back(root_id
                                           - id_shift_in_dim_<0>()
                                           + id_shift_in_dim_<1>());
                }
            }
            else {
                neighbor_ids.push_back(root_id - id_shift_in_dim_<0>());
            }

            // check if at back boundary
            if (root_id % shape[0] == shape[0] - 1) {
                if constexpr (periodic) {
                    neighbor_ids.push_back(root_id
                                           + id_shift_in_dim_<0>()
                                           - id_shift_in_dim_<1>());
                }
            }
            else {
                neighbor_ids.push_back(root_id + id_shift_in_dim_<0>());
            }
        }


        else if constexpr (dim == 2) {
            // 'normalize' id to lowest height (if 3D)
            const auto root_id_nrm = root_id % id_shift_in_dim_<2>();

            // check if at front boundary
            // TODO Check what the cast is needed for and why this is not the
            //      same as with the other dimensions
            if ((long) root_id_nrm / shape[0] == 0) {
                if constexpr (periodic) {
                    neighbor_ids.push_back(root_id
                                           - id_shift_in_dim_<1>()
                                           + id_shift_in_dim_<2>());
                }
            }
            else {
                neighbor_ids.push_back(root_id - id_shift_in_dim_<1>());
            }

            // check if at back boundary
            if ((long) root_id_nrm / shape[0] == shape[1] - 1) {
                if constexpr (periodic) {
                    neighbor_ids.push_back(root_id
                                           + id_shift_in_dim_<1>()
                                           - id_shift_in_dim_<2>());
                }
            }
            else {
                neighbor_ids.push_back(root_id + id_shift_in_dim_<1>());
            }
        }


        else if constexpr (dim == 3) {
            const auto id_max = id_shift_in_dim_<3>() - 1;

            // check if at front boundary
            if (root_id - id_shift_in_dim_<2>() < 0) {
                if constexpr (periodic) {
                    neighbor_ids.push_back(root_id
                                           - id_shift_in_dim_<2>()
                                           + id_shift_in_dim_<3>());
                }
            }
            else {
                neighbor_ids.push_back(root_id - id_shift_in_dim_<2>());
            }

            // check if at back boundary
            if (root_id + id_shift_in_dim_<2>() > id_max) {
                if constexpr (periodic) {
                    neighbor_ids.push_back(root_id
                                           + id_shift_in_dim_<2>()
                                           - id_shift_in_dim_<3>());
                }
            }
            else {
                neighbor_ids.push_back(root_id + id_shift_in_dim_<2>());
            }
        }
    }

};

// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRIDS_SQUARE_HH
