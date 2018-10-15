#ifndef UTOPIA_MODELS_AMEEMULTI_CUSTOM_MANAGERS_HH
#define UTOPIA_MODELS_AMEEMULTI_CUSTOM_MANAGERS_HH

#include <algorithm>
#include <dune/utopia/core/grid.hh>
#include <dune/utopia/models/AmeeMulti/utils/memorypool.hh>

namespace Utopia
{
/// GridManager specialization for agents
template <typename DataType, typename GridType, bool structured, bool periodic>
class MempoolGridManager<Manager::Agents, DataType, GridType, structured, periodic>
    : public GridManagerBase<GridType, structured, periodic>
{
public:
    /// Data types related to the grid
    using Traits = GridTypeAdaptor<GridType>;
    /// Base class type
    using Base = GridManagerBase<GridType, structured, periodic>;
    /// Data type of cells (shared pointer to it)
    using Agent = DataType;
    /// Data type of the container
    using Container = std::vector<Agent*>;

private:
    /// container for agents
    std::vector<Agent*> _agents;

    /// memorypool to hold the memory for the agents
    MemoryPool<Agent> _mempool;

public:
    /// Create a GridManager from a grid and cells
    explicit GridManager(const GridWrapper<GridType>& wrapper, std::size_t memorysize)
        : _mempool(MemoryPool<Agent>(memorysize))
    {
        _agents.reserve(memorysize);
        _agents.push_back(::new (mempool.allocate()) Agent());
    }

    /// Return const reference to the managed agents
    const AgentContainer<Agent>& agents() const
    {
        return _agents;
    }
    /// Return reference to the managed agents
    AgentContainer<Agent>& agents()
    {
        return _agents;
    }

    MemoryPool<Agent>& memorypool()
    {
        return _mempool;
    }

    /**
     * @brief Erase all agents for which rule evaluates to true
     *
     * @param rule Unary function of signature bool(Agent*)
     */
    template <typename Rule>
    void erase_if(Rule rule)
    {
        auto cutoff = std::remove_if(_agents.begin(), _agents.end(), rule);
        _agents.erase(cutoff, _agents.end());
    }

    /**
     * @brief Apply a unary function which does not change the containersize to
     *        each agent in the population
     *
     * @tparam Rule
     * @param rule
     */
    template <typename Rule>
    void apply_rule(Rule&& rule)
    {
        std::for_each(_agents.begin(), _agents.end(), std::forward<Rule&&>(rule));
    }

    /**
     * @brief Apply a unary function which may change the containersize to
     *        each agent in the subpopulation in the index interval [s,e)
     *
     * @tparam Rule Unaryfunctiontype of signature void(Agent*)
     * @param s start of subinterval to apply rule to
     * @param e end of subinterval to apply rule to
     * @param rule Rule to apply
     */
    template <typename Rule>
    void apply_rule_n(std::size_t s, std::size_t e, Rule&& rule)
    {
        for (std::size_t i = s; i < e; ++i)
        {
            rule(_agents[i]);
        }
    }

    /**
     * @brief Apply a unary function which may change the containersize to
     *        each agent in the subpopulation in the index interval [0,n)
     *
     * @tparam Rule Unaryfunctiontype of signature void(Agent*)
     * @param  n end of subinterval to apply rule to
     * @param  rule Rule to apply
     */
    template <typename Rule>
    void apply_rule_n(std::size_t n, Rule&& rule)
    {
        apply_rule_n(0, n, std::forward<Rule&&>(rule));
    }

    /**
     * @brief Apply a unary function which may change the containersize to
     *        each agent in the subpopulation in the index interval [s,e)
     *        after shuffling said interval via the supplied rng
     *
     * @tparam Rule Unaryfunctiontype of signature void(Agent*)
     * @param s start of subinterval to apply rule to
     * @param e end of subinterval to apply rule to
     * @param rng Randomnumber generator for shuffle
     * @param rule Rule to apply
     */
    template <typename Rule, typename Rng>
    void apply_rule_n(std::size_t s, std::size_t e, Rng& rng, Rule&& rule)
    {
        std::shuffle(std::next(agents.begin(), s), std::next(agents.begin(), e), rng);

        for (std::size_t i = s; i < e; ++i)
        {
            rule(_agents[i]);
        }
    }

    /**
     * @brief Apply a unary function which may change the containersize to
     *        each agent in the subpopulation in the index interval [0,n)
     *
     * @tparam Rule Unaryfunctiontype of signature void(Agent*)
     * @param  n end of subinterval to apply rule to
     * @param rng Randomnumber generator for shuffle
     * @param  rule Rule to apply
     */
    template <typename Rule, typename Rng>
    void apply_rule_n(std::size_t n, Rng& rng, Rule&& rule)
    {
        apply_rule_n(0, n, rng, std::forward<Rule&&>(rule));
    }
};
} // namespace Utopia
#endif