#ifndef UTOPIA_MODELS_AMEE_MULTI_AGENTSTATE_HH
#define UTOPIA_MODELS_AMEE_MULTI_AGENTSTATE_HH
#include <algorithm>
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
 * @brief Abstract basic interface for Agentstates. Includes a basic genome
 * copying function a function for making an analog of the codon-amino-acid map
 * and an abstract function for genotype-phenotype-map
 *
 * @tparam Cell  The cell type which represents the habitat the agents live on
 * @tparam Gtype The genome type, e.g. std::vector<int>
 * @tparam Ptype the phenotype type e.g. std::vector<double>
 * @tparam RNG the random number generator type.
 */
template <typename Cell, typename Statepolicy>
struct AgentState : public Statepolicy
{
    using Genotype = typename Statepolicy::Genotype;
    using Phenotype = typename Statepolicy::Phenotype;
    using RNG = typename Statepolicy::RNG;
    using G = typename Genotype::value_type;
    using P = typename Phenotype::value_type;

    std::shared_ptr<RNG> rng;
    double resources;
    double fitness;
    unsigned sumlen;
    double divisor;
    int start;
    int end;
    int start_mod;
    int end_mod;
    std::vector<double> adaption;
    double intensity;
    std::size_t age;
    std::shared_ptr<Cell> habitat;
    bool deathflag;
    Genotype genotype;
    Phenotype phenotype;

    void swap(AgentState<Cell, Statepolicy>& other)
    {
        if (this == &other)
        {
            return;
        }
        else
        {
            using std::swap;
            swap(rng, other.rng);
            swap(resources, other.resources);
            swap(fitness, other.fitness);
            swap(sumlen, other.sumlen);
            swap(divisor, other.divisor);
            swap(start, other.start);
            swap(end, other.end);
            swap(start_mod, other.start_mod);
            swap(end_mod, other.end_mod);
            swap(adaption, other.adaption);
            swap(intensity, other.intensity);
            swap(age, other.age);
            swap(habitat, other.habitat);
            swap(deathflag, other.deathflag);
            swap(genotype, other.genotype);
            swap(phenotype, other.phenotype);
        }
    }

    AgentState() = default;
    AgentState(const AgentState& other) = default;
    AgentState(AgentState&& other) = default;
    AgentState& operator=(AgentState&& other) = default;
    AgentState& operator=(const AgentState& other) = default;
    virtual ~AgentState() = default;

    /**
     * @brief Construct a new Agent State Base object - reproduction constructor
     *
     * @param parent_state State object of the parent
     * @param init_resources Resource value inherited from parent
     * @param mutationrates mutationrates to use for mutating the genome, per default [substmut, editmut]
     */
    AgentState(AgentState& parent_state, double init_resources, std::vector<double>& mutationrates)
        : rng(parent_state.rng),
          resources(init_resources),
          fitness(0),
          age(0),
          habitat(parent_state.habitat),
          deathflag(false)
    {
        genotype = this->copy_genome(parent_state.genotype, mutationrates, rng);

        std::tie(sumlen, divisor, start, end, start_mod, end_mod, intensity,
                 phenotype) = this->genotype_phenotype_map(genotype);

        if (end < 0)
        {
            end = 0;
        }
        if (start < 0)
        {
            start = 0;
        }
        if (end < start)
        {
            start = end;
        }

        if (end_mod < 0)
        {
            end_mod = 0;
        }
        if (start_mod < 0)
        {
            start_mod = 0;
        }
        if (end_mod < start_mod)
        {
            start_mod = end_mod;
        }

        adaption = std::vector<double>(end - start, 0.);
    }

    /**
     * @brief Construct a new Agent State Base object, initial constructor for creating an agent state out of the blue
     *
     * @param init_genome  initial genome
     * @param cell  initial cell
     * @param init_resources initial resources
     * @param randomgenerator  shared pointer on some random generator
     */
    AgentState(Genotype init_genome,
               std::shared_ptr<Cell> cell,
               double init_resources,
               std::shared_ptr<RNG> randomgenerator)
        : rng(randomgenerator),
          resources(init_resources),
          fitness(0),
          age(0),
          habitat(cell),
          deathflag(false),
          genotype(init_genome)
    {
        std::tie(sumlen, divisor, start, end, start_mod, end_mod, intensity,
                 phenotype) = this->genotype_phenotype_map(genotype);
        if (end < 0)
        {
            end = 0;
        }
        if (start < 0)
        {
            start = 0;
        }
        if (end < start)
        {
            start = end;
        }

        if (end_mod < 0)
        {
            end_mod = 0;
        }
        if (start_mod < 0)
        {
            start_mod = 0;
        }
        if (end_mod < start_mod)
        {
            start_mod = end_mod;
        }

        adaption = std::vector<double>(end - start, 0.);
    }

}; // namespace Amee

/**
 * @brief Exchanges the state of lhs and rhs
 *
 * @param lhs object to swap state with rhs
 * @param rhs object to swap state with lhs
 */
template <typename Cell, typename Policy>
void swap(AgentState<Cell, Policy>& lhs, AgentState<Cell, Policy>& rhs)
{
    lhs.swap(rhs);
}
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia

#endif