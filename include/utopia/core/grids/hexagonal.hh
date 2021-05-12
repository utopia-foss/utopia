#ifndef UTOPIA_CORE_GRIDS_HEXAGONAL_HH
#define UTOPIA_CORE_GRIDS_HEXAGONAL_HH

#include "base.hh"

namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

/// A grid discretization using hexagonal cells
/** This is a grid discretization using hexagonal cells
 * 
 *  The hexagonal tiling is constructed in pointy-topped orientation (one vertex
 *  of the hexagon pointing up) with an even row offset (every even row is 
 *  displaced half a cell width to the right).
 * 
 * The hexagonal cellular arrangement covers a periodic space perfectly. In
 * this setup the right-most cell in offset rows is half cut, covering the
 * remaining space at the left boundary. Similarly, the tip of the cells in the
 * upper-most row covers the free space at the lower boundary. In non-periodic 
 * space these cells are cut and at the opposite boundary space remains
 * under-represented.
 *      -# Top: pointy tops are cut off
 *      -# Right: offset row cells are cut in half
 *      -# Bottom: no cutoff, but unrepresented area the same shape as the
 *                 top-side cutoff. Points within these triangles are mapped to 
 *                 nearest cell
 *      -# Left: no cutoff, but unrepresented area the same shape as the 
 *               right-side cutoff. Points within these half-cells are mapped 
 *               to the nearest cell.
 * 
 * \note Indices are constructed in "Fortran"-style, i.e. with the first index
 *       changing fastest (also called "column major").
 *       For example, when iterating over the cell IDs and having a 2D space,
 *       the iteration goes first along the x-axis and then along the y-axis:
 *       ``0, 1, …, N_x, N_x + 1, …, 2*N_x, 2*N_x + 1, …, N_x * N_y - 1``
 * 
 * For more information on hexagonal grids, see this
 * <a href="https://www.redblobgames.com/grids/hexagons/">tutorial</a>.
 * 
 * This class has been implemented following the above tutorial written by
 * Amit Patel and published on www.redblobgames.com
 * (last accessed version from May 2020).
 */
template<class Space>
class HexagonalGrid
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

    /// The configuration type
    using Config = DataIO::Config;


private:
    // -- HexagonalGrid-specific members --------------------------------------
    /// The (multi-index) shape of the grid, resulting from resolution
    const MultiIndex _shape;

    /// The extent of each cell of this discretization (same for all)
    /** The cell's extent corresponds to the tip-to-tip distance
      */
    const SpaceVec _cell_extent;

