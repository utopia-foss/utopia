#ifndef UTOPIA_MODELS_AMEEMULTI_AGENTSTATEPOLICY_SIMPLE_HH
#define UTOPIA_MODELS_AMEEMULTI_AGENTSTATEPOLICY_SIMPLE_HH
#include <cmath>
#include <numeric>
#include <random>
#include <vector>

namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
/**
 * @brief Simple agentstate in which there is no genotype-phenotype distinction
 *
 * @tparam Gt
 * @tparam Pt
 * @tparam PRNG
 */
template <class Gt, class Pt, class PRNG>
struct Agentstate_policy_simple
{
    using Phenotype = Pt;
    using Genotype = Gt;
    using RNG = PRNG;
    using P = typename Pt::value_type;
    using G = typename Gt::value_type;

    /**
     * @brief Copy function for genome
     *
     * @param parent_genome reference to parent's genome
     * @param mutationrates  mutation rates vector
     * @param rng  shared pointer to random number generator
     * @return Genotype  New, possibly mutated genotype
     */
    virtual Genotype copy_genome(Genotype& parent_genome,
                                 std::vector<double>& mutationrates,
                                 std::shared_ptr<RNG> rng)
    {
        using G = typename Genotype::value_type;
        if constexpr (!std::is_floating_point_v<G>)
        {
            throw std::runtime_error(
                "genotype needs to hold floating point values");
        }

        if (parent_genome.size() == 0)
        {
            return Genotype();
        }

        Genotype new_genotype = parent_genome;
        auto min = *std::min_element(parent_genome.begin(), parent_genome.end());
        auto max = *std::max_element(parent_genome.begin(), parent_genome.end());

        double substmut = mutationrates[0];
        double editmut = mutationrates[1];
        double std = mutationrates[2];
        std::uniform_real_distribution<double> choice(0., 1.);
        std::uniform_real_distribution<P> values(min, max);
        std::uniform_int_distribution<std::size_t> loc(0, new_genotype.size() - 1);
        // mutate new genotype
        if (choice(*rng) < substmut)
        {
            auto where = loc(*rng);
            new_genotype[where] =
                std::normal_distribution<double>(parent_genome[where], std)(*rng);
        }
        if (choice(*rng) < editmut)
        {
            new_genotype.insert(std::next(new_genotype.begin(), loc(*rng)), values(*rng));
        }
        if (choice(*rng) < editmut)
        {
            new_genotype.erase(std::next(new_genotype.begin(), loc(*rng)));
        }
        return new_genotype;
    }

    /**
     * @brief Genotype phenotype map function
     *
     * @param genotype
     * @return std::tuple<unsigned, double, int, int, int, int, double, Phenotype>
     */
    virtual std::tuple<unsigned, double, int, int, int, int, double, Phenotype> genotype_phenotype_map(Genotype& genotype)
    {
        unsigned sumlen = 0;
        double divisor = 0.;
        int start = 0;
        int end = 0;
        int startmod = 0;
        int endmod = 0;
        double intensity = 0.;
        Phenotype phenotype;

        if (genotype.size() < 6)
        {
            return std::make_tuple(sumlen, divisor, start, end, startmod,
                                   endmod, intensity, phenotype);
        }
        else
        {
            start = genotype[0];
            end = genotype[1];
            startmod = genotype[2];
            endmod = genotype[3];
            intensity = genotype[4];
            phenotype = Phenotype(genotype.begin() + 5, genotype.end());

            return std::make_tuple(sumlen, divisor, start, end, startmod,
                                   endmod, intensity, phenotype);
        }
    }
};
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif