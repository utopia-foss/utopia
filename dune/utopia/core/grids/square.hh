#ifndef UTOPIA_CORE_GRIDS_SQUARE_HH
#define UTOPIA_CORE_GRIDS_SQUARE_HH

#include <sstream>

#include "base.hh"

namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

/// A grid discretization using square cells
template<class Space>
class SquareGrid
    : public Grid<Space>
{
public:
    /// Base class type
    using Base = Grid<Space>;

    /// The dimensionality of the space to be discretized (for easier access)
    static constexpr std::size_t dim = Space::dim;

private:
    // -- SquareGrid-specific members -- //
    /// The (multi-index) shape of the grid, resulting from resolution
    /** \note For the exact interpretation of the shape and how it results from
      *       the resolution, consult the derived classes documentation
      */
    const GridShapeType<dim> _shape;

public:
    /// Construct a rectangular grid discretization
    /** \param  space   The space to construct the discretization for
      * \param  cfg     Further configuration parameters
      */
    SquareGrid (std::shared_ptr<Space> space, const DataIO::Config& cfg)
    :
        Base(space, cfg),
        _shape(determine_shape())
    {
        if constexpr (dim > 1) {
            // Make sure the cells really are square
            auto eff_res = effective_resolution();

            if (not std::equal(eff_res.begin() + 1, eff_res.end(),
                               eff_res.begin()))
            {
                // Format effective resolution to show it in error message
                std::stringstream efr_ss;
                efr_ss.precision(8);
                efr_ss << "(";
                for (std::size_t i=0; i<dim-1; i++) {
                    efr_ss << eff_res[i] << ", ";
                }
                efr_ss << eff_res[dim-1] << ")";

                throw std::invalid_argument("Given the extent of the physical "
                    "space and the specified resolution, a mapping with "
                    "exactly square cells could not be found! Either adjust "
                    "the physical space, the resolution of the grid, or "
                    "choose another grid.\nEffective resolution was: "
                    + efr_ss.str() + ", but should be the same in all "
                    "dimensions!");
            }
        }
    }


    // -- Implementations of virtual base class functions -- //

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
    const std::array<double, dim> effective_resolution() const override {
        std::array<double, dim> res_eff;

        for (std::size_t i = 0; i < dim; i++) {
            res_eff[i] = double(_shape[i]) / this->_space->extent[i];
        }

        return res_eff;
    }

    /// Get shape of the square grid
    const GridShapeType<Space::dim> shape() const override {
        // Can just return the calculated member here
        return _shape;
    }



protected:
    /// Given the resolution, return the grid shape required to fill the space
    /** \detail Integer rounding takes place here. A physical space of extents
      *         of 2.1 length units in each dimension and a resolution of two
      *         cells per unit length will result in 4 cells in each dimension,
      *         each cell's size scaled up slightly and the effective
      *         effective resolution thus slightly smaller than the specified
      *         resolution.
      */
    GridShapeType<dim> determine_shape() const {
        GridShapeType<dim> shape;

        for (std::size_t i = 0; i < dim; i++) {
            shape[i] = this->_space->extent[i] * this->_resolution;
        }

        return shape;
    }


    /// The expected number of neighbors dependent on the neighborhood
    /** \detail This function is used to calculate the amount of memory
     *          that should be reserved for the neighbor_ids vector.
     *          For the calculation it uses the member variables: 
     *          `dim` and `_nbh_distance`
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
     *  \warning The expected number of neighbors is not equal to the actually
     *           calculated number of neighbors. For example in a nonperiodic
     *           grids the expected number of neighbors is greater than the
     *           actual number of neighbors in the edge cases. Thus, this
     *           function should only be used to reserve memory for the 
     *           neighbor_ids vector.
     * 
     * @return const std::size_t The expected number of neighbors
     */
    std::size_t expected_num_neighbors(const NBMode nb_mode) {
        // empty neighborhood
        if (nb_mode == NBMode::empty) {
            return 0;
        }
        
        // Moore neighborhood
        else if (nb_mode == NBMode::Moore) {
            return std::pow(2 * this->_nbh_distance + 1, dim) - 1; 
        }

        // von Neumann neighborhood
        else if (nb_mode == NBMode::vonNeumann) {
            auto num_nbs_impl = [](const unsigned short int dim, 
                                    std::size_t distance, auto& num_nbs_ref) {
                if (dim == 1) {
                    return 2 * distance;
                }
                else {
                    std::size_t counter = 0;
                    while (distance > 0) {
                        counter += 2 * num_nbs_ref(dim-1, distance, num_nbs_ref);
                        --distance;
                    }
                    return counter;
                }                   
            };
            return num_nbs_impl(dim, this->_nbh_distance, num_nbs_impl);
        }

        // no valid neighborhood mode
        else {
            throw std::invalid_argument("No '" + nb_mode_to_string(nb_mode)
                + "' available for rectangular grid discretization!");
        }
    };


    /// Retrieve the neighborhood function depending on the mode
    NBFuncID<Base> get_nb_func(NBMode nb_mode,
                               const DataIO::Config& nbh_params) override
    {
        // Choose the appropriate neighborhood function
        if (nb_mode == NBMode::empty) {
            return this->_nb_empty;
        }
        else if (nb_mode == NBMode::vonNeumann) {
            // Supports the optional neighborhood parameter 'distance'
            this->set_nbh_params(nbh_params,
                                 // Container of (key, required?) pairs:
                                 {{"distance", false}});
            // If the distance was not given, _nbh_distance is 0

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


    // -- Neighborhood implementations -- //
    // NOTE With C++20, the below lambdas would allow template arguments

    /// The Von-Neumann neighborhood for periodic grids
    NBFuncID<Base> _nb_vonNeumann_periodic = 
    [this](const IndexType& root_id)
    {
        static_assert((dim >= 1 and dim <= 2),
            "VonNeumann neighborhood is only implemented in 1-2 dimensions!");

        // Instantiate container in which to store the neighboring cell IDs
        IndexContainer neighbor_ids{};

        // The number of neighbors is known; pre-allocating space brings a
        // speed improvement of about factor 2
        neighbor_ids.reserve(2 * dim);

        // Depending on the number of dimensions, add the IDs of neighboring
        // cells in those dimensions
        add_neighbors_in_dim_<1, true>(root_id, neighbor_ids);

        if constexpr (dim >= 2) {
            add_neighbors_in_dim_<2, true>(root_id, neighbor_ids);
        }

        // Return the container of cell indices
        return neighbor_ids;
    };

    /// The Von-Neumann neighborhood for periodic grids and arbitrary Manhatten distance
    NBFuncID<Base> _nb_VonNeumann_periodic_with_Manhatten_distance =
    [this](const IndexType& root_id)
    {
        static_assert((dim >= 1 and dim <= 2),
            "VonNeumann neighborhood is only implemented in 1-2 dimensions!");

        if (this->_nbh_distance > 0) {
            std::runtime_error("The Manhatten (taxicab) distance has to be >0!");
        }

        // Use the _nb_vonNeumann_periodic function for _nbh_distance=1
        if (this->_nbh_distance == 1) {
            // TODO: Write warning to directly use the function without distance
            //       specification.
            return _nb_vonNeumann_periodic(root_id);
        }
        else {
            // Instantiate container in which to store the neighboring cell IDs
            IndexContainer neighbor_ids{};

            // Pre-allocating space brings a speed improvement of about factor 2
            neighbor_ids.reserve(expected_num_neighbors(NBMode::vonNeumann));

            // Depending on the number of dimensions, add the IDs of neighboring
            // cells in those dimensions
            // Add neighbors in dimension 1
            for (std::size_t dist=1; dist<=this->_nbh_distance; ++dist) {
                add_front_neighbor_in_dim_<1, true>(root_id, dist, neighbor_ids);
                add_back_neighbor_in_dim_<1, true>(root_id, dist, neighbor_ids);
            }

            // If the dimension exists, add neighbors in dimension 2
            if constexpr (dim >= 2) {
                // Go through all the previously added neighbors
                // and add the additional neighbors from the other dimension.
                // NOTE that this algorithm requires the neighbors nearest
                //      to the root_id to have been pushed to the vector first.
                //      The fixed ordering of the previous addition is required.
                const std::size_t nb_size = neighbor_ids.size(); 
                for (std::size_t i=0; i<nb_size; ++i) {
                    // Add all neighbor ids up to the maximal distance along the
                    // dimension 2.
                    for (std::size_t dist=1; 
                         dist<=this->_nbh_distance - 1 - i/2 + (i%2)/2; 
                         ++dist)
                    {
                        // front neighbor
                        add_front_neighbor_in_dim_<2, true>
                            (neighbor_ids[i], 
                            dist, 
                            neighbor_ids);

                        // back neighbor
                        add_back_neighbor_in_dim_<2, true>
                            (neighbor_ids[i], 
                            dist, 
                            neighbor_ids);
                    }
                }

                // Finally, add the root cell's neighbors in the second dimension
                for (std::size_t dist=1; dist<=this->_nbh_distance; ++dist) {
                    // front neighbor
                    add_front_neighbor_in_dim_<2, true>(root_id, 
                                                        dist, 
                                                        neighbor_ids);

                    // back neighbor
                    add_back_neighbor_in_dim_<2, true>(root_id, 
                                                        dist, 
                                                        neighbor_ids);
                }
            }

            // Return the container of cell indices
            return neighbor_ids;
        }
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
        add_neighbors_in_dim_<1, false>(root_id, neighbor_ids);

        if constexpr (dim >= 2) {
            add_neighbors_in_dim_<2, false>(root_id, neighbor_ids);
        }

        // Return the container of cell indices
        return neighbor_ids;
    };

    /// The Von-Neumann neighborhood for non-periodic grids and arbitrary Manhatten distance
    NBFuncID<Base> _nb_vonNeumann_nonperiodic_with_Manhatten_distance = 
        [this](const IndexType& root_id)
    {
        static_assert(((dim == 1) or (dim == 2)),
            "VonNeumann neighborhood is only implemented in 1 or 2 dimensions "
            "in space!");

        // Instantiate containers in which to store the neighboring cell IDs
        IndexContainer front_neighbor_ids{};
        IndexContainer back_neighbor_ids{};

        // Pre-allocating space brings a speed improvement of about factor 2
        // NOTE The front_neighbor_ids vector needs to reserve memory for all
        //      neighbors including the back neighbors because these will be
        //      added to the container directly before returning it.
        front_neighbor_ids.reserve(expected_num_neighbors(NBMode::vonNeumann));
        back_neighbor_ids.reserve(expected_num_neighbors(NBMode::vonNeumann)/2);

        // Depending on the number of dimensions, add the IDs of neighboring
        // cells in those dimensions
        // Add front neighbors in dimension 1
        for (std::size_t dist=1; dist<=this->_nbh_distance; ++dist) {
            add_front_neighbor_in_dim_<1, false>(root_id, dist, front_neighbor_ids);
            add_back_neighbor_in_dim_<1, false>(root_id, dist, back_neighbor_ids);
        }

        // If the dimension exists, add neighbors in dimension 2
        if constexpr (dim >= 2) {
            // Go through the front neighbor ids in dimension 1 and add
            // the neighbor ids in dimension 2
            // NOTE that this algorithm requires the neighbors nearest
            //      to the root_id to have been pushed to the vector first.
            //      The fixed ordering of the previous addition is required.
            const std::size_t front_nb_size = front_neighbor_ids.size(); 
            for (std::size_t i=0; i<front_nb_size; ++i) {
                // Add all front neighbor ids up to the maximal distance along
                // dimension 2.
                for (std::size_t dist=1; 
                        dist<=this->_nbh_distance - (i + 1); 
                        ++dist)
                {
                    // add front neighbors ids in dimension 2
                    add_front_neighbor_in_dim_<2, false>
                        (front_neighbor_ids[i], 
                        dist, 
                        front_neighbor_ids);
                    
                    // add back neighbors ids in dimension 2
                    add_back_neighbor_in_dim_<2, false>
                        (front_neighbor_ids[i], 
                        dist, 
                        front_neighbor_ids);
                }
            }

            // Go through the front neighbor ids in dimension 1 and add
            // the neighbor ids in dimension 2
            // NOTE that this algorithm requires the neighbors nearest
            //      to the root_id to have been pushed to the vector first.
            //      The fixed ordering of the previous addition is required.
            const std::size_t back_nb_size = back_neighbor_ids.size(); 
            for (std::size_t i=0; i<back_nb_size; ++i) {
                // Add all back neighbor ids up to the maximal distance along
                // dimension 2.
                for (std::size_t dist=1; 
                        dist<=this->_nbh_distance - (i + 1); 
                        ++dist)
                {
                    // front neighbor ids in dimension 2
                    add_front_neighbor_in_dim_<2, false>
                        (back_neighbor_ids[i], 
                        dist, 
                        back_neighbor_ids);

                    // back neighbors ids in dimension 2
                    add_back_neighbor_in_dim_<2, false>
                        (back_neighbor_ids[i], 
                        dist, 
                        back_neighbor_ids);
                }
            }

            // Finally, add the root cell's neighbors in the second dimension
            for (std::size_t dist=1; dist<=this->_nbh_distance; ++dist) {
                // front neighbor ids
                add_front_neighbor_in_dim_<2, false>(root_id, 
                                                    dist, 
                                                    front_neighbor_ids);

                // back neighbor ids
                add_back_neighbor_in_dim_<2, false>(root_id, 
                                                    dist, 
                                                    back_neighbor_ids);
            }
        }

        // Return the container of cell indices, both front and back neighbors
        front_neighbor_ids.insert(front_neighbor_ids.end(),
                                         back_neighbor_ids.begin(), 
                                         back_neighbor_ids.end());

        return front_neighbor_ids;
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
        add_neighbors_in_dim_<2, true>(root_id, neighbor_ids);
        // ...have these neighbors at indices 0 and 1 now.

        // For these neighbors, add _their_ neighbors in the first dimension
        add_neighbors_in_dim_<1, true>(neighbor_ids[0], neighbor_ids);
        add_neighbors_in_dim_<1, true>(neighbor_ids[1], neighbor_ids);

        // And finally, add the root cell's neighbors in the first dimension
        add_neighbors_in_dim_<1, true>(root_id, neighbor_ids);

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
        for (std::size_t dist=1; dist<=this->_nbh_distance; ++dist) {
            add_front_neighbor_in_dim_<1, true>(root_id, dist, neighbor_ids);
            add_back_neighbor_in_dim_<1, true>(root_id, dist, neighbor_ids);
        }

        // For these neighbors, add _their_ neighbors in the second dimension
        for (const auto& nb : neighbor_ids) {
            for (std::size_t dist=1; dist<=this->_nbh_distance; ++dist) {
                add_front_neighbor_in_dim_<2, true>(nb, dist, neighbor_ids);
                add_back_neighbor_in_dim_<2, true>(nb, dist, neighbor_ids);
            }
        }

        // And finally, add the root cell's neighbors in the second dimension
        for (std::size_t dist=1; dist<=this->_nbh_distance; ++dist) {
            add_front_neighbor_in_dim_<2, true>(root_id, dist, neighbor_ids);
            add_back_neighbor_in_dim_<2, true>(root_id, dist, neighbor_ids);
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
        for (std::size_t dist=1; dist<=this->_nbh_distance; ++dist) {
            add_front_neighbor_in_dim_<1, false>(root_id, dist, neighbor_ids);
            add_back_neighbor_in_dim_<1, false>(root_id, dist, neighbor_ids);
        }

        // For these neighbors, add _their_ neighbors in the second dimension
        for (const auto& nb : neighbor_ids) {
            for (std::size_t dist=1; dist<=this->_nbh_distance; ++dist) {
                add_front_neighbor_in_dim_<2, false>(nb, dist, neighbor_ids);
                add_back_neighbor_in_dim_<2, false>(nb, dist, neighbor_ids);
            }
        }

        // And finally, add the root cell's neighbors in the second dimension
        for (std::size_t dist=1; dist<=this->_nbh_distance; ++dist) {
            add_front_neighbor_in_dim_<2, false>(root_id, dist, neighbor_ids);
            add_back_neighbor_in_dim_<2, false>(root_id, dist, neighbor_ids);
        }

        return neighbor_ids;
    };

    // -- Neighborhood helper functions -- //

    /// Return i-dimensional shift in cell indices, depending on grid shape
    /** It returns in the different cases:
     *    - shift_dim=0 -> 1
     *    - shift_dim=1 -> shape[0]
     *    - shift_dim=2 -> shape[1] * shape[0]
     *    - shift_dim=3 -> shape[2] * shape[1] * shape[0]
     *    - ...
     * 
     * @tparam shift_dim 
     * @return constexpr GridShapeType<dim>::value_type 
     */
    template<std::size_t shift_dim>
    constexpr typename GridShapeType<dim>::value_type
        id_shift_in_dim_ () const
    {
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
     * \tparam dim         The dimensions in which to add neighbors
     * \tparam periodic    Whether the grid is periodic
     * 
     * \return void
     */
    template<std::size_t dim, bool periodic>
    void add_neighbors_in_dim_ (const IndexType& root_id,
                                IndexContainer& neighbor_ids) const
    {
        // Assure the number of dimensions is supported
        static_assert((dim == 1) or (dim == 2),
                      "Unsupported dimensionality in space! Need be 1 or 2.");

        // Conditions for the front and back boundary; the conditions are
        // dependent on the dimension in which to add neighbors.
        bool _cond_front;
        bool _cond_back;

        // Set the boundary conditions for different dimensions
        if constexpr (dim == 1) {
            _cond_front = (root_id % this->_shape[0] == 0);
            _cond_back = (root_id % this->_shape[0] == this->_shape[0] - 1);
        }
        else if constexpr (dim == 2) {
            // 'normalize' id to lowest height (in 3D)
            const auto root_id_nrm = root_id % id_shift_in_dim_<dim>();

            _cond_front = (root_id_nrm / this->_shape[0] == 0);
            _cond_back = (root_id_nrm / this->_shape[0] == this->_shape[1] - 1);
        }

        // check if at front boundary
        if (_cond_front) {
            if constexpr (periodic) {
                neighbor_ids.push_back(root_id
                                        - id_shift_in_dim_<dim-1>()
                                        + id_shift_in_dim_<dim>());
            }
        }
        else {
            neighbor_ids.push_back(root_id - id_shift_in_dim_<dim-1>());
        }

        // check if at back boundary
        if (_cond_back) {
            if constexpr (periodic) {
                neighbor_ids.push_back(root_id
                                        + id_shift_in_dim_<dim-1>()
                                        - id_shift_in_dim_<dim>());
            }
        }
        else {
            neighbor_ids.push_back(root_id + id_shift_in_dim_<dim-1>());
        }
    }


    /// Fill an index container with a front neighbor.
    /** This function takes an index container and populates it with the
     *  indices of neighboring cells in different dimensions, specified by
     *  template parameter `dim`.
     * 
     *  The algorithm first calculates whether the given root cell index has a
     *  front boundary in the chosen dimension. If so, the neighboring
     *  cell is only added if the grid is periodic.
     * 
     * \param root_id      Which cell to find the agents of
     * \param distance     Which distance the neighbor has to the root cell
     * \param neighbor_ids The container to populate with the indices
     * 
     * \tparam dim         The dimensions in which to add neighbors
     * \tparam periodic    Whether the grid is periodic
     * 
     * \return void
     */
    template<std::size_t dim, bool periodic>
    void add_front_neighbor_in_dim_ (const IndexType& root_id,
                                     const std::size_t distance,
                                     IndexContainer& neighbor_ids) const
    {
        // Assure the number of dimensions is supported
        static_assert((dim == 1) or (dim == 2),
                      "Unsupported dimensionality in space! Need be 1 or 2.");

        // If the distance is zero, no neighbor can be added; return nothing.
        if (distance == 0) {
            return;
        }

        // Conditions for the front boundary; the conditions are
        // dependent on the dimension in which to add neighbors.
        bool _cond_front;

        // Set the boundary conditions for different dimensions
        if constexpr (dim == 1) {
            _cond_front = (root_id % this->_shape[0] < distance);
        }
        else if constexpr (dim == 2) {
            _cond_front = (root_id / this->_shape[0] < distance);
        }

        // check if at front boundary
        if (_cond_front) {
            if constexpr (periodic) {
                neighbor_ids.push_back(root_id
                                        - distance * id_shift_in_dim_<dim-1>()
                                        + id_shift_in_dim_<dim>());
            }
        }
        else {
            neighbor_ids.push_back(root_id 
                                    - distance * id_shift_in_dim_<dim-1>());
        }
    }


    /// Fill an index container with a back neighbor.
    /** This function takes an index container and populates it with the
     *  index of a neighboring cell in different dimensions, specified by
     *  template parameter `dim`.
     * 
     *  The algorithm first calculates whether the given root cell index has a
     *  back boundary in the chosen dimension. If so, the neighboring
     *  cell is only added if the grid is periodic.
     * 
     * \param root_id      Which cell to find the agents of
     * \param distance     Which distance the neighbor has to the root cell
     * \param neighbor_ids The container to populate with the indices
     * 
     * \tparam dim         The dimensions in which to add neighbors
     * \tparam periodic    Whether the grid is periodic
     * 
     * \return void
     */
    template<std::size_t dim, bool periodic>
    void add_back_neighbor_in_dim_ (const IndexType& root_id,
                                    const std::size_t distance,
                                    IndexContainer& neighbor_ids) const
    {
        // Assure the number of dimensions is supported
        static_assert((dim == 1) or (dim == 2),
                      "Unsupported dimensionality in space! Need be 1 or 2.");

        // If the distance is zero, no neighbor can be added; return nothing.
        if (distance == 0) {
            return;
        }

        // Gather the required grid information
        const auto num_cells = std::accumulate(this->_shape.begin(), 
                                               this->_shape.end(), 
                                               1, 
                                               std::multiplies<std::size_t>());

        // Conditions for the back boundary; the conditions are
        // dependent on the dimension in which to add neighbors.
        bool _cond_back;

        // Set the boundary conditions for different dimensions
        if constexpr (dim == 1) {
            _cond_back  = (root_id % this->_shape[0] 
                            >= this->_shape[0] - distance);
        }
        else if constexpr (dim == 2) {
            _cond_back  = (root_id / this->_shape[0] 
                            >= this->_shape[1] - distance);
        }

        // check if at back boundary
        if (_cond_back) {
            if constexpr (periodic) {
                neighbor_ids.push_back(root_id
                                        + distance * id_shift_in_dim_<dim-1>()
                                        - id_shift_in_dim_<dim>());
            }
        }
        else {
            // NOTE Normalization by the number of cells is needed because
            //      otherwise the index could exceed the vector range.
            //      It's literally the edge case (bottom right hand corner) ;D
            neighbor_ids.push_back((root_id 
                                    + distance * id_shift_in_dim_<dim-1>()) 
                                    % num_cells);
        }
    }
};

// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRIDS_SQUARE_HH
