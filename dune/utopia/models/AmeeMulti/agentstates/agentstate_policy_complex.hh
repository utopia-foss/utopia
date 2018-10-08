#ifndef UTOPIA_MODELS_AMEEMULTI_AGENTSTATEPOLICY_COMPLEX_HH
#define UTOPIA_MODELS_AMEEMULTI_AGENTSTATEPOLICY_COMPLEX_HH

#include "agentstate_policy_simple.hh"
namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
template <class Gt, class Pt, class PRNG>
struct Agentstate_policy_complex : Agentstate_policy_simple<Gt, Pt, PRNG>
{
    using Phenotype = Pt;
    using Genotype = Gt;
    using RNG = PRNG;
    using P = typename Pt::value_type;
    using G = typename Gt::value_type;
    /**
     * @brief Function for computing a value from a range [s,e) in the genome
     * - analog to biological codon-aminoacid map
     *
     * @param s start for computation
     * @param e end for computation
     * @return P value of type Phenotype::value_type
     */
protected:
    P get_codon_value(int s, int e, double divisor, Genotype& genotype)
    {
        if (std::abs(divisor) < 1e-16 or s < 0 or e < s)
        {
            return 0.;
        }
        else
        {
            return std::accumulate(genotype.begin() + s, genotype.begin() + e, 0.) / divisor;
        }
    }

    Phenotype translate_genome(unsigned sumlen, double divisor, Genotype& genotype)
    {
        if (sumlen == 0 || std::abs(divisor) < 1e-16)
        {
            return Phenotype();
        }

        // build phenotype
        std::size_t size = genotype.size();
        Phenotype phenotype;
        phenotype.reserve(size + 1);

        std::size_t i = 4 + 5 * sumlen;
        // run over genotype and get codon elements
        for (; i < size - sumlen; i += sumlen)
        {
            phenotype.push_back(get_codon_value(i, i + sumlen, divisor, genotype));
        }

        phenotype.push_back(get_codon_value(
            i, (i + sumlen) <= size ? (i + sumlen) : genotype.size(), divisor, genotype));
        return phenotype;
    }

public:
    /**
     * @brief function for copying the genotype upon reproduction, includes mutations
     *
     * @param parent_genome The genome of the parent
     * @param mutationrates the mutationrates used
     * @return Genotype New genotype, possibly mutated
     */
    virtual Genotype copy_genome(Genotype& parent_genome,
                                 std::vector<double>& mutationrates,
                                 std::shared_ptr<RNG> rng) override
    {
        if (parent_genome.size() == 0)
        {
            return Genotype();
        }
        Genotype new_genome(parent_genome);

        double substmut = mutationrates[0];
        double editmut = mutationrates[1];

        auto min = *std::min_element(parent_genome.begin(), parent_genome.end());
        auto max = *std::max_element(parent_genome.begin(), parent_genome.end());
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
            new_genome.erase(std::next(new_genome.begin(), loc(*rng)));
        }
        return new_genome;
    }

    /**
     * @brief Function for translateing genotype to phenotype
     *
     * @param genotype
     * @return std::tuple<unsigned, double, int, int, int, int, double, Phenotype>
     */
    virtual std::tuple<unsigned, double, int, int, int, int, double, Phenotype> genotype_phenotype_map(
        Genotype& genotype) override
    {
        unsigned sumlen = 0;
        double divisor = 0.;
        int start = 0;
        int end = 0;
        int startmod = 0;
        int endmod = 0;
        double intensity = 0.;
        Phenotype phenotype;
        if (genotype.size() < 4)
        {
            return std::make_tuple(sumlen, divisor, start, end, startmod,
                                   endmod, intensity, phenotype);
        }
        else
        {
            auto sl = std::round(genotype[0] + genotype[2]);
            if (sl < 0)
            {
                sumlen = 0;
            }
            else
            {
                sumlen = sl;
            }

            if (genotype.size() < (4 + 6 * sumlen))
            {
                return std::make_tuple(sumlen, divisor, start, end, startmod,
                                       endmod, intensity, phenotype);
            }
            else
            {
                divisor = static_cast<double>(genotype[1] + genotype[3]);

                start = std::round(get_codon_value(4, 4 + sumlen, divisor, genotype));

                end = std::round(get_codon_value(4 + sumlen, 4 + 2 * sumlen, divisor, genotype));

                startmod = std::round(get_codon_value(
                    4 + 2 * sumlen, 4 + 3 * sumlen, divisor, genotype));

                endmod = std::round(get_codon_value(
                    4 + 3 * sumlen, 4 + 4 * sumlen, divisor, genotype));

                intensity = get_codon_value(4 + 4 * sumlen, 4 + 5 * sumlen, divisor, genotype);

                phenotype = translate_genome(sumlen, divisor, genotype);

                return std::make_tuple(sumlen, divisor, start, end, startmod,
                                       endmod, intensity, phenotype);
            }
        }
    }
};
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia

#endif