#ifndef AGENT_HH
#define AGENT_HH

namespace Utopia {

// --- helper functions for decreasing runtime overhead --- //

/// Return the index of the grid entity for a certain position
/** \param pos Query position
 *  \param ext Extensions of the grid
 *  \param grid_cells Number of grid cells in each direction
 */
template<std::size_t i, typename Position, typename Extensions, typename GridCells>
std::enable_if_t<i==0,std::size_t> grid_index (const Position& pos, const Extensions& ext, const GridCells& grid_cells)
{
    std::size_t id_inc = pos[i] / ( ext[i] / grid_cells[i] );
    return id_inc;
}

/// Return the index of the grid entity for a certain position
/** \param pos Query position
 *  \param ext Extensions of the grid
 *  \param grid_cells Number of grid cells in each direction
 */
template<std::size_t i, typename Position, typename Extensions, typename GridCells>
std::enable_if_t<i!=0,std::size_t> grid_index (const Position& pos, const Extensions& ext, const GridCells& grid_cells)
{
    std::size_t id_inc = pos[i] / ( ext[i] / grid_cells[i] );
    return id_inc * shift<i>(grid_cells) + grid_index<i-1>(pos,ext,grid_cells);
}

/// Return the limits of a grid cell in one direction for a certain index
/** \param index Query index
 *  \param extensions Extensions of the grid
 *  \param grid_cells Number of grid cells in each direction
 */
template<std::size_t i, typename Extensions, typename GridCells>
std::enable_if_t<i==0,std::pair<double,double>> cell_limits_per_index (const std::size_t index, const Extensions& extensions, const GridCells& grid_cells)
{
    std::size_t offset = index % shift<1>(grid_cells);
    const double ext_per_cell = extensions[i] / grid_cells[i];
    return std::make_pair( offset * ext_per_cell , (offset + 1) * ext_per_cell );
}

/// Return the limits of a grid cell in one direction for a certain index
/** \param index Query index
 *  \param extensions Extensions of the grid
 *  \param grid_cells Number of grid cells in each direction
 */
template<std::size_t i, typename Extensions, typename GridCells>
std::enable_if_t<i!=0,std::pair<double,double>> cell_limits_per_index (const std::size_t index, const Extensions& extensions, const GridCells& grid_cells)
{
    std::size_t offset = index / shift<i>(grid_cells);
    const double ext_per_cell = extensions[i] / grid_cells[i];
    return std::make_pair( offset * ext_per_cell , (offset + 1) * ext_per_cell );
}

// ---  --- //

/// Clone an agent, copying state, traits, and position
/** \param agent Pointer to the agent that should be cloned
 *  \return Shared pointer to the new agent
 */
template<class Agent>
std::shared_ptr<Agent> clone (const std::shared_ptr<Agent> agent)
{
    return std::make_shared<Agent>(*agent);
}

/// Remove an agent from a managed container
/** \param agent Agent to be removed
 *  \param manager Manager of agents
 *  \throw Exception Agent is not managed by this manager
 */
template<class Agent, class Manager>
void remove (const std::shared_ptr<Agent> agent, Manager& manager)
{
    auto& agents = manager.agents();
    const auto it = std::find(agents.cbegin(),agents.cend(),agent);
    if(it==agents.cend()){
        DUNE_THROW(Dune::Exception,"Agent is not managed by this manager");
    }
    agents.erase(it);
}

/// Add an agent to a managed container, if it is not yet managed
/** The agent will be inserted at the end of the container
 *  \param agent Agent to be added
 *  \param manager Manager of agents
 *  if template parameter debug=true:
 *  agents are compared and only new agents are added
 *  else: agents are always added 
 *  \return true if the agent was inserted
 */
template<bool debug=false,class Agent, class Manager>
bool add (const std::shared_ptr<Agent> agent, Manager& manager)
{
    if(debug==true)
    {
         auto& agents = manager.agents();
         if(std::find(agents.cbegin(),agents.cend(),agent)==agents.cend())
         {
             agents.push_back(agent);
             return true;
         }
         return false;
    }
    else
    {
        auto& agents = manager.agents();
        agents.push_back(agent);
        return true;
    }
   
}

/// Append an Agentcontainer to a managed container
/** The new container will be inserted at the end of the container.
 *  \tparam debug Check if agents already exist in manager
 *  \param additional_agents AgentContainer to be added
 *  \param manager Manager of agents
 *  if template parameter debug=true:
 *  agents are compared and only new agents are added
 *  else: agents are always added
 *  \return true if the agent was inserted.
 *      Debug: return vector of booleans indicating succesful insertion.
 */
template<bool debug=false, class Agent, class Manager>
//bool add (const std::shared_ptr<Agent> agent, Manager& manager)
auto add (const AgentContainer<Agent>& additional_agents, Manager& manager)
{
    //check if agent is already in the container
    if constexpr(debug)
    {
        auto& agents = manager.agents();
        agents.reserve(agents.size() + additional_agents.size());

        // copy new agents and store which ones have not been copied
        std::vector<bool> was_inserted;
        was_inserted.reserve(additional_agents.size());
        auto contains_not = [&agents, &was_inserted](const auto agent) {
            const auto insert = std::find(agents.cbegin(), agents.cend(), agent)
                == agents.cend();
            was_inserted.push_back(insert);
            return insert;
        };

        std::copy_if(additional_agents.begin(), additional_agents.end(),
            std::back_inserter(agents), contains_not);
             
        return was_inserted;                           
    }
    // just copy all
    else
    {
        auto& agents = manager.agents();
        agents.reserve(agents.size() + additional_agents.size());
        std::copy(additional_agents.begin(), additional_agents.end(),
            std::back_inserter(agents));

        return true;
    }
}

/// Return all agents on a cell on a structured grid
/** Check match by calculating cell boundaries
 *  \param cell Cell on which agents should be found
 *  \param manager Manager of agents and associated grid
 */
template<class Cell, class Manager,
    bool enabled = Manager::is_structured()>
auto find_agents_on_cell (const std::shared_ptr<Cell> cell, const Manager& manager)
    -> std::enable_if_t<enabled,
        std::vector<std::shared_ptr<typename Manager::Agent>> >
{
    std::vector<std::shared_ptr<typename Manager::Agent>> ret;
    const auto id = cell->id();
    const auto& extensions = manager.extensions();
    const auto& grid_cells = manager.grid_cells();

    // deduce cell boundaries
    constexpr int dim = Manager::Traits::dim;
    std::array<std::pair<double,double>,dim> limits;

    if(dim == 3){
        limits[2] = cell_limits_per_index<2>(id,extensions,grid_cells);
        // 'normalize'
        const auto id_nrm = id % shift<2>(grid_cells);
        limits[1] = cell_limits_per_index<1>(id_nrm,extensions,grid_cells);
    }
    else{
        limits[1] = cell_limits_per_index<1>(id,extensions,grid_cells);
    }
    limits[0] = cell_limits_per_index<0>(id,extensions,grid_cells);

    // find agents inside cell boundaries
    for(auto agent : manager.agents()){
        const auto& pos = agent->position();
        if(std::equal(pos.begin(),pos.end(),limits.begin(),
            [](const auto val, const auto& lim){
                return lim.first <= val && val < lim.second;
            }))
        {
            ret.push_back(agent);
        }
    }

    return ret;
}

/// Return all agents on a cell on an unstructured grid
/** Transform coordinates for every agent and check match
 *  \param cell Cell on which agents should be found
 *  \param manager Manager of agents and associated grid
 */
template<class Cell, class Manager,
    bool enabled = Manager::is_structured()>
auto find_agents_on_cell (const std::shared_ptr<Cell> cell, const Manager& manager)
    -> std::enable_if_t<!enabled,
        std::vector<std::shared_ptr<typename Manager::Agent>> >
{
    using Coordinate = typename Manager::Traits::Coordinate;
    constexpr int dim = Manager::Traits::dim;
    std::vector<std::shared_ptr<typename Manager::Agent>> ret;

    // get reference element for cell
    auto it = elements(manager.grid_view()).begin();
    std::advance(it,cell->id());
    const auto& geo = it->geometry();
    const auto& ref = Dune::ReferenceElements<Coordinate,dim>::general(geo.type());

    // check location by transforming coordinates
    for(auto agent : manager.agents()){
        const auto pos_local = geo.local(agent->position());
        if(ref.checkInside(pos_local)){
            ret.push_back(agent);
        }
    }

    return ret;
}

/// Find parent cell in unstructured grid
/** Transform coordinates for every grid entity and check match
 *  \param agent Agent for which the CA cell should be found
 *  \param manager Manager of cells and associated grid
 *  \throw Exception if agent is outside the grid
 */
template<class Agent, class Manager, bool enabled = Manager::is_structured()>
auto find_cell (const std::shared_ptr<Agent> agent, const Manager& manager)
    -> std::enable_if_t<!enabled,std::shared_ptr<typename Manager::Cell>>
{
    using Coordinate = typename Manager::Traits::Coordinate;
    constexpr int dim = Manager::Traits::dim;
    const auto& gv = manager.grid_view();

    const auto& pos_global = agent->position();
    const auto it = std::find_if(
        elements(gv).begin(),
        elements(gv).end(),
        [&pos_global](const auto& entity)
        {
            // perform coordinate transformation
            const auto& geo = entity.geometry();
            const auto pos_local = geo.local(pos_global);
            const auto& ref = Dune::ReferenceElements<Coordinate,dim>::general(geo.type());
            return ref.checkInside(pos_local);
        }
    );

    if(it == elements(gv).end()){
        DUNE_THROW(Dune::Exception,"Agent is not inside the grid!");
    }

    const auto id = manager.mapper().index(*it);
    return manager.cells()[id];
}

/// Find parent cell in structured grid
/** Calculate index of cell using grid information
 *  \param agent Agent for which the CA cell should be found
 *  \param manager Manager of cells and associated grid
 *  \throw Exception if agent is outside the grid
 */
template<class Agent, class Manager, bool enabled = Manager::is_structured()>
auto find_cell (const std::shared_ptr<Agent> agent, const Manager& manager)
    -> std::enable_if_t<enabled,std::shared_ptr<typename Manager::Cell>>
{
    const auto& position = agent->position();
    const auto& extensions = manager.extensions();
    const auto& grid_cells = manager.grid_cells();

    std::size_t index;
    if(Manager::Traits::dim == 3)
    {
        index = grid_index<2>(position,extensions,grid_cells);
    }
    else{
        index = grid_index<1>(position,extensions,grid_cells);
    }

    if(index >= manager.mapper().size()){
        DUNE_THROW(Dune::Exception,"Agent is not inside the grid!");
    }

    return manager.cells()[index];
}

/// Move agent to a new location on a periodic grid
/** This function automatically handles coordinate transformation
 *  if the position is outside the actual grid.
 *  \param pos New position (may be outside the grid)
 *  \param agent Agent to move
 *  \param manager Manager of Agent and associated grid
 */
template<typename Position, class Agent, class Manager,
    bool enabled = Manager::is_periodic()>
std::enable_if_t<enabled,void> move_to (const Position& pos, const std::shared_ptr<Agent> agent, const Manager& manager)
{
    const auto& ext = manager.extensions();
    auto pos_transf = pos;
    std::transform(pos.begin(),pos.end(),ext.begin(),pos_transf.begin(),
        [](const auto& a, const auto& b){
            const int shift = a / b > 0.0 ? - int(a / b) : - ( int(a / b) - 1 );
            return a + shift * b;
        });
    agent->position() = pos_transf;
}

/// Move agent to a new location on a non-periodic grid
/** \param pos New position
 *  \param agent Agent to move
 *  \param manager Manager of Agent and associated grid
 *  \throw Exception if position is outside the grid
 */
template<typename Position, class Agent, class Manager,
    bool enabled = Manager::is_periodic()>
std::enable_if_t<!enabled,void> move_to (const Position& pos, const std::shared_ptr<Agent> agent, const Manager& manager)
{
    const auto& ext = manager.extensions();
    std::vector<bool> diff(Manager::Traits::dim);
    std::transform(pos.begin(),pos.end(),ext.begin(),diff.begin(),
        [](const auto& a, const auto& b){
            return a > b;
        });
    if(std::any_of(diff.begin(),diff.end(),[](const auto a){ return a; })){
        DUNE_THROW(Dune::Exception,"Position is out of grid boundaries");
    }
    agent->position() = pos;
}


/// This class implements a moving Agent on a grid
/** An object of this class only saves its global position on the grid
 *  \tparam StateType Data type of state
 *  \tparam Tags Tags class from which Entity and thus Agent derive
 *  \tparam IndexType Data type of index
 *  \tparam PositionType Data type of position vector
 */
template<typename StateType, class Tags, typename IndexType, typename PositionType, std::size_t custom_neighborhood_count = 0>
class Agent :
    public Entity< Agent<StateType,Tags,IndexType,PositionType, custom_neighborhood_count> ,StateType, false, Tags, IndexType, custom_neighborhood_count>
{

public:
    
    using State = StateType;
    using Position = PositionType;
    using Index = IndexType;

private:
    //!< Global position on the grid
    Position _position;

public:
    /// Constructor
    /** \param state Initial state
     *  \param traits Initial traits
     *  \param position Initial position
     *  \param tag Tracking tag
     */
    Agent (const State state, const Index index, const Position position) :
        Entity<Agent, State, false, Tags, Index,custom_neighborhood_count> (state, index),
        _position(position)
    { }

    /// Return const reference to the position of this agent
    const Position& position () const { return _position; }

    /// Return reference to the position of this agent
    Position& position () { return _position; }

};

} // namespace Utopia

#endif // AGENT_HH
