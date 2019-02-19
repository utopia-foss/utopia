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
    NBFuncID<Base> get_nb_func(NBMode nb_mode,
                               const DataIO::Config& nbh_params) override
    {
        if (nb_mode == NBMode::empty) {
            return this->_nb_empty;
        }
        else if (nb_mode == NBMode::vonNeumann) {
            // Supports the optional neighborhood parameter 'distance'
            this->set_nbh_params(nbh_params,
                                 // Container of (key, required?) pairs:
                                 {{"distance", false}});
            // If the distance was not given, _nbh_distance is 0

            // Use the function that is best specialized for each scenario
            if (this->is_periodic()) {
                if (this->_nbh_distance <= 1) {
                    return _nb_vonNeumann_periodic;
                }
                else {
                    return _nb_VonNeumann_periodic_with_Manhatten_distance;
                }
            }
            else {
                if (this->_nbh_distance <= 1) {
                    return _nb_vonNeumann_nonperiodic;
                }
                else {
                    return _nb_vonNeumann_nonperiodic_with_Manhatten_distance;
                }
            }
        }
        else if (nb_mode == NBMode::Moore) {
            // Supports the optional neighborhood parameter 'distance'
            this->set_nbh_params(nbh_params,
                                 // Container of (key, required?) pairs:
                                 {{"distance", false}});
            // If the distance was not given, _nbh_distance is 0

            // Use the function that is best specialized for each scenario
            if (this->is_periodic()) {
                if (this->_nbh_distance <= 1) {
                    return _nb_Moore_periodic;
                }
                else {
                    return _nb_Moore_periodic_with_Chebychev_distance;
                }
            }
            else {
                if (this->_nbh_distance <= 1) {
                    return _nb_Moore_nonperiodic;
                }
                else {
                    return _nb_Moore_nonperiodic_with_Chebychev_distance;
                }
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
    NBFuncID<Base> _nb_vonNeumann_periodic = 
        [this](const IndexType& root_id)
    {
        static_assert((dim >= 1 and dim <= 2),
            "VonNeumann neighborhood is implemented for 1D or 2D space!");

        // Instantiate container in which to store the neighboring cell IDs
        IndexContainer neighbor_ids{};

        // The number of neighbors is known; pre-allocating space brings a
        // speed improvement of about factor 2
        neighbor_ids.reserve(2 * dim);

        // Depending on the number of dimensions, add the IDs of neighboring
        // cells in those dimensions
        add_neighbors_in_<0, true>(root_id, neighbor_ids);

        if constexpr (dim >= 2) {
            add_neighbors_in_<1, true>(root_id, neighbor_ids);
        }

        // Return the container of cell indices
        return neighbor_ids;
    };

    /// The Von-Neumann neighborhood for periodic grids and arbitrary Manhatten distance
    NBFuncID<Base> _nb_VonNeumann_periodic_with_Manhatten_distance =
        [this](const IndexType& root_id)
    {
        static_assert((dim >= 1 and dim <= 2),
            "VonNeumann neighborhood is implemented for 1D or 2D space!");

        // Instantiate container in which to store the neighboring cell IDs
        IndexContainer neighbor_ids{};

        // Compute neighborhood size
        const auto nb_size = expected_num_neighbors(NBMode::vonNeumann);

        // Pre-allocating space brings a speed improvement of about factor 2
        neighbor_ids.reserve(nb_size);

        // Depending on the number of dimensions, add the IDs of neighboring
        // cells in those dimensions
        // Add neighbors in dimension 1
        for (DistType dist=1; dist <= this->_nbh_distance; ++dist) {
            add_low_val_neighbor_in_<0, true>(root_id, dist, neighbor_ids);
            add_high_val_neighbor_in_<0, true>(root_id, dist, neighbor_ids);
        }

        // If the dimension exists, add neighbors in dimension 2
        if constexpr (dim >= 2) {
            // Go through all the previously added neighbors and add the
            // additional neighbors from the other dimension.
            // NOTE that this algorithm requires the neighbors nearest
            //      to the root_id to have been pushed to the vector first.
            //      The fixed ordering of the previous addition is required.
            const auto nb_size = neighbor_ids.size();

            for (DistType i=0; i < nb_size; ++i) {
                // Add all neighbor ids up to the maximal distance along the
                // dimension 2.
                for (DistType dist = 1; 
                     dist <= this->_nbh_distance - 1 - i/2 + (i%2)/2; 
                     ++dist)
                {
                    // front neighbor
                    add_low_val_neighbor_in_<1, true>(neighbor_ids[i],
                                                      dist,
                                                      neighbor_ids);

                    // back neighbor
                    add_high_val_neighbor_in_<1, true>(neighbor_ids[i],
                                                       dist,
                                                       neighbor_ids);
                }
            }

            // Finally, add the root cell's neighbors in the second dimension
            for (DistType dist=1; dist <= this->_nbh_distance; ++dist) {
                add_low_val_neighbor_in_<1, true>(root_id, dist, neighbor_ids);
                add_high_val_neighbor_in_<1,true>(root_id, dist, neighbor_ids);
            }
        }

        // Return the container of cell indices
        return neighbor_ids;
    
    };

    /// The Von-Neumann neighborhood for non-periodic grids
    NBFuncID<Base> _nb_vonNeumann_nonperiodic = 
        [this](const IndexType& root_id)
    {
        static_assert(((dim == 1) or (dim == 2)),
            "VonNeumann neighborhood is only implemented in 1 or 2 dimensions "
            "in space!");

        // Instantiate container in which to store the neighboring cell IDs
        IndexContainer neighbor_ids{};

        // The number of neighbors is known; pre-allocating space brings a
        // speed improvement of about factor 2
        neighbor_ids.reserve(2 * dim);

        // Depending on the number of dimensions, add the IDs of neighboring
        // cells in those dimensions
        add_neighbors_in_<0, false>(root_id, neighbor_ids);

        if constexpr (dim >= 2) {
            add_neighbors_in_<1, false>(root_id, neighbor_ids);
        }

        // Return the container of cell indices
        return neighbor_ids;
    };

    /// The Von-Neumann neighborhood for non-periodic grids and arbitrary Manhatten distance
    NBFuncID<Base> _nb_vonNeumann_nonperiodic_with_Manhatten_distance = 
        [this](const IndexType& root_id)
    {
        static_assert(((dim == 1) or (dim == 2)),
            "VonNeumann neighborhood is implemented for 1D or 2D space!");

        // Instantiate containers in which to store the neighboring cell IDs
        IndexContainer front_nb_ids{};
        IndexContainer back_nb_ids{};

        // Pre-allocating space brings a speed improvement of about factor 2
        // NOTE The front_nb_ids vector needs to reserve memory for all
        //      neighbors including the back neighbors because these will be
        //      added to the container directly before returning it.
        front_nb_ids.reserve(expected_num_neighbors(NBMode::vonNeumann));
        back_nb_ids.reserve(expected_num_neighbors(NBMode::vonNeumann) / 2);

        // Depending on the number of dimensions, add the IDs of neighboring
        // cells in those dimensions
        // Add front neighbors in dimension 1
        for (DistType dist=1; dist <= this->_nbh_distance; ++dist) {
            add_low_val_neighbor_in_<0, false>(root_id, dist, front_nb_ids);
            add_high_val_neighbor_in_<0, false>(root_id, dist, back_nb_ids);
        }

        // If the dimension exists, add neighbors in dimension 2
        if constexpr (dim >= 2) {
            // Go through the front neighbor ids in dimension 1 and add
            // the neighbor ids in dimension 2
            // NOTE that this algorithm requires the neighbors nearest
            //      to the root_id to have been pushed to the vector first.
            //      The fixed ordering of the previous addition is required.
            const DistType front_nb_size = front_nb_ids.size();

            for (DistType i=0; i < front_nb_size; ++i) {
                // Add all front neighbor ids up to the maximal distance along
                // dimension 2.
                for (DistType dist = 1; 
                     dist <= this->_nbh_distance - (i + 1); 
                     ++dist)
                {
                    // add front neighbors ids in dimension 2
                    add_low_val_neighbor_in_<1, false>(front_nb_ids[i],
                                                       dist,
                                                       front_nb_ids);
                    
                    // add back neighbors ids in dimension 2
                    add_high_val_neighbor_in_<1, false>(front_nb_ids[i],
                                                        dist,
                                                        front_nb_ids);
                }
            }

            // Go through the front neighbor ids in dimension 1 and add
            // the neighbor ids in dimension 2
            // NOTE that this algorithm requires the neighbors nearest
            //      to the root_id to have been pushed to the vector first.
            //      The fixed ordering of the previous addition is required.
            const DistType back_nb_size = back_nb_ids.size();

            for (DistType i=0; i<back_nb_size; ++i) {
                // Add all neighbor ids up to the maximal distance along
                // second dimension
                for (DistType dist = 1; 
                     dist <= this->_nbh_distance - (i + 1); 
                     ++dist)
                {
                    add_low_val_neighbor_in_<1, false>(back_nb_ids[i],
                                                       dist,
                                                       back_nb_ids);

                    add_high_val_neighbor_in_<1, false>(back_nb_ids[i],
                                                        dist,
                                                        back_nb_ids);
                }
            }

            // Finally, add the root cell's neighbors in the second dimension
            for (DistType dist=1; dist <= this->_nbh_distance; ++dist) {
                add_low_val_neighbor_in_<1, false>(root_id,
                                                   dist,
                                                   front_nb_ids);

                add_high_val_neighbor_in_<1, false>(root_id,
                                                    dist,
                                                    back_nb_ids);
            }
        }

        // Combine the front and back neighbors container
        front_nb_ids.insert(front_nb_ids.end(),
                            back_nb_ids.begin(), back_nb_ids.end());

        // Done now. The front neighbor container contains all the IDs.
        return front_nb_ids;
    };
    
    /// Moore neighbors for periodic 2D grid
    NBFuncID<Base> _nb_Moore_periodic = 
       [this](const IndexType& root_id)
    {
        static_assert(dim == 2, "Moore neighborhood is only available in 2D!");

        // Generate vector in which to store the neighbors and allocate space
        IndexContainer neighbor_ids{};
        neighbor_ids.reserve(8);

        // Get the neighbors in the second dimension
        add_neighbors_in_<1, true>(root_id, neighbor_ids);
        // ...have these neighbors at indices 0 and 1 now.

        // For these neighbors, add _their_ neighbors in the first dimension
        add_neighbors_in_<0, true>(neighbor_ids[0], neighbor_ids);
        add_neighbors_in_<0, true>(neighbor_ids[1], neighbor_ids);

        // And finally, add the root cell's neighbors in the first dimension
        add_neighbors_in_<0, true>(root_id, neighbor_ids);

        return neighbor_ids;
    };

    /// Moore neighbors for periodic 2D grid for arbitrary Chebychev distance
    NBFuncID<Base> _nb_Moore_periodic_with_Chebychev_distance = 
        [this](const IndexType& root_id)
    {
        static_assert(dim == 2, "Moore neighborhood is only available in 2D!");
        
        // Generate vector in which to store the neighbors... 
        IndexContainer neighbor_ids{};

        // ... and allocate space
        neighbor_ids.reserve(expected_num_neighbors(NBMode::Moore));

        // Get all neighbors in the first dimension
        for (DistType dist=1; dist <= this->_nbh_distance; ++dist) {
            add_low_val_neighbor_in_<0, true>(root_id, dist, neighbor_ids);
            add_high_val_neighbor_in_<0, true>(root_id, dist, neighbor_ids);
        }

        // For these neighbors, add _their_ neighbors in the second dimension
        for (const auto& nb : neighbor_ids) {
            for (DistType dist=1; dist <= this->_nbh_distance; ++dist) {
                add_low_val_neighbor_in_<1, true>(nb, dist, neighbor_ids);
                add_high_val_neighbor_in_<1, true>(nb, dist, neighbor_ids);
            }
        }

        // And finally, add the root cell's neighbors in the second dimension
        for (DistType dist=1; dist <= this->_nbh_distance; ++dist) {
            add_low_val_neighbor_in_<1, true>(root_id, dist, neighbor_ids);
            add_high_val_neighbor_in_<1, true>(root_id, dist, neighbor_ids);
        }

        return neighbor_ids;
    };

    /// Moore neighbors for non-periodic 2D grid
    NBFuncID<Base> _nb_Moore_nonperiodic = 
        [this](const IndexType& root_id)
    {
        static_assert(dim == 2, "Moore neighborhood is only available in 2D!");

        // Generate vector in which to store the neighbors and allocate space
        IndexContainer neighbor_ids{};
        neighbor_ids.reserve(8);

        // Get the neighbors in the second dimension
        add_neighbors_in_<1, false>(root_id, neighbor_ids);
        // root not at border: have them at indices 0 and 1 now
        // root at border: less than two neighbors were added

        // Distinguish these two cases
        if (neighbor_ids.size() == 2) {
            // Was not at a boundary.
            add_neighbors_in_<0, false>(neighbor_ids[0], neighbor_ids);
            add_neighbors_in_<0, false>(neighbor_ids[1], neighbor_ids);
        }
        else if (neighbor_ids.size() == 1) {
            // Was at a front XOR back boundary in dimension 2
            add_neighbors_in_<0, false>(neighbor_ids[0], neighbor_ids);
        }
        // else: was at front AND back boundary (single row of cells in dim 2)

        // Finally, add the root's neighbors in the first dimension
        add_neighbors_in_<0, false>(root_id, neighbor_ids);

        return neighbor_ids;
    };

    /// Moore neighbors for non-periodic 2D grid
    NBFuncID<Base> _nb_Moore_nonperiodic_with_Chebychev_distance = 
        [this](const IndexType& root_id)
    {
        static_assert(dim == 2, "Moore neighborhood is only available in 2D!");

        // Generate vector in which to store the neighbors...
        IndexContainer neighbor_ids{};
        
        // ... and allocate space
        neighbor_ids.reserve(expected_num_neighbors(NBMode::Moore));

        // Get all neighbors in the first dimension
        for (DistType dist=1; dist <= this->_nbh_distance; ++dist) {
            add_low_val_neighbor_in_<0, false>(root_id, dist, neighbor_ids);
            add_high_val_neighbor_in_<0, false>(root_id, dist, neighbor_ids);
        }

        // For these neighbors, add _their_ neighbors in the second dimension
        for (const auto& nb : neighbor_ids) {
            for (DistType dist=1; dist <= this->_nbh_distance; ++dist) {
                add_low_val_neighbor_in_<1, false>(nb, dist, neighbor_ids);
                add_high_val_neighbor_in_<1, false>(nb, dist, neighbor_ids);
            }
        }

        // And finally, add the root cell's neighbors in the second dimension
        for (DistType dist=1; dist <= this->_nbh_distance; ++dist) {
            add_low_val_neighbor_in_<1, false>(root_id, dist, neighbor_ids);
            add_high_val_neighbor_in_<1, false>(root_id, dist, neighbor_ids);
        }

        return neighbor_ids;
    };

    // .. Neighborhood helper functions .......................................

    /// Return the shift in cell indices necessary if moving along an axis
    /** It returns in the different cases:
     *    - axis == 0 -> 1
     *    - axis == 1 -> shape[0] * 1
     *    - axis == 2 -> shape[1] * shape[0] * 1
     *    - axis == 3 -> shape[2] * shape[1] * shape[0] * 1
     *    - ...
     * 
     * \tparam axis  In which dimension the shift is desired
     *
     * \return constexpr IndexType
     */
    template<DimType axis>
    constexpr IndexType id_shift() const {
        if constexpr (axis == 0) {
            // End of recursion
            return 1;
        }
        else {
            // Recursive branch
            return _shape[axis - 1] * id_shift<axis-1>();
        }
    }

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
    void add_neighbors_in_(const IndexType& root_id,
                           IndexContainer& neighbor_ids) const
    {
        // Assure the number of dimensions is supported
        static_assert(dim <= 2,
            "Unsupported dimensionality of underlying space! Need be 1 or 2.");
        static_assert(axis < dim);

        // Compute a "normalized" ID along the desired dimension in which the
        // neighbors are to be added. Is always in [0, shape[d] - 1].
        const auto nrm_id = (  (root_id % id_shift<axis+1>())
                             / id_shift<axis>());
        // NOTE _Should_ also work for d > 2, but need tests for that.

        // Check if at low value boundary
        if (nrm_id == 0) {
            if constexpr (periodic) {
                neighbor_ids.push_back(  root_id
                                       - id_shift<axis>()
                                       + id_shift<axis+1>());
            }
            // else: not periodic; nothing to add here
        }
        else {
            // Not at boundary; no need for the correction term
            neighbor_ids.push_back(root_id - id_shift<axis>());
        }

        // Check if at high value boundary
        if (nrm_id == _shape[axis] - 1) {
            if constexpr (periodic) {
                neighbor_ids.push_back(  root_id
                                       + id_shift<axis>()
                                       - id_shift<axis+1>());
            }
        }
        else {
            neighbor_ids.push_back(root_id + id_shift<axis>());
        }
    }

    /// Add a neighbor on the low (ID) value side to an index container
    /** This function takes an index container and populates it with the
     *  indices of neighboring cells in different dimensions, specified by
     *  template parameter 0 < `axis` < number of dimensions - 1.
     * 
     *  The algorithm first calculates whether the given root cell index has a
     *  front boundary in the chosen dimension. If so, the neighboring
     *  cell is only added if the grid is periodic.
     * 
     * \param root_id      Which cell to find the agents of
     * \param distance     Which distance the neighbor has to the root cell
     * \param neighbor_ids The container to populate with the indices
     * 
     * \tparam axis        The axis along which to add the neighbor (0-based!)
     * \tparam periodic    Whether the grid is periodic
     * 
     * \return void
     */
    template<DimType axis, bool periodic>
    void add_low_val_neighbor_in_(const IndexType& root_id,
                                  const DistType distance,
                                  IndexContainer& neighbor_ids) const
    {
        // Assure the number of dimensions is supported
        static_assert(dim <= 2,
            "Unsupported dimensionality of underlying space! Need be 1 or 2.");
        static_assert(axis < dim);

        // If the distance is zero, no neighbor can be added; return nothing.
        if (distance == 0) {
            return;
        }

        // Check if the neighbor to be added would pass a low value boundary.
        // Do so by computing a "normalized" ID along the desired dimension in
        // which the neighbor is to be added (always in [0, shape[d] - 1]) and
        // then compare to the distance
        if (  (root_id % id_shift<axis+1>()) / id_shift<axis>()
            < distance)
        {
            if constexpr (periodic) {
                neighbor_ids.push_back(  root_id
                                       - distance * id_shift<axis>()
                                       + id_shift<axis+1>());
            }
        }
        else {
            neighbor_ids.push_back(  root_id 
                                   - distance * id_shift<axis>());
        }
    }

    /// Add a neighbor on the high (ID) value side to an index container
    /** This function takes an index container and populates it with the
     *  index of a neighboring cell in different dimensions, specified by
     *  template parameter 0 < `axis` < number of dimensions - 1.
     * 
     *  The algorithm first calculates whether the given root cell index has a
     *  back boundary in the chosen dimension. If so, the neighboring
     *  cell is only added if the grid is periodic.
     * 
     * \param root_id      Which cell to find the agents of
     * \param distance     Which distance the neighbor has to the root cell
     * \param neighbor_ids The container to populate with the indices
     * 
     * \tparam axis        The axis along which to add the neighbor (0-based!)
     * \tparam periodic    Whether the grid is periodic
     * 
     * \return void
     */
    template<DimType axis, bool periodic>
    void add_high_val_neighbor_in_(const IndexType& root_id,
                                   const DistType distance,
                                   IndexContainer& neighbor_ids) const
    {
        // Assure the number of dimensions is supported
        static_assert(dim <= 2,
            "Unsupported dimensionality of underlying space! Need be 1 or 2.");
        static_assert(axis < dim);

        // If the distance is zero, no neighbor can be added; return nothing.
        if (distance == 0) {
            return;
        }

        // Check if the neighbor to be added would pass a high value boundary.
        // Do so by computing a "normalized" ID along the desired dimension in
        // which the neighbor is to be added (always in [0, shape[d] - 1]) and
        // then compare to the distance from the high value boundary.
        if (  (root_id % id_shift<axis+1>()) / id_shift<axis>()
            >= _shape[axis] - distance)
        {
            if constexpr (periodic) {
                neighbor_ids.push_back(  root_id
                                       + distance * id_shift<axis>()
                                       - id_shift<axis+1>());
            }
        }
        else {
            neighbor_ids.push_back(  root_id 
                                   + distance * id_shift<axis>());
        }
    }

    /// Computes the expected number of neighbors for a neighborhood mode
    /** \detail This function is used to calculate the amount of memory
      *         that should be reserved for the neighbor_ids vector.
      *         For the calculation it uses the member variables: 
      *         `dim` and `_nbh_distance`
      *  
      * For a von Neumann neighborhood, the number of neighbors is:
      *                      { 2 * distance   for distance = 1
      *  N(dim, distance) =  { 2 * sum_{distances} N(dim-1, distance)
      *                      {                for distance > 1)
      *  
      * For a Moore neighborhood, the number of neighbors is:
      *     
      *  N(dim, distance) = (2 * distance + 1)^2 - 1
      * 
      * 
      * \warning The expected number of neighbors is not equal to the actually
      *          calculated number of neighbors. For example in a nonperiodic
      *          grids the expected number of neighbors is greater than the
      *          actual number of neighbors in the edge cases. Thus, this
      *          function should only be used to reserve memory for the 
      *          neighbor_ids vector.
      * 
      * \return const DistType The expected number of neighbors
      */
    DistType expected_num_neighbors(const NBMode nb_mode) const {
        if (nb_mode == NBMode::empty) {
            return 0;
        }
        else if (nb_mode == NBMode::Moore) {
            if (this->_nbh_distance <= 1) {
                return std::pow(2 + 1, dim) - 1;
            }
            else {
                return std::pow(2 * this->_nbh_distance + 1, dim) - 1;
            }
        }
        else if (nb_mode == NBMode::vonNeumann) {
            // This one is more complicated ...
            // Define a lambda that can be called recursively
            auto num_nbs_impl = [](const unsigned short int d,  // dimension
                                   DistType distance,
                                   auto& num_nbs_ref)
            {
                if (d == 1) {
                    // End of recursion
                    return 2 * distance;
                }
                else {
                    // Recursive branch
                    DistType cnt = 0;
                    while (distance > 0) {
                        cnt += 2 * num_nbs_ref(d-1, distance, num_nbs_ref);
                        --distance;
                    }
                    return cnt;
                }                   
            };

            // Call the recursive lambda with the space's dimensionality
            return num_nbs_impl(this->dim, this->_nbh_distance, num_nbs_impl);
        }
        else {
            throw std::invalid_argument("No '" + nb_mode_to_string(nb_mode)
                + "' available for rectangular grid discretization!");
        }
    };
};

// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRIDS_SQUARE_HH
