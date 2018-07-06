#ifndef UTOPIA_DATAIO_TYPES_HH
#define UTOPIA_DATAIO_TYPES_HH

#include <yaml-cpp/yaml.h>

namespace Utopia
{

namespace DataIO
{
    
    /// Type of the configuration
    using Config = YAML::Node;
    // NOTE This type is made available mainly that we can potentially change
    //      the type used for the config. If changing something here, it might
    //      still be required to explicitly change other parts of core and/or
    //      data i/o where yaml-cpp is referenced directly

} // namespace DataIO
} // Utopia

#endif // UTOPIA_DATAIO_TYPES_HH
