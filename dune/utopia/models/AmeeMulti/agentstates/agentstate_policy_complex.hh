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
struct Agentstate_policy_complex : public Agentstate_policy_simple<Gt, Pt, PRNG>
{
    using Phenotype = Pt;
    using Genotype = Gt;
    using RNG = PRNG;
    using P = typename Phenotype::value_type;

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
            phenotype.push_back(this->get_codon_value(i, i + sumlen, divisor, genotype));
        }
        // std::cout << "size = " << size << std::endl;
        // std::cout << " i   = " << i << std::endl;
        phenotype.push_back(this->get_codon_value(
            i, (i + sumlen) <= size ? (i + sumlen) : genotype.size(), divisor, genotype));
        return phenotype;
    }

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

                start = std::round(this->get_codon_value(4, 4 + sumlen, divisor, genotype));

                end = std::round(this->get_codon_value(
                    4 + sumlen, 4 + 2 * sumlen, divisor, genotype));

                startmod = std::round(this->get_codon_value(
                    4 + 2 * sumlen, 4 + 3 * sumlen, divisor, genotype));

                endmod = std::round(this->get_codon_value(
                    4 + 3 * sumlen, 4 + 4 * sumlen, divisor, genotype));

                intensity = this->get_codon_value(
                    4 + 4 * sumlen, 4 + 5 * sumlen, divisor, genotype);

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