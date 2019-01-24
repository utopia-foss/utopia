#ifndef UTOPIA_CORE_GRIDS_RECTANGULAR_HH
#define UTOPIA_CORE_GRIDS_RECTANGULAR_HH

#include "base.hh"

namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

/// A grid discretization using rectangular cells
template<class Space>
class RectangularGrid
    : public Grid<Space>
{
public:
    /// Base class type
    using Base = Grid<Space>;

    /// The dimensionality of the space to be discretized (for easier access)
    static constexpr DimType dim = Space::dim;

    /// The type of the grid shape array
    using GridShape = GridShapeType<dim>;

private:
    // -- RectangularGrid-specific members -- //

public:
    /// Construct a rectangular grid discretization
    RectangularGrid (std::shared_ptr<Space> space,
                     const GridShape shape)
    :
        Base(space, shape)
    {}


protected:
    // -- Custom implementations of virtual base class functions -- //

    /// Calculate the number of cells required to fill the current grid shape
    IndexType calc_num_cells() {
        return std::accumulate(this->_shape.begin(), this->_shape.end(),
                               1, std::multiplies<IndexType>());
    };

    // -- Neighborhood implementations -- //

    /// Retrieve the neighborhood function depending on the mode
    NBFuncID<Base> get_nb_func(NBMode nb_mode) {
        if (nb_mode == NBMode::empty) {
            return this->_nb_empty;
        }
        else if (nb_mode == NBMode::vonNeumann) {
            return _nb_vonNeumann;
        }
        else {
            throw std::invalid_argument("No '" + nb_mode_to_string.at(nb_mode)
                + "' available for rectangular grid discretization!");
        }
    }

    NBFuncID<Base> _nb_vonNeumann = [this](const IndexType& root_id) {
        static_assert((dim >= 1 and dim <= 3),
            "VonNeumann neighborhood is only implemented in 1-3 dimensions!");
        
        // Instantiate container in which to store the neighboring cell IDs
        IndexContainer neighbor_ids{};

        // The number of neighbors is known; pre-allocating space brings a
        // speed improvemet of about factor 2
        neighbor_ids.reserve(2 * dim);

        // Depending on the number of dimensions, add the IDs of neighboring
        // cells in those dimensions
        add_neighbors_in_dim_<1>(root_id, neighbor_ids);

        if constexpr (dim >= 2) {
            add_neighbors_in_dim_<2>(root_id, neighbor_ids);
        }

        if constexpr (dim >= 3) {
            add_neighbors_in_dim_<3>(root_id, neighbor_ids);
        }
        // ... _could_ continue here

        // Return the container of cell indices
        return neighbor_ids;
    };


    // -- Neighborhood helper functions -- //

    /// Return i-dimensional shift in cell indices, depending on grid shape
    template<DimType shift_dim>
    constexpr typename GridShape::value_type id_shift_in_dim_ () {
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
     * 
     * \return void
     */
    template<DimType dim>
    void add_neighbors_in_dim_ (const IndexType& root_id,
                                IndexContainer& neighbor_ids)
    {
        // Assure the number of dimesions is supported
        static_assert(dim >= 1 and dim <= 3,
                      "Unsupported dimensionality! Need be 1, 2, or 3.");

        // Gather the grid information needed
        const bool periodic = this->is_periodic();
        const auto& shape = this->shape();

        // Distinguish by dimension parameter
        if constexpr (dim == 1) {
            // check if at front boundary
            if (root_id % shape[0] == 0) {
                if (periodic) {
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
                if (periodic) {
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
            // TODO Check what the cast is needed for and why this is not the same as with the other dimensions
            if ((long) root_id_nrm / shape[0] == 0) {
                if (periodic) {
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
                if (periodic) {
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
                if (periodic) {
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
                if (periodic) {
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

#endif // UTOPIA_CORE_GRIDS_RECTANGULAR_HH
