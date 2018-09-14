#ifndef UTOPIA_MODELS_AMEEMULTI_AGENT_STATE_HH
#define UTOPIA_MODELS_AMEEMULTI_AGENT_STATE_HH
#include "agentstate_base.hh"
namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
/**
 * @brief Simple agentstate which makes the phenotype from the genotype
 *        in a direct copy of (4+3*sumlen, len(genotype)).
 *
 * @tparam Cell
 * @tparam Gtype
 * @tparam Ptype
 * @tparam RNG
 */
template <typename Cell, typename Gtype, typename Ptype, typename RNG>
struct AgentStateSimple : public AgentStateBase<Cell, Gtype, Ptype, RNG>
{
    using Base = AgentStateBase<Cell, Gtype, Ptype, RNG>;
    using Phenotype = typename Base::Phenotype;
    using Genotype = typename Base::Genotype;
    using G = typename Genotype::value_type;
    using P = typename Phenotype::value_type;

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
            if (sl < 0 or sl >= this->genotype.size())
            {
                this->sumlen = 0;
            }
            else
            {
                this->sumlen = sl;
            }

            if (this->genotype.size() < (4 + (4 * this->sumlen)))
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

                this->phenotype =
                    Phenotype(this->genotype.begin() + (4 + 3 * this->sumlen),
                              this->genotype.end());
            }
        }
    }

    void swap(AgentStateSimple& other)
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

    AgentStateSimple() = default;
    AgentStateSimple(const AgentStateSimple& other) = default;
    AgentStateSimple(AgentStateSimple&& other) = default;
    AgentStateSimple& operator=(AgentStateSimple&& other) = default;
    AgentStateSimple& operator=(const AgentStateSimple& other) = default;
    virtual ~AgentStateSimple() = default;

    /**
     * @brief Construct a new AgentStateBase_Complex object
     *
     * @param init_genome
     * @param cell
     * @param init_resources
     * @param randomgenerator
     */
    AgentStateSimple(typename Base::Genotype init_genome,
                     std::shared_ptr<Cell> cell,
                     double init_resources,
                     std::shared_ptr<RNG> randomgenerator)
        : Base(init_genome, cell, init_resources, randomgenerator)
    {
        this->genotype_phenotype_map();
        this->adaption = std::vector<double>(this->end - this->start, 0.);
    }

    /**
     * @brief Construct a new AgentState object
     *
     * @param parent_state
     * @param init_resources
     * @param substmut
     * @param editmut
     */
    AgentStateSimple(AgentStateSimple& parent_state,
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
void swap(AgentStateSimple<Cell, Gtype, Ptype, RNG>& lhs,
          AgentStateSimple<Cell, Gtype, Ptype, RNG>& rhs)
{
    lhs.swap(rhs);
}

} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif