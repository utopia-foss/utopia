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
/** \details It is, for example, used by the CellManager and its Grid
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
    using SpaceVec = SpaceVecType<dim>;

    /// Whether the space is to be assumed periodic
    const bool periodic;

    /// The physical (euclidean) extent of the space
    const SpaceVec extent;


    // -- Constructors --------------------------------------------------------
    /// Construct a Space using information from a config node
    /** \param cfg  The config node to read the `periodic` and `extent` entries
      *             from.
      */
    Space(const Config& cfg)
    :
        periodic(setup_periodic(cfg)),
        extent(setup_extent(cfg))
    {
        static_assert(dim > 0, "Space::dim needs to be >= 1");
    }

    /// Constructor without any arguments, i.e. constructing a default space
    /** \details The default space is non-periodic and has default extent of 1.
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
    /** \details Checks whether the given coordinate is within this space's
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
    bool contains(const SpaceVec& pos) const {
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
    /** This is intended for use with periodic space. It will also work with
      * non-periodic space, but the input value should not have been permitted
      * in the first place.
      *
      * \note   The high-value boundary is is mapped back to the low-value
      *         boundary, such that all points are well-defined.
      */
    SpaceVec map_into_space(const SpaceVec& pos) const {
        // If it is already within space, nothing to map
        // Check whether it is contained (_excluding_ high value boundary)
        if (contains<false>(pos)) {
            return pos;
        }
        // else: Need to transform back into space

        return pos - arma::floor(pos / extent) % extent;
    }

    
    /// Compute the displacement vector between two coordinates
    /** Calculates vector pointing from pos_0 to pos_1.
     *  In periodic space, it calculates the shorter displacement.
     *  
     *  \warning The displacement of two coordinates in periodic boundary can
     *           be maximum of half the domain size, i.e. moving away from a
     *           coordinate in a certain direction will decrease the
     *           displacement once reached half the domain size.
     */
    SpaceVec displacement(const SpaceVec& pos_0, const SpaceVec& pos_1) const {
        SpaceVec dx = pos_1 - pos_0;

        if (not periodic) {
            return dx;
        }
        // else: Need to get shortest distance

        return dx - arma::round(dx / extent) % extent;
    }

    /// The distance of two coordinates in space
    /** Calculates the distance of two coordinates using the norm implemented
     *  within Armadillo.
     *  In periodic boundary it calculates the shorter distance.
     * 
     *  \warning The distance of two coordinates in periodic boundary can be
     *           maximum of half the domain size wrt every dimension,
     *           i.e. moving away from a coordinate in a certain direction will
     *           decrease the distance once reached half the domain size.
     * 
     *  \param   p   The norm used to compute the distance, see arma::norm.
     *               Can be either an integer >= 1 or one of:
     *               "-inf", "inf", "fro"
     */
    template<class NormType=std::size_t>
    auto distance(const SpaceVec& pos_0, const SpaceVec& pos_1,
                  const NormType p=2) const
    {
        return arma::norm(displacement(pos_0, pos_1), p);
    }


private:
    // -- Setup functions -----------------------------------------------------
    /// Setup the member `periodic` from a config node
    bool setup_periodic(const Config& cfg) {
        if (not cfg["periodic"]) {
            throw std::invalid_argument("Missing config entry `periodic` to "
                                        "set up a Space object!");
        }
        return get_as<bool>("periodic", cfg);
    }

    /// Setup the extent type if no config parameter was available
    SpaceVec setup_extent() {
        SpaceVec extent;
        extent.fill(1.);
        return extent;
    }

    /// Setup the extent member from a config node
    /** \param cfg  The config node to read the `extent` parameter from. If
      *             that entry is missing, the default extent is used.
      */
    SpaceVec setup_extent(const Config& cfg) {
        if (cfg["extent"]) {
            return get_as_SpaceVec<dim>("extent", cfg);
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