public:
    // -- Constructors --------------------------------------------------------
    /// Construct a hexagonal grid discretization
    /** \param  space   The space to construct the discretization for
      * \param  cfg     Further configuration parameters
      */
    HexagonalGrid (std::shared_ptr<Space> space, const Config& cfg)
    :
        Base(space, cfg),
        _shape(determine_shape()),
        _cell_extent(1./effective_resolution())
    {
        // Make sure the cells really are hexagonal
        static_assert(dim == 2, "Hexagonal grid is only implemented for 2D "
                                "space!");
                                
        const SpaceVec eff_res = effective_resolution();

        // check that the cell extent corresponds to that of pointy-topped 
        // hexagonal cells
        // NOTE the test only requires the aspect ratio is precise to 2% for 
        //      easy user experience. The hexagonal grid requires a space
        //      with an aspect ratio involving a sqrt(3)
        if (  fabs(_cell_extent[0] / _cell_extent[1] - sqrt(3.) / 2.)
            > get_as<double>("aspect_ratio_tolerance", cfg, 0.02))
        {
            SpaceVec required_space = SpaceVec({1., 0.75 * 2. / sqrt(3.)});
            required_space[0] = this->_space->extent[0];
            required_space[1] = _shape[1] * _cell_extent[0] * 1.5 /sqrt(3.);
            std::stringstream required_space_ss;
            required_space_ss << required_space;

            throw std::invalid_argument(fmt::format(
                "Given the extent of the physical space and the specified "
                "resolution, a mapping with hexagonal cells could not be "
                "found! Either adjust the the extent of physical space or the "
                "resolution of the grid. Alternatively increase the tolerance "
                "to distorted hexagons or choose another grid. \n"
                "The required aspect ratio of sqrt(3) / 2 is violated by {} "
                "(> `aspect_ratio_tolerance` = {})! \n"
                "With the given resolution, set the space extent to : \n"
                + required_space_ss.str() +
                "or increase the resolution.",
                fabs(_cell_extent[0] / _cell_extent[1] - sqrt(3.) / 2.),
                get_as<double>("aspect_ratio_tolerance", cfg, 0.02)));
        }
    }


    // -- Implementations of virtual base class functions ---------------------
    // .. Number of cells & shape .............................................

    /// Number of hexagonal cells required to fill the physical space
    /** This is calculated simply from the _shape member.
      */
    IndexType num_cells() const override {
        return _shape[0] * _shape[1];
    }

    /// The effective cell resolution into each physical space dimension
    SpaceVec effective_resolution() const override {
        return SpaceVec({static_cast<double>(_shape[0]), 0.75*_shape[1]})
                / this->_space->extent;
    }

    /// Get shape of the hexagonal grid
    MultiIndex shape() const override {
        return _shape;
    }

    /// Structure of the grid
    GridStructure structure() const override {
        return GridStructure::hexagonal;
    }


    // .. Position-related methods ............................................
    /// Returns the multi-index of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    MultiIndex midx_of(const IndexType id) const override {
        return MultiIndex({id % _shape[0], id / _shape[0]});
    }

    /// Returns the barycenter of the cell with the given ID
    /** \note This method does not perform bounds checking of the given ID!
      */
    SpaceVec barycenter_of(const IndexType id) const override {
        MultiIndex mid = midx_of(id);
        if (mid[1] % 2 == 0) {
            // even row
            return    (mid % SpaceVec({1., 0.75}) + SpaceVec({1., 0.5}))
                    % _cell_extent;
        }
        else {
            // odd row
            return (  (mid % SpaceVec({1, 0.75}) + SpaceVec({0.5, 0.5}))
                    % _cell_extent);
        }
    }

    /// Returns the extent of the cell with the given ID
    /** The cell's extent corresponds to the tip-to-tip distance of the cell
      * 
      * \note This method does not perform bounds checking of the given ID!
      */
    SpaceVec extent_of(const IndexType) const override {
        return _cell_extent;
    }

    /// Returns the vertices of the cell with the given ID
    /** The vertices (pointy-topped arrangement) are given in counter-clockwise
      * order, starting with the position of the bottom left-hand vertex 
      * (8 o'clock) of the cell.
      *
      * \note   This method does not perform bounds checking of the given ID!
      */
    std::vector<SpaceVec> vertices_of(const IndexType id) const override {
        std::vector<SpaceVec> vertices{};
        vertices.reserve(6);

        const SpaceVec center = barycenter_of(id);

        vertices.push_back(center + SpaceVec({-0.5, -0.25}) % _cell_extent);
        vertices.push_back(center + SpaceVec({ 0. , -0.5 }) % _cell_extent);
        vertices.push_back(center + SpaceVec({ 0.5, -0.25}) % _cell_extent);
        vertices.push_back(center + SpaceVec({ 0.5,  0.25}) % _cell_extent);
        vertices.push_back(center + SpaceVec({ 0. ,  0.5 }) % _cell_extent);
        vertices.push_back(center + SpaceVec({-0.5,  0.25}) % _cell_extent);

        return vertices;
    }

    /// Return the ID of the cell covering the given point in physical space
    /** Cells are interpreted as covering half-open intervals in space, i.e.,
      * including their low-value edges and excluding their high-value edges.
      * The special case of points on high-value edges for non-periodic space
      * behaves such that these points are associated with the cells at the
      * boundary.
      * 
      * For non-periodic space only:
      * The offset geometry of the hexagonal lattice does not permit to cover
      * a rectangular non-periodic space perfectly.
      * At the boundary, a position in uncovered space is mapped to the closest
      * cell.
      * Details:
      * The hexagonal cellular arrangement is covering the periodic 
      * equivalent perfectly. In this setup the right-most cell in offset rows 
      * is half cut, covering the space at the left boundary. Similarly, the 
      * tip of the cells in the upper-most row covers the free space at the 
      * lower boundary. In non-periodic space these cells are cut and at the 
      * opposite boundary space remains under-represented. Positions in this 
      * space are mapped to the closest cell.
      *      -# Top: pointy tops are cut off
      *      -# Right: offset row cells are cut in half
      *      -# Bottom: no cutoff, but unrepresented area the same shape as the
      *                 top-side cutoff. Points within these triangles are 
      *                 mapped to nearest cell
      *      -# Left: no cutoff, but unrepresented area the same shape as the 
      *               right-side cutoff. Points within these half-cells are 
      *               mapped to the nearest cell.
      *
      * \note   This function always returns IDs of cells that are inside
      *         physical space. For non-periodic space, a check is performed
      *         whether the given point is inside the physical space
      *         associated with this grid. For periodic space, the given
      *         position is mapped back into the physical space.
      */
    IndexType cell_at(const SpaceVec& pos) const override {
        SpaceVec ridx = pos;
        // check position
        if (this->is_periodic()) {
            ridx = this->_space->map_into_space(pos);
        }
        else if (not this->_space->contains(pos)) {
            throw std::invalid_argument("The given position is outside "
                "the non-periodic space associated with this grid!");
        }

        // relative and centered in cell 0
        ridx = ridx / _cell_extent - SpaceVec({1., 0.5});

        arma::Col<int>::fixed<dim> midx = {
            static_cast<int>(std::round(ridx[0] + 2. / 3. * ridx[1])),
            static_cast<int>(std::round(4. / 3. * ridx[1]))
        };

        // correct for offset
        midx[0] -= std::floor(midx[1] / 2.);

        if (not this->is_periodic()) {
            // remap not represented space to first cell in this row
            // NOTE this distorts the cells, but the rectangular space is 
            //      correctly represented
            if (midx[0] == -1) {
                midx[0]++;
            }
            if (midx[1] == -1) {
                midx[1]++;
            }

            // Associate points on high-value boundaries with boundary cells
            if (midx[0] == static_cast<int>(_shape[0])) {
                midx[0]--;
            }
        }
        else {
            midx[0] = (midx[0] + _shape[0]) % _shape[0];
            midx[1] = (midx[1] + _shape[1]) % _shape[1];
        }

        // From the multi index, calculate the corresponding ID
        return static_cast<IndexType>(midx[0] + (midx[1] * _shape[0]));
        // Equivalent to:
        //     midx[0] * id_shift<0>()
        //   + midx[1] * id_shift<1>()
    }

    /// Retrieve a set of cell indices that are at a specified boundary
    /** \note   For a periodic space, an empty container is returned; no error
      *         or warning is emitted.
      *
      * \param  select  Which boundary to return the cell IDs of. If 'all',
      *         all boundary cells are returned. Other available values depend
      *         on the dimensionality of the grid:
      *                2D:  left, right, bottom, top
      */
    std::set<IndexType> boundary_cells(std::string select="all") const override
    {
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

        // The target set all IDs are to be emplaced in
        std::set<IndexType> bc_ids;
        
        // For periodic space, this is easy:
        if (this->is_periodic()) {
            return {};
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
            for (DistType id = 0; id < _shape[0]; id++) {
                bc_ids.emplace_hint(hint, id);
                hint = bc_ids.end();
            }
        }

        // Left boundary
        if (select == "left") {
            // First IDs in _shape[1] rows:  0, _shape[0], 2*_shape[0], ...
            for (DistType row = 0; row < _shape[1]; row++) {
                bc_ids.emplace_hint(hint, row * _shape[0]);
                hint = bc_ids.end();
            }
        }

        // Right boundary
        if (select == "right") {
            // Last IDs in _shape[1] rows
            const auto offset = _shape[0] - 1;

            for (DistType row = 0; row < _shape[1]; row++) {
                bc_ids.emplace_hint(hint, offset + row * _shape[0]);
                hint = bc_ids.end();
            }
        }

        // Left AND right (only for 'all' case, allows better hints)
        if (select == "all") {
            // First and last IDs in _shape[1] rows
            const auto offset = _shape[0] - 1;

            for (DistType row = 0; row < _shape[1]; row++) {
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
            for (DistType id = _shape[0] * (_shape[1]-1);
                    id < _shape[0] * _shape[1]; id++)
            {
                bc_ids.emplace_hint(hint, id);
                hint = bc_ids.end();
            }
        }

        return bc_ids;
    }


private:
    // -- Helper functions ----------------------------------------------------
    /// Get shape of the hexagonal grid
    /** Integer rounding takes place here. A physical space of extents of 2.1
      * length units in each dimension (resp. the shape of the hexagon) and a 
      * resolution of two cells per unit length will result in 4 cells in each 
      * dimension, each cell's size scaled up slightly and the effective 
      * resolution thus slightly smaller than the specified resolution.
      * 
      * \note In non-periodic space the the number of rows needs
      *       to be a pair value in pointy-top orientation.
      * \note Note the offset rows are cut and the right end of the domain
              (if not periodic). Therefore the width of the cells is such, that
              N * width = Length, that is non-offset rows span the entire width.
      */
    MultiIndex determine_shape() const {
        MultiIndex shape;
        
        // obtain the sidelength of a unit area hexagon
        // A = 3^1.5 / 2 s^2
        double s = sqrt(2. / (3 * sqrt(3)));
        double width = sqrt(3) * s;
        double height = 2 * s;

        shape[0] = std::round(  this->_space->extent[0] / width
                              * this->_resolution);
        shape[1] = std::round(  this->_space->extent[1] / (0.75 * height)
                              * this->_resolution);
        
        // in non periodic space pair number of rows required
        if (this->is_periodic()) {
            shape[1] = shape[1] - shape[1] % 2;
        }

        return shape;
    }


protected:
    // -- Neighborhood interface ----------------------------------------------
    /// Retrieve the neighborhood function depending on the mode
    NBFuncID<Base> get_nb_func(NBMode nb_mode,
                               const Config& nb_params) override
    {
        if (nb_mode == NBMode::empty) {
            return this->_nb_empty;
        }
        else if (nb_mode == NBMode::hexagonal) {
            return get_nb_func_hexagonal(nb_params);
        }
        else {
            throw std::invalid_argument("No '" + nb_mode_to_string(nb_mode)
                + "' neighborhood available for HexagonalGrid! "
                  "Available modes: empty, hexagonal.");
        }
    }

    /// Computes the expected number of neighbors for a neighborhood mode
    DistType expected_num_neighbors(const NBMode& nb_mode,
                                    const Config&) const override
    {
        if (nb_mode == NBMode::empty) {
            return 0;
        }
        else if (nb_mode == NBMode::hexagonal) {
            return 6;
        }
        else {
            throw std::invalid_argument("No '" + nb_mode_to_string(nb_mode)
                + "' neighborhood available for HexagonalGrid! "
                  "Available modes: empty, hexagonal.");
        }
    }


    // .. Neighborhood implementations ........................................
    // NOTE With C++20, the below lambdas would allow template arguments

    // .. hexagonal . . . . . . . . . . . . . . . . . . . . . . . . . . . . . .

    /// Returns a standalone hexagonal neighborhood function
    /** \details It extracts the distance parameter from the configuration and
      *          depending on the distance parameter and the periodicity of the
      *          space decides between four different neighborhood calculation
      *          functions.
      *          The returned callable does rely on the SquareGrid object, but
      *          it includes all parameters from the configuration that can be
      *          computed once and then captured; this avoids recomputation.
      * 
      * \param  nb_params   The configuration for the hexagonal neighborhood
      *                     method. Expected keys: ``distance`` (optional,
      *                     defaults to 1), which refers to the Manhattan
      *                     distance of included neighbors.
      */
    NBFuncID<Base> get_nb_func_hexagonal(const Config& nb_params) {
        // Extract the optional distance parameter
        constexpr bool check_shape = true;
        const auto distance = get_nb_param_distance<check_shape>(nb_params);

        // For distance 1, use the specialized functions which are defined as
        // class members (to don't bloat this method even more). Those
        // functions do not require any calculation or capture that goes beyond
        // the capture of ``this``.
        if (distance <= 1) {
            if (this->is_periodic()) {
                return _nb_hexagonal_periodic;
            }
            else {
                return _nb_hexagonal_nonperiodic;
            }
        }
        // else: distance is > 1. Depending on periodicity of the grid, define
        // the relevant lambda and let it capture as many values as possible in
        // order to avoid recomputation.

        throw std::invalid_argument(fmt::format(
            "Hexagonal neighborhood is not implemented for a "
            "distance larger than 1. Requested distance was {}.",
            distance
        ));
    }


    /// The Von-Neumann neighborhood for periodic grids
    NBFuncID<Base> _nb_hexagonal_periodic = 
        [this](const IndexType root_id)
    {
        // Instantiate container in which to store the neighboring cell IDs
        IndexContainer neighbor_ids{};

        // The number of neighbors is known; pre-allocating space brings a
        // speed improvement of about factor 2.
        neighbor_ids.reserve(3 * dim);

        // Depending on the number of dimensions, add the IDs of neighboring
        // cells in those dimensions
        add_neighbors_in_<0, true>(root_id, neighbor_ids);
        add_neighbors_in_<1, true>(root_id, neighbor_ids);
        add_neighbors_in_<2, true>(root_id, neighbor_ids);

        // Return the container of cell indices
        return neighbor_ids;
    };


    /// The Von-Neumann neighborhood for non-periodic grids
    NBFuncID<Base> _nb_hexagonal_nonperiodic = 
        [this](const IndexType root_id)
    {
        // Instantiate container in which to store the neighboring cell IDs
        IndexContainer neighbor_ids{};

        // The number of neighbors is known; pre-allocating space brings a
        // speed improvement of about factor 2
        neighbor_ids.reserve(2 * dim);

        // Depending on the number of dimensions, add the IDs of neighboring
        // cells in those dimensions
        add_neighbors_in_<0, false>(root_id, neighbor_ids);
        add_neighbors_in_<1, false>(root_id, neighbor_ids);
        add_neighbors_in_<2, false>(root_id, neighbor_ids);

        // Return the container of cell indices
        return neighbor_ids;
    };


    // .. Neighborhood helper functions .......................................

    /// Add both direct neighbors to a container of indices
    /** This function takes an index container and populates it with the
     *  indices of neighboring cells in different dimensions, specified by
     *  template parameter 0 < `axis` < number of dimensions - 1.
     * 
     *  The algorithm first calculates whether the given root cell index has a
     *  front or back boundary in the chosen dimension. If so, the neighboring
     *  cell is only added if the grid is periodic.
     * 
     * \param root_id      Which cell to find the agents of
     * \param neighbor_ids The container to populate with the indices
     * 
     * \tparam axis        The axis along which to add the neighbors (0-based!)
     * \tparam periodic    Whether the grid is periodic
     * 
     * \return void
     */
    template<DimType axis, bool periodic>
    void add_neighbors_in_(const IndexType root_id,
                           IndexContainer& neighbor_ids) const
    {
        // Assure the number of dimensions is supported
        static_assert(axis < 3);

        // left and right
        if constexpr (axis == 0) {
            // Compute a normalized id
            const IndexType nrm_id = root_id % _shape[0];

            // Check if at low value boundary
            if (nrm_id == 0) {
                // left most column
                if constexpr (periodic) {
                    neighbor_ids.push_back(root_id - 1 + _shape[0]);
                }
                // else: not periodic; nothing to add here
            }
            else {
                // Not at boundary; no need for the correction term
                neighbor_ids.push_back(root_id - 1);
            }

            // Check if at high value boundary
            if (nrm_id == _shape[0] - 1) {
                // right most column
                if constexpr (periodic) {
                    neighbor_ids.push_back(root_id + 1 - _shape[0]);
                }
                // else: not periodic; nothing to add here
            }
            else {
                // Not at boundary; no need for the correction term
                neighbor_ids.push_back(root_id + 1);
            }
        }
        // top-left and bottom-right
        else if constexpr (axis == 1) {
            // Compute normalized ids
            const IndexType nrm_id_0 = root_id % _shape[0]; // column
            const IndexType nrm_id_1 = (  (root_id % (_shape[0] * _shape[1]))
                                        / _shape[0]); // row

            // Check if at low value boundary
            if (nrm_id_1 == 0) {
                // bottom row
                if constexpr (periodic) {
                    if (nrm_id_0 < _shape[0] - 1) {
                        neighbor_ids.push_back(  root_id - _shape[0] + 1
                                               + _shape[0] * _shape[1]);
                    }
                    else {
                        // also right most
                        neighbor_ids.push_back(  root_id - 2*_shape[0] + 1
                                               + _shape[0] * _shape[1]);
                    }
                }
                // else: not periodic; nothing to add here
            }
            else if (nrm_id_0 == _shape[0] - 1 and nrm_id_1 % 2 == 0) {
                // right column, offset rows
                if constexpr (periodic) {
                    neighbor_ids.push_back(root_id - 2*_shape[0] + 1);
                }
                // else: not periodic; nothing to add here
            }
            else if (nrm_id_1 % 2 == 0) {
                // offset row
                // Not at boundary; no need for the correction term
                neighbor_ids.push_back(root_id - _shape[0] + 1);
            }
            else {
                // non-offset row
                neighbor_ids.push_back(root_id - _shape[0]);
            }

            // Check if at high value boundary
            if (nrm_id_1 == _shape[1] - 1) {
                // top row
                if constexpr (periodic) {
                    if (nrm_id_0 > 0) {
                        // is an non-offset row
                        neighbor_ids.push_back(  root_id + _shape[0] - 1
                                               - _shape[0] * _shape[1]);
                    }
                    else {
                        // left most cell
                        neighbor_ids.push_back(  root_id + 2*_shape[0] - 1
                                               - _shape[0] * _shape[1]);
                    }
                }
                // else: not periodic; nothing to add here
            }
            else if (nrm_id_0 == 0 and nrm_id_1 % 2 == 1){
                // left column impair row
                if constexpr (periodic) {
                    neighbor_ids.push_back(root_id + 2*_shape[0] - 1);                    
                }
            }
            else if (nrm_id_1 % 2 == 0) {
                // Not at boundary; no need for the correction term
                neighbor_ids.push_back(root_id + _shape[0]);
            }
            else {
                // non-offset row
                neighbor_ids.push_back(root_id + _shape[0] - 1);
            }
        }
        // top right and bottom left
        else {
            // Compute normalized ids
            const IndexType nrm_id_0 = root_id % _shape[0]; // column
            const IndexType nrm_id_1 = (  (root_id % (_shape[0] * _shape[1]))
                                        / _shape[0]); // row 

            if (nrm_id_1 == 0) {
                // bottom row
                if constexpr (periodic) {
                    neighbor_ids.push_back(  root_id - _shape[0]
                                           + _shape[0] * _shape[1]);
                }
                // else: not periodic; nothing to add here
            }
            else if (nrm_id_0 == 0 and nrm_id_1 % 2 == 1) {
                // left most, non-offset row
                if constexpr (periodic) {
                    neighbor_ids.push_back(root_id - 1);
                }
                // else: not periodic; nothing to add here
            }
            else if (nrm_id_1 % 2 == 1) {
                neighbor_ids.push_back(root_id - _shape[0] - 1);
            }
            else {
                neighbor_ids.push_back(root_id - _shape[0]);
            }

            if (nrm_id_1 == _shape[1] - 1) {
                // top row
                if constexpr (periodic) {
                    neighbor_ids.push_back(  root_id + _shape[0]
                                           - _shape[0] * _shape[1]);
                }
                // else: not periodic; nothing to add here
            }
            else if (nrm_id_0 == _shape[0] - 1 and nrm_id_1 % 2 == 0) {
                // right column in offset row
                if constexpr (periodic) {
                    neighbor_ids.push_back(root_id + 1);
                }
                // else: not periodic; nothing to add here
            }
            else if (nrm_id_1 % 2 == 0) {
                neighbor_ids.push_back(root_id + _shape[0] + 1);
            }
            else {
                neighbor_ids.push_back(root_id + _shape[0]);
            }
        }
    }


    // .. Neighborhood parameter extraction helpers ...........................

    /// Extract the ``distance`` neighborhood parameter from the given config
    /** \note   This will never return a KeyError; if the key is not given,
      *         this method will return ``1``.
      *
      * \tparam check_shape  Whether to check the shape of the grid is large
      *                      enough for this distance. For all SquareGrid
      *                      neighborhoods in periodic space, the grid needs
      *                      to be at least ``2 * distance + 1`` cells wide in
      *                      each dimension.
      *
      * \param  params       The neighborhood parameters to extract the
      *                      ``distance`` parameter from.
      */
    template<bool check_shape=false>
    DistType get_nb_param_distance(const Config& params) const {
        const auto distance = get_as<DistType>("distance", params, 1);

        // Check the value is smaller than the grid shape. It needs to fit into
        // the shape of the grid, otherwise all the algorithms above would have
        // to check for duplicate entries and be set-based, which would be
        // very inefficient.
        if constexpr (check_shape) {
            if (    this->is_periodic()
                and (distance * 2 + 1 > this->shape().min()))
            {
                // To inform about the grid shape, print it to the stringstream
                // and include it in the error message below.
                std::stringstream shape_ss;
                this->shape().print(shape_ss, "Grid Shape:");

                throw std::invalid_argument("The grid shape is too small to "
                    "accomodate a neighborhood with 'distance' parameter set "
                    "to " + get_as<std::string>("distance", params, "1")
                    + " in a periodic space!\n" + shape_ss.str());
            }
        }

        return distance;
    }
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRIDS_HEXAGONAL_HH
