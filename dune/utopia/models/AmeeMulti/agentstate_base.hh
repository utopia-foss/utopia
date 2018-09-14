#ifndef UTOPIA_MODELS_AMEEMULTI_AGENT_STATE_BASE_HH
#define UTOPIA_MODELS_AMEEMULTI_AGENT_STATE_BASE_HH
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
template <typename Cell, typename Gtype, typename Ptype, typename RNG>
struct AgentStateBase
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

    /**
     * @brief Function for computing a value from a range [s,e) in the genome
     * - analog to biological codon-aminoacid map
     *
     * @param s start for computation
     * @param e end for computation
     * @return P value of type Phenotype::value_type
     */
    virtual P get_codon_value(int s, int e)
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

    /**
     * @brief function for copying the genotype upon reproduction, includes mutations
     *
     * @param parent_genome The genome of the parent
     * @param mutationrates the mutationrates used
     * @return Genotype New genotype, possibly mutated
     */
    Genotype copy_genome(Genotype& parent_genome, std::vector<double>& mutationrates)
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
     * @brief Function for genotype phenotype map
     *
     * @return Phenotype
     */
    void genotype_phenotype_map();

    AgentStateBase() = default;
    AgentStateBase(const AgentStateBase& other) = default;
    AgentStateBase(AgentStateBase&& other) = default;
    AgentStateBase& operator=(AgentStateBase&& other) = default;
    AgentStateBase& operator=(const AgentStateBase& other) = default;
    virtual ~AgentStateBase() = default;

    /**
     * @brief Construct a new Agent State Base object - reproduction constructor
     *
     * @param parent_state State object of the parent
     * @param init_resources Resource value inherited from parent
     * @param mutationrates mutationrates to use for mutating the genome, per default [substmut, editmut]
     */
    AgentStateBase(AgentStateBase& parent_state, double init_resources)
        : rng(parent_state.rng),
          resources(init_resources),
          fitness(0),
          sumlen(0),
          divisor(0.),
          start(0),
          end(0),
          adaption(std::vector<double>()),
          intensity(0),
          age(0),
          habitat(parent_state.habitat),
          deathflag(false),
          genotype(Genotype()),
          phenotype(Phenotype())
    {
    }

    /**
     * @brief Construct a new Agent State Base object, initial constructor for creating an agent state out of the blue
     *
     * @param init_genome  initial genome
     * @param cell  initial cell
     * @param init_resources initial resources
     * @param randomgenerator  shared pointer on some random generator
     */
    AgentStateBase(Genotype init_genome,
                   std::shared_ptr<Cell> cell,
                   double init_resources,
                   std::shared_ptr<RNG> randomgenerator)
        : rng(randomgenerator),
          resources(init_resources),
          fitness(0),
          sumlen(0),
          divisor(0.),
          start(0),
          end(0),
          adaption(std::vector<double>()),
          intensity(0),
          age(0),
          habitat(cell),
          deathflag(false),
          genotype(init_genome),
          phenotype(Phenotype())

    {
    }

}; // namespace Amee

/**
 * @brief Exchanges the state of lhs and rhs
 *
 * @param lhs object to swap state with rhs
 * @param rhs object to swap state with lhs
 */
template <typename Cell, typename Gtype, typename Ptype, typename RNG>
void swap(AgentStateBase<Cell, Gtype, Ptype, RNG>& lhs,
          AgentStateBase<Cell, Gtype, Ptype, RNG>& rhs)
{
    lhs.swap(rhs);
}

} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif