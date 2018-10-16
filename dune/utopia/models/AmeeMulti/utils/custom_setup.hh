#ifndef UTOPIA_MODELS_AMEEMULTI_CUSTOM_MANAGERS_HH
#define UTOPIA_MODELS_AMEEMULTI_CUSTOM_MANAGERS_HH

#include <algorithm>
#include <dune/utopia/core/grid.hh>
#include <dune/utopia/core/tags.hh>
#include <dune/utopia/models/AmeeMulti/utils/memorypool.hh>

namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
namespace Setup
{
/// Create a set of cells on a grid
/** The cells will only map to the grid, but not share data with it.
 *  \param grid_wrapper GridWrapper instance of the grid the cells
 *  	will be created on
 *  \param state Default state of all cells
 *  \param traits Default traits of all cells
 *  \return Container with created cells
 */
template <template <typename...> class Gridcell, bool sync, typename State = int, typename Tag = EmptyTag, std::size_t custom_neighborhood_count = 0, typename GridType>
decltype(auto) create_cells_on_grid(const GridWrapper<GridType>& grid_wrapper,
                                    const State state = State())
{
    using GridTypes = GridTypeAdaptor<GridType>;
    using Position = typename GridTypes::Position;
    using GV = typename GridTypes::GridView;
    using Mapper = typename GridTypes::Mapper;
    using Index = typename GridTypes::Index;

    using CellType = Gridcell<State, sync, Tag, Position, Index>;

    auto grid = grid_wrapper._grid;
    GV gv(*grid);
    Mapper mapper(gv, Dune::mcmgElementLayout());
    CellContainer<CellType> cells;
    cells.reserve(mapper.size());

    // loop over all entities and create cells
    for (const auto& e : elements(gv))
    {
        const Position pos = e.geometry().center();
        const Index id = mapper.index(e);

        // check if entity is at boundary
        bool boundary = false;
        for (const auto& is : intersections(gv, e))
        {
            if (!is.neighbor())
            {
                boundary = true;
                break;
            }
        }

        cells.emplace_back(std::make_shared<CellType>(state, pos, boundary, id));
    }

    cells.shrink_to_fit();
    return cells;
}

/// Create a grid from a model configuration
/**\detail Using information from the configuration extracted from a
 *         parent model instance, a new grid instance is returned
 *
 * \param name          The name of the model instance; needed for access
 *                      to the correct configuration parameter
 * \param parent_model  The parent model the new model instance will
 *                      reside in
 *
 * \tparam dim          Dimensionaliy of the grid, can be 2 or 3
 * \tparam ParentModel  The parent model type
 */
template <unsigned short dim = 2, typename ParentModel>
auto create_grid_from_model(const std::string name, const ParentModel& parent_model)
{
    // Get the logger
    const auto log = parent_model.get_logger();
    log->info("Setting up grid from model instance '{}'...", name);

    // Get the configuration
    const auto cfg = parent_model.get_cfg()[name];

    // Extract grid size from config
    static_assert(dim == 2 || dim == 3,
                  "Template argument dim must be 2 or 3!");
    const auto gsize = as_array<unsigned int, dim>(cfg["grid_size"]);

    // Inform about the size
    if constexpr (dim == 2)
    {
        log->info("Creating 2-dimensional grid of size: {} x {} ...", gsize[0], gsize[1]);
    }
    else
    {
        log->info("Creating 3-dimensional grid of size: {} x {} x {} ...",
                  gsize[0], gsize[1], gsize[2]);
    }

    // Create grid of that size and return
    return create_grid<dim>(gsize);
}

/// Grid setup function
/** \detail Sets up a GridManager with cells using the configuration info
 * supplied by a model and its model configuration.
 *
 * \param name          The name of the model instance; needed for access
 *                      to the correct configuration parameter
 * \param parent_model  The parent model the new model instance will
 *                      reside in.
 * \param initial_state The initial state of all cells
 *
 * \tparam periodic     Whether the grid should be periodic
 * \tparam dim          Dimensionaliy of the grid, can be 2 or 3
 * \tparam structured   Whether the grid should be structured
 * \tparam sync         Whether the cells should be synchronous or not
 * \tparam ParentModel  The parent model type
 * \tparam State        Type of the initial state
 */
template <typename State, bool periodic = true, unsigned short dim = 2, bool structured = true, bool sync = true, typename ParentModel>
auto create_grid_manager_cells(const std::string name,
                               const ParentModel& parent_model,
                               const State initial_state = State())
{
    // Get the grid, passing through arguments
    auto grid = create_grid_from_model<dim>(name, parent_model);

    // Create cells on that grid, passing the initial state
    auto cells = create_cells_on_grid<sync>(grid, initial_state);

    // Create the grid manager, passing the template argument
    parent_model.get_logger()->info(
        "Initializing GridManager with {} "
        "boundary conditions ...",
        (periodic ? "periodic" : "fixed"));
    // TODO add the other template arguments to the log message

    return create_manager_cells<structured, periodic>(grid, cells);
}

/// GridManager specialization for agents
template <typename DataType, typename GridType, bool structured, bool periodic>
class AgentGridManager : public GridManagerBase<GridType, structured, periodic>
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

    explicit AgentGridManager(const GridWrapper<GridType>& wrapper,
                              const AgentContainer<Agent>& agents,
                              std::size_t memorysize)
        : Base(wrapper), _mempool(MemoryPool<Agent>(memorysize))
    {
        using AT = typename AgentContainer<Agent>::value_type;
        _agents.reserve(memorysize);

        if (memorysize < agents.size())
        {
            throw std::invalid_argument(
                "GridManager constructor: Memorysize needs to be at least "
                "agentsize");
        }

        // inplace construct into memorypool
        for (auto&& agent : agents)
        {
            if constexpr (std::is_pointer_v<AT> ||
                          std::is_same_v<AT, std::shared_ptr<Agent>>)
            {
                _agents.push_back(::new (_mempool.allocate()) Agent(*agent));
            }
            else
            {
                _agents.push_back(::new (_mempool.allocate()) Agent(agent));
            }
        }
    }

    /// Return const reference to the managed agents
    const Container& agents() const
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

    template <typename... Args>
    Agent* add_agent(Agent* ptr_to_construct_in, Args&&... args)
    {
        return ::new (ptr_to_construct_in) Agent(std::forward<Args&&...>(args...));
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
        std::shuffle(std::next(_agents.begin(), s), std::next(_agents.begin(), e), rng);

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

/**
 * @brief Create a agentmanager object
 *
 * @tparam structured
 * @tparam periodic
 * @tparam GridType
 * @tparam Agent
 * @param wrapper
 * @param agents
 * @param memorypoolsize
 * @return GridManager<Manager::Agents, Agent, GridType, structured, periodic>
 */
template <bool structured, bool periodic, typename GridType, typename Agent>
auto create_manager_agents(const GridWrapper<GridType>& wrapper,
                           const AgentContainer<Agent>& agents,
                           std::size_t memorypoolsize)
    -> AgentGridManager<Agent, GridType, structured, periodic>
{
    return AgentGridManager<Agent, GridType, structured, periodic>(
        wrapper, agents, memorypoolsize);
}
} // namespace Setup
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif