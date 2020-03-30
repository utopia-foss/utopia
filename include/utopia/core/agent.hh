#ifndef UTOPIA_CORE_AGENT_HH
#define UTOPIA_CORE_AGENT_HH

#include "state.hh"
#include "tags.hh"
#include "space.hh"
#include "types.hh"
#include "entity.hh"


namespace Utopia {
/**
 *  \addtogroup AgentManager
 *  \{
 */

/// AgentTraits are just another name for Utopia::EntityTraits
template<typename StateType,
         Update update_mode,
         bool use_def_state_constr=false,
         typename AgentTags=EmptyTag,
         template<class> class CustomLinkContainers=NoCustomLinks>
using AgentTraits = EntityTraits<StateType,
                                 update_mode,
                                 use_def_state_constr,
                                 AgentTags,
                                 CustomLinkContainers>;


/// An agent is a slightly specialized state container
/** It can be extended with the use of tags and can be associated with
  * so-called "custom links". These specializations are carried into the agent
  * by means of the AgentTraits struct.
  *
  * An agent is embedded into the Utopia::AgentManager, which has knowledge of
  * the Utopia::Space an agent is embedded in. This allows assigning a
  * position in space to the agent. The agent holds this position as a private
  * member and requires the AgentManager to set it.
  *
  * \tparam Traits  Valid Utopia::EntityTraits, describing the type of agent
  * \tparam Space   The Utopia::Space in which the agent lives; this is only
  *                 used to extract the information known at compile time, i.e.
  *                 the dimensionality of the space the agent lives in.
  * \tparam enabled Template parameter to enable sync or async specialization
  */
template<typename Traits, typename Space, typename enabled=void>
class Agent;


/// Agent specialization for asynchronous update
template<typename Traits, typename Space>
class Agent<Traits,
            Space,
            std::enable_if_t<Traits::mode != Update::sync>>
:
    public Entity<Agent<Traits, Space>, Traits>
{
public:
    /// The type of this agent
    using Self = Agent<Traits, Space>;

    /// The type of the state
    using State = typename Traits::State;

    /// The type of the position vector
    using Position = typename Space::SpaceVec;

    /// Make the agent manager a (template) friend of this class
    template<class T, class M>
    friend class AgentManager;

private:
    /// The position of the agent
    Position _pos;

public:
    /// Construct an agent
    /** \param id            The id of this agent, ideally unique
     *  \param initial_state The initial state
     *  \param initial_pos   The initial position
     */
    Agent (const IndexType id,
           const State initial_state,
           const Position& initial_pos)
    :
        Entity<Self, Traits>(id, initial_state),
        _pos(initial_pos)
    {}

    /// Return a copy of the current position of the agent
    Position position() const {
        return _pos;
    };


protected:
    /// Set the position of the agent
    /** \details This function allows befriended classes to set the position of
     *          this agent.
     *
     *  \note   No check is carried out whether the new position is valid; this
     *          needs to happen in the construct that is aware of the space the
     *          agent resides in, i.e.: Utopia::AgentManager.
     *
     *  \param  pos The new position of the agent
     */
    void set_pos(const Position& pos) {
        _pos = pos;
    };
};



/// Agent Specialisation for synchronous update
template<typename Traits, typename Space>
class Agent<Traits,
            Space,
            std::enable_if_t<Traits::mode == Update::sync>>
:
    public Entity<Agent<Traits, Space>, Traits>
{
public:
    /// The type of this agent
    using Self = Agent<Traits, Space>;

    /// The type of the state
    using State = typename Traits::State;

    /// The type of the position vector
    using Position = typename Space::SpaceVec;

    /// Make the agent manager a (template) friend
    template<class T, class M>
    friend class AgentManager;

private:
    /// The current position of the agent
    Position _pos;

    /// The position buffer for the synchronous position update
    Position _pos_new;

public:
    /// Construct an agent
    /** \param id            The id of this agent, ideally unique
     *  \param initial_state The initial state
     *  \param initial_pos   The initial position
     */
    Agent(const IndexType id,
          const State& initial_state,
          const Position& initial_pos)
    :
        Entity<Self, Traits>(id, initial_state),
        _pos(initial_pos),
        _pos_new(initial_pos)
    {}

    /// Return the current position of the agent
    Position position() const {
        return _pos;
    }

    /// Return the position buffer of the agent
    Position position_new() const {
        return _pos_new;
    }

    /// Update the position and the state
    /** \details Writes the buffer of state and position to the current state
     *          and position. This is necessary for the synchronous update mode
     *          only.
     */
    void update () {
        // Update the state as defined in the Entity class
        Entity<Self, Traits>::update();

        // Update the position
        _pos = _pos_new;
    }

protected:
    /// Set the position buffer of the (synchronously updated) agent
    /** \details This function allows befriended classes to set the position of
     *          this agent.
     *
     *  \note   No check is carried out whether the new position is valid; this
     *          needs to happen in the construct that is aware of the space the
     *          agent resides in, i.e.: Utopia::AgentManager.
     *
     *  \param  pos The new position of the agent
     */
    void set_pos(const Position& pos) {
        _pos_new = pos;
    }
};

// end group AgentManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_AGENT_HH
