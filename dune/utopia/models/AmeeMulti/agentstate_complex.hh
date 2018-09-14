#ifndef UTOPIA_MODELS_AMEEMULTI_AGENT_STATE_COMPLEX_HH
#define UTOPIA_MODELS_AMEEMULTI_AGENT_STATE_COMPLEX_HH
#include "agentstate_base.hh"
namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
template <typename Cell, typename Gtype, typename Ptype, typename RNG>
struct AgentStateComplex : public AgentStateBase<Cell, Gtype, Ptype, RNG>
{
    using Base = AgentStateBase<Cell, Gtype, Ptype, RNG>;
    using Phenotype = typename Base::Phenotype;
    using Genotype = typename Base::Genotype;

    Phenotype translate_genome()
    {
        if (this->sumlen == 0 || std::abs(this->divisor) < 1e-16)
        {
            return typename Base::Phenotype();
        }

        // build phenotype
        std::size_t size = this->genotype.size();
        typename Base::Phenotype phenotype;
        phenotype.reserve(size + 1);

        std::size_t i = 4 + 3 * this->sumlen;
        // run over genotype and get codon elements
        for (; i < size - this->sumlen; i += this->sumlen)
        {
            phenotype.push_back(this->get_codon_value(i, i + this->sumlen));
        }
        // std::cout << "size = " << size << std::endl;
        // std::cout << " i   = " << i << std::endl;
        phenotype.push_back(this->get_codon_value(
            i, (i + this->sumlen) <= size ? (i + this->sumlen) : this->genotype.size()));
        return phenotype;
    }

    void genotype_phenotype_map()
    {
        if (this->genotype.size() < 4)
        {
            this->sumlen = 0;
            this->divisor = 0;
            this->start = 0;
            this->end = 0;
            this->intensity = 0.;
            this->phenotype = Phenotype();
        }

        else
        {
            auto sl = std::round(this->genotype[0] + this->genotype[2]);
            if (sl < 0)
            {
                this->sumlen = 0;
            }
            else
            {
                this->sumlen = sl;
            }

            if (this->genotype.size() < (4 + 4 * this->sumlen))
            {
                this->sumlen = 0;
                this->divisor = 0;
                this->start = 0;
                this->end = 0;
                this->intensity = 0.;
                this->phenotype = Phenotype();
            }
            else
            {
                this->divisor =
                    static_cast<double>(this->genotype[1] + this->genotype[3]);

                this->start = std::round(this->get_codon_value(4, 4 + this->sumlen));
                this->end = std::round(this->get_codon_value(
                    4 + this->sumlen, 4 + 2 * this->sumlen));

                this->intensity =
                    this->get_codon_value(4 + 2 * this->sumlen, 4 + 3 * this->sumlen);
                // if (this->intensity < 0.)
                // {
                //     this->intensity = 0.;
                // }
                this->phenotype = translate_genome();
            }
        }
    }

    /**
     * @brief      function for swapping the state of the caller and the
     * argument
     *
     * @param      other  The other
     */
    void swap(AgentStateComplex& other)
    {
        if (this == &other)
        {
            return;
        }
        else
        {
            using std::swap;
            swap(this->genotype, other.genotype);
            swap(this->phenotype, other.phenotype);
            swap(this->resources, other.resources);
            swap(this->fitness, other.fitness);
            swap(this->sumlen, other.sumlen);
            swap(this->divisor, other.divisor);
            swap(this->start, other.start);
            swap(this->end, other.end);
            swap(this->adaption, other.adaption);
            swap(this->intensity, other.intensity);
            swap(this->age, other.age);
            swap(this->habitat, other.habitat);
            swap(this->deathflag, other.deathflag);
            swap(this->rng, other.rng);
        }
    }

    AgentStateComplex() = default;
    AgentStateComplex(const AgentStateComplex& other) = default;
    AgentStateComplex(AgentStateComplex&& other) = default;
    AgentStateComplex& operator=(AgentStateComplex&& other) = default;
    AgentStateComplex& operator=(const AgentStateComplex& other) = default;
    virtual ~AgentStateComplex() = default;

    /**
     * @brief Construct a new AgentStateBase_Complex object
     *
     * @param init_genotype
     * @param cell
     * @param init_resources
     * @param randomgenerator
     */
    AgentStateComplex(typename Base::Genotype init_genotype,
                      std::shared_ptr<Cell> cell,
                      double init_resources,
                      std::shared_ptr<RNG> randomgenerator)
        : Base(init_genotype, cell, init_resources, randomgenerator)
    {
        this->genotype_phenotype_map();
        this->adaption = std::vector<double>(this->end - this->start, 0.);
    }

    /**
     * @brief Construct a new Agent State Complex object
     *
     * @param parent_state
     * @param init_resources
     * @param mutationrates
     */
    AgentStateComplex(AgentStateComplex& parent_state,
                      double init_resources,
                      std::vector<double>& mutationrates)
        : Base(parent_state, init_resources)
    {
        this->genotype = this->copy_genome(parent_state.genotype, mutationrates);

        this->genotype_phenotype_map();
        this->adaption = std::vector<double>(this->end - this->start, 0.);
    }
};

template <typename Cell, typename Gtype, typename Ptype, typename RNG>
void swap(AgentStateComplex<Cell, Gtype, Ptype, RNG>& lhs,
          AgentStateComplex<Cell, Gtype, Ptype, RNG>& rhs)
{
    lhs.swap(rhs);
}
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif