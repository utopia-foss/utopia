#ifndef UTOPIA_CORE_SPACE_HH
#define UTOPIA_CORE_SPACE_HH

#include "../data_io/cfg_utils.hh"
#include "types.hh"


namespace Utopia {
/**
 *  \addtogroup Model
 *  \{
 */

template<DimType dim>
class Space {
public:
    /// The type of the extent container
    using ExtentType = std::array<double, dim>;


    // -- Members -- //
    const bool periodic;
    const ExtentType extent;


    // -- Constructors -- //
    /// Constructor via config
    Space(const DataIO::Config& cfg)
    :
        periodic(as_bool(cfg["periodic"])),
        extent(setup_extent(cfg))
    {}

    /// Constructor without any arguments, i.e. constructing a default space
    Space()
    :
        periodic(false),
        extent(setup_extent())
    {}

private:
    // -- Setup functions ----------------------------------------------------
    /// Setup the extent type if no config parameter was available
    ExtentType setup_extent() {
        ExtentType extent;
        extent.fill(1);
        return extent;
    }

    /// Setup the extent member from a config node
    ExtentType setup_extent(const DataIO::Config& cfg) {
        ExtentType extent;

        if (!cfg["extent"]) {
            extent.fill(1.);
        }
        else {
            extent = as_<ExtentType>(cfg["extent"]);
        }

        return extent;
    }
};


/// The default space has dimensionality 2
using DefaultSpace = Space<2>;

// end group Model
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_SPACE_HH
