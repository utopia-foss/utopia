#ifndef UTOPIA_CORE_SPACE_HH
#define UTOPIA_CORE_SPACE_HH

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
    /// The type of the extent container
    using ExtentType = std::array<double, num_dims>;

    // -- Members -- //
    static constexpr std::size_t dim = num_dims;

    /// Whether the space is to be assumed periodic
    const bool periodic;

    /// The physical (euclidean) extent of the space
    const ExtentType extent;


    // -- Constructors -- //
    /// Construct a Space using information from a config node
    /** \param cfg  The config node to read the `periodic` and `extent` entries
      *             from.
      */
    Space(const DataIO::Config& cfg)
    :
        periodic(setup_periodic(cfg)),
        extent(setup_extent(cfg))
    {}

    /// Constructor without any arguments, i.e. constructing a default space
    /** \detail The default space is non-periodic and has default extent of 1.
      *         into each dimension.
      */
    Space()
    :
        periodic(false),
        extent(setup_extent())
    {}

private:
    // -- Setup functions ----------------------------------------------------
    /// Setup the member `periodic` from a config node
    bool setup_periodic(const DataIO::Config& cfg) {
        if (not cfg["periodic"]) {
            throw std::invalid_argument("Missing config entry `periodic` to "
                                        "set up a Space object!");
        }
        return as_bool(cfg["periodic"]);
    }

    /// Setup the extent type if no config parameter was available
    ExtentType setup_extent() {
        ExtentType extent;
        extent.fill(1.);
        return extent;
    }

    /// Setup the extent member from a config node
    /** \param cfg  The config node to read the `extent` parameter from. If
      *             that entry is missing, the default extent is used.
      */
    ExtentType setup_extent(const DataIO::Config& cfg) {
        if (cfg["extent"]) {
            return as_<ExtentType>(cfg["extent"]);
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
