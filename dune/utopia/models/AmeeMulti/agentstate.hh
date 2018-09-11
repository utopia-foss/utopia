#ifndef UTOPIA_MODELS_AGENT_STATE_HH
#define UTOPIA_MODELS_AGENT_STATE_HH

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
template <typename Cell, typename Trait, typename RNG>
struct Agentstate
{
    using Phenotype = Trait;
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

    Phenotype phenotype(Phenotype& phenotype, std::vector<double>& mutationrates)
    {
        Phenotype new_genome(parent_genome);

        double substmut = mutationrates[0];
        double editmut = mutationrates[1];

        P min = *std::min_element(parent_genome.begin(), parent_genome.end());
        P max = *std::max_element(parent_genome.begin(), parent_genome.end());

        std::uniform_real_distribution<double> choice(0., 1.);
        std::uniform_real_distribution<P> values(min, max);
        std::uniform_int_distribution<std::size_t> loc(0, new_genome.size() - 1);
        if (choice(*rng) < substmut)
        {
            new_genome[loc(*rng)] = values(*rng);
        } // insert  mutation
        if (choice(*rng) < editmut)
        {
            new_genome.insert(std::next(new_genome.begin(), loc(*rng)), values(*rng));
        } // delete mutation
        if (choice(*rng) < editmut)
        {
            new_genome.erase(std::next( new_genome.begin(), loc(*rng));
        }
        return new_genome;
    }

    Agentstate() = default;
    Agentstate(Phenotype ph, std::shared_ptr<Cell> loc, std::shared_ptr<RNG> rand, int s, int e, double i)
        : rng(rand),
          start(s),
          end(e),
          intensity(i),
          phenotype(ph),
          adaption(std::vector<double>(e - s, 0.)),
          resources(0.),
          age(0),
          fitness(0),
          habitat(loc),
          deathflag(false)
    {
    }

    Agentstate(Agentstate& parent, std::vector<double>& mutationrates)
        : rng(parent.rng),
          start([&]() {
              if (std : uniform_real_distribution<>(0., 1.) < mutationrates[0])
              {
                  return std::normal_distribution<>(
                      double(parent.start), mutationrates[2])(*parent.rng);
              }
              else
              {
                  return parent.start;
              }
          }()),
          end([&]() {
              if (std : uniform_real_distribution<>(0., 1.) < mutationrates[0])
              {
                  return std::normal_distribution<>(
                      double(parent.end), mutationrates[2])(*parent.rng);
              }
              else
              {
                  return parent.end;
              }
          }()),
          intensity([&]() {
              if (std : uniform_real_distribution<>(0., 1.) < mutationrates[0])
              {
                  return std::normal_distribution<>(
                      double(parent.intensity), mutationrates[2])(*parent.rng);
              }
              else
              {
                  return parent.intensity;
              }
          }),
          phenotype(copy_genome(parent.phenotype, mutationrates)),
          adaption(std::vector<double>(e - s, 0.)),
          resources(0.),
          age(0),
          fitness(0),
          rng(rand),
          habitat(loc),
          deathflag(false)
    {
    }
};
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia

#endif