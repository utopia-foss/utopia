#ifndef UTOPIA_DATAIO_TYPES_HH
#define UTOPIA_DATAIO_TYPES_HH

#include "yaml-cpp/yaml.h"

namespace Utopia
{

namespace DataIO
{
    
    /// Type of the configuration
    using Config = typename YAML::Node;

} // namespace DataIO

} // Utopia

#endif // TYPES_HH