#ifndef UTOPIA_MODELS_CELLSTATE_HH
#define UTOPIA_MODELS_CELLSTATE_HH
#include <vector>

namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
template <typename Trait>
struct Cellstate
{
    using Celltrait = Trait;
    Celltrait celltraits;
    std::vector<double> resources;
    std::vector<double> resource_influxes;
    Celltrait original;
    std::vector<double> modtimes;

    Cellstate() = default;
    Cellstate(Celltrait ct, std::vector<double> res, std::vector<double> ri)
        : celltraits(ct),
          resources(res),
          resource_influxes(ri),
          original(ct),
          modtimes(std::vector<double>(ct.size(), 0.))
    {
    }
};
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif