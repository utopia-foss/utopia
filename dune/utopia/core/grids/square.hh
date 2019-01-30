#ifndef UTOPIA_CORE_GRIDS_SQUARE_HH
#define UTOPIA_CORE_GRIDS_SQUARE_HH

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
        _shape(shape_from_resolution())
    {}


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
    GridShapeType<dim> shape_from_resolution() const {
        GridShapeType<dim> shape;

        for (std::size_t i = 0; i < dim; i++) {
            shape[i] = this->_space->extent[i] * this->_resolution;
        }

        return shape;
    }


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


    // -- Neighborhood implementations -- //
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


    // -- Neighborhood helper functions -- //

    /// Return i-dimensional shift in cell indices, depending on grid shape
    template<std::size_t shift_dim>
    constexpr typename GridShapeType<dim>::value_type id_shift_in_dim_ () {
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
    template<std::size_t dim, bool periodic>
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
