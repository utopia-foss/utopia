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
template <typename Cell, typename Gtype, typename Ptype, typename RNG, typename Statepolicy>
struct AgentState : public Statepolicy
{
    using Genotype = Gtype;
    using Phenotype = Ptype;
    using G = typename Genotype::value_type;
    using P = typename Phenotype::value_type;

    std::shared_ptr<RNG> rng;
    double resources;
    double fitness;
    unsigned sumlen;
    double divisor;
    int start;
    int end;
    std::vector<double> adaption;
    double intensity;
    std::size_t age;
    std::shared_ptr<Cell> habitat;
    bool deathflag;
    Genotype genotype;
    Phenotype phenotype;

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

        std::tie(sumlen, divisor, start, end, intensity, phenotype) =
            this->genotype_phenotype_map(genotype);
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
        std::tie(sumlen, divisor, start, end, intensity, phenotype) =
            this->genotype_phenotype_map(genotype);
        adaption = std::vector<double>(end - start, 0.);
    }

}; // namespace Amee

/**
 * @brief Exchanges the state of lhs and rhs
 *
 * @param lhs object to swap state with rhs
 * @param rhs object to swap state with lhs
 */
template <typename Cell, typename Gtype, typename Ptype, typename RNG, typename Policy>
void swap(AgentState<Cell, Gtype, Ptype, RNG, Policy>& lhs,
          AgentState<Cell, Gtype, Ptype, RNG, Policy>& rhs)
{
    lhs.swap(rhs);
}
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia

#endif