#ifndef UTOPIA_MODELS_AMEEMULTI_AGENTSTATEPOLICY_COMPLEX_HH
#define UTOPIA_MODELS_AMEEMULTI_AGENTSTATEPOLICY_COMPLEX_HH

#include "agentstate_policy_simple.hh"
namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
template <class Genotype, class Phenotype, class RNG>
struct Agentstate_policy_complex
    : public Agentstate_policy_simple<Genotype, Phenotype, RNG>
{
    virtual Genotype copy_genome(Genotype& parent_genome,
                                 std::vector<double>& mutationrates,
                                 std::shared_ptr<RNG> rng) override
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
                std::normal_distribution<>(parent_genome[where], std)(*rng);
        }
        if (choice(*rng) < editmut)
        {
            new_genotype.insert(std::next(new_genotype.begin(), loc(rng), values(*rng)));
        }
        if (choice(*rng) < editmut)
        {
            new_genotype.erase(std::next(new_genotype.begin(), loc(*rng)));
        }
        return new_genotype;
    }

    virtual auto genotype_phenotype_map(Genotype& genotype) override
    {
        unsigned sumlen = 0;
        double divisor = 0.;
        int start = 0;
        int end = 0;
        double intensity = 0.;
        Phenotype phenotype;

        if (genotype.size() < 4)
        {
            start = 0;
            end = 0;
            intensity = 0.;
            phenotype = Phenotype();
        }
        else
        {
            start = genotype[0];
            end = genotype[1];
            intensity = genotype[2];
            phenotype = Phenotype(genotype.begin(), genotype.end());
        }

        return std::make_tuple(sumlen, divisor, start, end, intensity, phenotype);
    }
};
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia

#endif