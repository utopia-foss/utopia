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
/**
 * @brief
 *
 * @tparam Traittype
 */
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
    std::vector<double> resource_capacities;

    Cellstate() = default;

    /**
     * @brief Construct a new Cellstate object
     *
     * @param ct
     * @param res
     * @param res_in
     * @param res_cap
     */
    Cellstate(Trait ct, std::vector<double> res, std::vector<double> res_in, std::vector<double> res_cap)
        : celltrait(ct),
          original(ct),
          resources(res),
          resourceinfluxes(res_in),
          modtimes(std::vector<double>(ct.size(), 0.)),
          resource_capacities(res_cap)
    {
    }
};
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif