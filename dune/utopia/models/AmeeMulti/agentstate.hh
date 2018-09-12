#ifndef UTOPIA_MODELS_AMEEMULTI_AGENTSTATE_HH
#define UTOPIA_MODELS_AMEEMULTI_AGENTSTATE_HH
#include <memory>
#include <random>
#include <vector>

namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
/**
 * @brief agentstate
 *
 * @tparam Cell
 * @tparam Gtype
 * @tparam Ptype
 * @tparam RNG
 */
template <typename Cell, typename Traittype, typename RNG>
struct Agentstate
{
    using Trait = Traittype;
    using T = typename Trait::value_type;

    std::shared_ptr<RNG> rng;
    int start;
    int end;
    double intensity;
    Trait trait;
    std::vector<double> adaption;
    double resources;
    unsigned age;
    unsigned fitness;
    std::shared_ptr<Cell> habitat;
    bool deathflag;

    Trait copy_trait(Trait& parent_trait, std::vector<double>& mutationrates)
    {
        Trait new_trait(parent_trait);

        double substmut = mutationrates[0];
        double editmut = mutationrates[1];
        double subststd = mutationrates[2];
        T min = *std::min_element(parent_trait.begin(), parent_trait.end());
        T max = *std::max_element(parent_trait.begin(), parent_trait.end());

        std::uniform_real_distribution<double> choice(0., 1.);
        std::uniform_real_distribution<T> values(min, max);
        std::uniform_int_distribution<std::size_t> loc(0, new_trait.size() - 1);

        if (choice(*rng) < substmut)
        {
            new_trait[loc(*rng)] =
                std::normal_distribution<T>(new_trait[loc(*rng)], subststd)(*rng);
        } // insert  mutation
        if (choice(*rng) < editmut)
        {
            new_trait.insert(std::next(new_trait.begin(), loc(*rng)), values(*rng));
        } // delete mutation
        if (choice(*rng) < editmut)
        {
            new_trait.erase(std::next(new_trait.begin(), loc(*rng)));
        }
        return new_trait;
    }

    Agentstate() = default;
    Agentstate(Trait trt, std::shared_ptr<Cell> loc, double res, std::shared_ptr<RNG> rnd, int s, int e, double i)
        : rng(rnd),
          start(s),
          end(e),
          intensity(i),
          trait(trt),
          adaption(std::vector<double>(e - s, 0.)),
          resources(res),
          age(0),
          fitness(0),
          habitat(loc),
          deathflag(false)
    {
    }

    Agentstate(Agentstate& parent, double offspringres, std::vector<double>& mutationrates)
        : rng(parent.rng),
          start([&]() -> int {
              if (std::uniform_real_distribution<>(0., 1.)(*this->rng) < mutationrates[0])
              {
                  return std::round(std::normal_distribution<>(
                      double(parent.start), mutationrates[2])(*parent.rng));
              }
              else
              {
                  return parent.start;
              }
          }()),
          end([&]() -> int {
              if (std::uniform_real_distribution<>(0., 1.)(*this->rng) < mutationrates[0])
              {
                  return std::round(std::normal_distribution<>(
                      double(parent.end), mutationrates[2])(*parent.rng));
              }
              else
              {
                  return parent.end;
              }
          }()),
          intensity([&]() -> double {
              if (std::uniform_real_distribution<>(0., 1.)(*this->rng) < mutationrates[0])
              {
                  return std::normal_distribution<>(
                      double(parent.intensity), mutationrates[2])(*parent.rng);
              }
              else
              {
                  return parent.intensity;
              }
          }()),
          trait(copy_trait(parent.trait, mutationrates)),
          adaption(std::vector<double>(end - start, 0.)),
          resources(offspringres),
          age(0),
          fitness(0),
          habitat(parent.habitat),
          deathflag(false)
    {
    }
};
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif
