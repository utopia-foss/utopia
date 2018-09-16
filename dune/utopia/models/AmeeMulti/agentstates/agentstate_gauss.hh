#ifndef UTOPIA_MODELS_AMEEMULTI_AGENTSTATE_GAUSS_HH
#define UTOPIA_MODELS_AMEEMULTI_AGENTSTATE_GAUSS_HH
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
template <typename Cell, typename PT, typename RNG>
struct AgentStateGauss
{
    using Phenotype = PT;
    using Genotype = PT;

    using P = typename Phenotype::value_type;

    std::shared_ptr<RNG> rng;
    int start;
    int end;
    double intensity;
    Phenotype phenotype;
    std::vector<double> adaption;
    double resources;
    unsigned age;
    unsigned fitness;
    std::shared_ptr<Cell> habitat;
    bool deathflag;

    Phenotype copy_phenotype(Phenotype& parent_phenotype, std::vector<double>& mutationrates)
    {
        // std::cout << "   copying phenotype" << std::endl;
        Phenotype new_phenotype(parent_phenotype);
        // std::cout << "    new phenotype size, mutationrates size: " << new_phenotype.size()
        // << "," << mutationrates.size() << std::endl;
        double substmut = mutationrates[0];
        double editmut = mutationrates[1];
        double subststd = mutationrates[2];

        // std::cout << "   min and max" << std::endl;
        P min = *std::min_element(parent_phenotype.begin(), parent_phenotype.end());
        P max = *std::max_element(parent_phenotype.begin(), parent_phenotype.end());

        // std::cout << "    " << min << "," << max << std::endl;

        std::uniform_real_distribution<double> choice(0., 1.);
        std::uniform_real_distribution<P> values(min, max);
        std::uniform_int_distribution<std::size_t> loc(0, new_phenotype.size() - 1);
        // std::cout << "   rng: " << rng.get() << std::endl;
        if (choice(*rng) < substmut)
        {
            // std::cout << "    substitution" << std::endl;
            new_phenotype[loc(*rng)] =
                std::normal_distribution<P>(new_phenotype[loc(*rng)], subststd)(*rng);
        } // insert  mutation
        if (choice(*rng) < editmut)
        {
            // std::cout << "    insertion" << std::endl;
            new_phenotype.insert(std::next(new_phenotype.begin(), loc(*rng)),
                                 values(*rng));
        } // delete mutation
        if (choice(*rng) < editmut)
        {
            // std::cout << "    deletion" << std::endl;
            new_phenotype.erase(std::next(new_phenotype.begin(), loc(*rng)));
        }
        // std::cout << "returning " << std::endl;
        return new_phenotype;
    }

    AgentStateGauss() = default;
    AgentStateGauss(Phenotype init_ptype, std::shared_ptr<Cell> loc, double res, std::shared_ptr<RNG> rnd)
        : rng(rnd),
          start(0),
          end(0),
          intensity(0),
          phenotype(init_ptype),
          adaption(std::vector<double>()),
          resources(res),
          age(0),
          fitness(0),
          habitat(loc),
          deathflag(false)
    {
    }

    AgentStateGauss(AgentStateGauss& parent, double offspringres, std::vector<double>& mutationrates)
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
          phenotype(copy_phenotype(parent.phenotype, mutationrates)),
          adaption(std::vector<double>(end - start, 0.)),
          resources(offspringres),
          age(0),
          fitness(0),
          habitat(parent.habitat),
          deathflag(false)
    {
        if (end < start)
        {
            end = 0;
            start = 0;
            adaption = std::vector<double>();
        }
    }
};
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif
