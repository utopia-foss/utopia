#ifndef UTOPIA_MODELS_AMEEMULTI_CELLSTATE_HH
#define UTOPIA_MODELS_AMEEMULTI_CELLSTATE_HH
#include <map>
#include <tuple>
#include <vector>
namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
template <typename Traittype>
struct Cellstate
{
    using Trait = Traittype;
    using T = typename Trait::value_type;

    Trait celltrait;
    Trait original;
    std::vector<double> resources;
    std::vector<double> resourceinfluxes;
    std::vector<double> modtimes;

    Cellstate() = default;
    Cellstate(Trait ct, std::vector<double> res, std::vector<double> res_in)
        : celltrait(ct),
          original(ct),
          resources(res),
          resourceinfluxes(res_in),
          modtimes(std::vector<double>(ct.size(), 0.))
    {
    }
};
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif