#ifndef AMEEMULTI_CUSTOM_MANAGERS_HH
#define AMEEMULTI_CUSTOM_MANAGERS_HH

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

    ///
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

    MemoryPool<Agent> memorypool()
    {
        return _mempool;
    }

    /// Erase all agents for which the rule evaluates to true
    template <typename Rule>
    void erase_if(Rule rule)
    {
        auto cutoff = std::remove_if(_agents.begin(), _agents.end(), rule);
        _agents.erase(cutoff, _agents.end());
    }
};
} // namespace Utopia
#endif