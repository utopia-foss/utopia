#ifndef UTOPIA_CORE_SPACE_HH
#define UTOPIA_CORE_SPACE_HH

#include <cmath>

#include <armadillo>

#include "../data_io/cfg_utils.hh"
#include "types.hh"


namespace Utopia {
/**
 *  \addtogroup Model
 *  \{
 */

/// The Space bundles properties about the physical space a model resides in
/** \detail It is, for example, used by the CellManager and its Grid
  *         discretization.
  * 
  * \tparam num_dims  The dimensionality of the space
  */
template<std::size_t num_dims>
struct Space {
    // -- Members -------------------------------------------------------------
    /// The dimensionality of the space
    static constexpr std::size_t dim = num_dims;

    /// The type for vectors relating to physical space
    using PhysVector = PhysVectorType<dim>;

    /// Whether the space is to be assumed periodic
    const bool periodic;

    /// The physical (euclidean) extent of the space
    const PhysVector extent;


    // -- Constructors --------------------------------------------------------
    /// Construct a Space using information from a config node
    /** \param cfg  The config node to read the `periodic` and `extent` entries
      *             from.
      */
    Space(const DataIO::Config& cfg)
    :
        periodic(setup_periodic(cfg)),
        extent(setup_extent(cfg))
    {
        static_assert(dim > 0, "Space::dim needs to be >= 1");
    }

    /// Constructor without any arguments, i.e. constructing a default space
    /** \detail The default space is non-periodic and has default extent of 1.
      *         into each dimension.
      */
    Space()
    :
        periodic(false),
        extent(setup_extent())
    {
        static_assert(dim > 0, "Space::dim needs to be >= 1");
    }


    // -- Public interface ----------------------------------------------------
    /// Whether this space contains the given coordinate (without mapping it)
    /** \detail Checks whether the given coordinate is within this space's
      *         extent by computing the relative position and checking whether
      *         it is within [0, 1] or [0, 1) for all elements.
      *
      * \note   No distinction is made between periodic and non-periodic space.
      *         
      * \tparam  include_high_value_boundary  Whether to check the closed or
      *         the half-open interval. The latter case is useful when working
      *         with periodic grids, allowing to map values on the high-value
      *         boundary back to the low-value boundary.
      */
    template<bool include_high_value_boundary=true>
    bool contains(const PhysVector& pos) const {
        // Calculate the relative position within the space
        const auto rel_pos = pos / extent;  // element-wise division

        // Check that all relative positions are in the respective interval
        if constexpr (include_high_value_boundary) {
            return arma::all(rel_pos >= 0. and rel_pos <= 1.);  // [0, 1]
        }
        else {
            return arma::all(rel_pos >= 0. and rel_pos <  1.);  // [0, 1)
        }
    }

    /// Map a position (potentially outside space's extent) back into space
    /** \detail This is intended for use with periodic grids. It will also work
      *         with non-periodic grids, but the input value should not have
      *         been permitted in the first place.
      *
      * \note   The high-value boundary is is mapped back to the low-value
      *         boundary, such that all points are well-defined.
      */
    PhysVector map_into_space(const PhysVector& pos) const {
        // If it is already within space, nothing to map
        // Check whether it is contained (_excluding_ high value boundary)
        if (contains<false>(pos)) {
            return pos;
        }
        // else: Need to transform back into space

        // Mapped position vector
        auto mpos = pos;
        std::cout << "before transform:" << std::endl << mpos;
        
        // Loop over position and extent and apply binary operation, storing
        // it in mpos
        std::transform(pos.begin(), pos.end(), extent.begin(), mpos.begin(),
            [](const double& p, const double& e){
                // Given the position in one dimension and the corresponding
                // extent, calculate the remainder of the relative position,
                // which can be in range [-0.5, 0.5]
                const double rem = std::remainder(p/e, 1.);
                std::cout.precision(17);
                std::cout << "remainder: " << rem << " \t";

                // Distinguish by sign of remainder calculation
                if (rem < 0.) {
                    std::cout << "new pos: " << (1. + rem) * e << std::endl;
                    return (1. + rem) * e;
                }
                else {
                    std::cout << "new pos: " << rem * e << std::endl;
                    return rem * e;
                }
            });

        std::cout << "after transform:" << std::endl << mpos << std::endl;
        return mpos;
    }


private:
    // -- Setup functions -----------------------------------------------------
    /// Setup the member `periodic` from a config node
    bool setup_periodic(const DataIO::Config& cfg) {
        if (not cfg["periodic"]) {
            throw std::invalid_argument("Missing config entry `periodic` to "
                                        "set up a Space object!");
        }
        return as_bool(cfg["periodic"]);
    }

    /// Setup the extent type if no config parameter was available
    PhysVector setup_extent() {
        PhysVector extent;
        extent.fill(1.);
        return extent;
    }

    /// Setup the extent member from a config node
    /** \param cfg  The config node to read the `extent` parameter from. If
      *             that entry is missing, the default extent is used.
      */
    PhysVector setup_extent(const DataIO::Config& cfg) {
        if (cfg["extent"]) {
            return as_PhysVector<dim>(cfg["extent"]);
        }

        // Return the default extent
        return setup_extent();
    }
};


/// The default Space object to be used throughout Utopia
using DefaultSpace = Space<2>;

// end group Model
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_SPACE_HH
