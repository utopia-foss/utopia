#ifndef UTOPIA_MODELS_AMEEMULTI_AGENT_STATE_HL_HH
#define UTOPIA_MODELS_AMEEMULTI_AGENT_STATE_HL_HH
#include "agentstate_base.hh"
#include <random>

namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
/// Agent State struct for Amee
template <typename Cell, typename Gtype, typename Ptype, typename RNG>
struct AgentStateHL : public AgentStateBase<Cell, Gtype, Ptype, RNG>
{
    using Base = AgentStateBase<Cell, Gtype, Ptype, RNG>;
    using Genotype = typename Base::Genotype;
    using Phenotype = typename Base::Phenotype;
    using G = typename Genotype::value_type;
    using P = typename Phenotype::value_type;

    void swap(AgentStateHL& other)
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

    /**
     * @brief Function for creating a new genotype of offspring from a parent
     *
     * @param parent_genotype genotype of parent
     * @param substmut  probability for replacing a trait value with another
     * value, mutated with gaussian around old value
     * @param editmut probablity to insert or delete a value, chosen from
     * [min, max], where min, max is minimum and maximum value of old_genotype.
     * @param std standard deviation for replace mutation
     * @return genotype new genotype, potentially mutated
     */
    Genotype copy_genome(Genotype& parent_genotype, std::vector<double>& mutationrates)
    {
        if constexpr (!std::is_floating_point_v<G>)
        {
            throw std::runtime_error(
                "genotype needs to hold floating point values");
        }

        if (parent_genotype.size() == 0)
        {
            return Genotype();
        }

        Genotype new_genotype = parent_genotype;
        auto min = *std::min_element(parent_genotype.begin(), parent_genotype.end());
        auto max = *std::max_element(parent_genotype.begin(), parent_genotype.end());

        double substmut = mutationrates[0];
        double editmut = mutationrates[1];
        double std = mutationrates[2];
        std::uniform_real_distribution<double> choice(0., 1.);
        std::uniform_real_distribution<P> values(min, max);
        std::uniform_int_distribution<std::size_t> loc(0, new_genotype.size() - 1);
        // mutate new genotype
        if (choice(*(this->rng)) < substmut)
        {
            auto where = loc(*this->rng);
            new_genotype[where] =
                std::normal_distribution<>(parent_genotype[where], std)(*(this->rng));
        }
        if (choice(*(this->rng)) < editmut)
        {
            new_genotype.insert(std::next(new_genotype.begin(), loc(*this->rng)),
                                values(*(this->rng)));
        }
        if (choice(*(this->rng)) < editmut)
        {
            new_genotype.erase(std::next(new_genotype.begin(), loc(*this->rng)));
        }
        return new_genotype;
    }

    void genotype_phenotype_map()
    {
        this->sumlen = 0;
        this->divisor = 0;
        if (this->genotype.size() < 4)
        {
            this->start = 0;
            this->end = 0;
            this->intensity = 0.;
            this->phenotype = Phenotype();
        }
        else
        {
            this->start = this->genotype[0];
            this->end = this->genotype[1];
            this->intensity = this->genotype[2];

            this->phenotype = Phenotype(this->genotype.begin(), this->genotype.end());
        }
    }

    AgentStateHL() = default;
    AgentStateHL(const AgentStateHL& other) = default;
    AgentStateHL(AgentStateHL&& other) = default;
    AgentStateHL& operator=(AgentStateHL&& other) = default;
    AgentStateHL& operator=(const AgentStateHL& other) = default;
    virtual ~AgentStateHL() = default;
    /**
     * @brief Construct a new AgentStateBase_Complex object
     *
     * @param init_genotype
     * @param cell
     * @param init_resources
     * @param randomgenerator
     */
    AgentStateHL(typename Base::Genotype init_genotype,
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
    AgentStateHL(AgentStateHL& parent_state, double init_resources, std::vector<double>& mutationrates)
        : Base(parent_state, init_resources)
    {
        this->genotype = this->copy_genome(parent_state.genotype, mutationrates);

        this->genotype_phenotype_map();
        this->adaption = std::vector<double>(this->end - this->start, 0.);
    }
};

template <typename Cell, typename Gtype, typename Ptype, typename RNG>
void swap(AgentStateHL<Cell, Gtype, Ptype, RNG>& lhs,
          AgentStateHL<Cell, Gtype, Ptype, RNG>& rhs)
{
    lhs.swap(rhs);
}

} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia

#endif