#ifndef UTOPIA_CORE_AGENT_HH
#define UTOPIA_CORE_AGENT_HH

// NOTE This file's final name will be cell.hh

#include "state.hh"
#include "tags.hh"
#include "space.hh"
#include "types.hh"
#include "entity_new.hh"       // NOTE Replace with entity.hh in final version


namespace Utopia {
/**
 *  \addtogroup AgentManager
 *  \{
 */

/// An agent trait is equivalent to an entity trait
/**
 * \ref Utopia::EntityTraits
 */
template<typename StateType, 
         UpdateMode update_mode,
         bool use_def_state_constr=false,
         typename AgentTags=EmptyTag,
         template<class> class CustomLinkContainers=NoCustomLinks>
using AgentTraits = EntityTraits<StateType, 
                                update_mode,
                                use_def_state_constr,
                                AgentTags, 
                                CustomLinkContainers>;


/// An agent is a slightly specialized state container
/** \detail  It can be extended with the use of tags and can be associated with
  *          so-called "custom links". These specializations are carried into
  *          the agent by means of the AgentTraits struct.
  *          An agent is embedded into the AgentManager, where the discretization
  *          allows assigning a position in space to the agent. The agent holds 
  *          this position as a private member and the Manager to set it.
  * \tparam Traits as in AgentTraits
  * \tparam Space the Space in which the agent lives
  * \tparam enabled template parameter to enable sync or async specialisation
  */
template<typename Traits, typename Space, typename enabled=void>
class __Agent;


/// Agent specialisation for asynchronous update
template<typename Traits, typename Space>
class __Agent<Traits, Space, typename std::enable_if_t<Traits::sync == false>>
:
    public __Entity<Traits>
{   
public:  
    /// The type of the state
    using State = typename Traits::State;

    /// The type of the Position
    using Position = typename Space::SpaceVec;

    /// Make the agent manager a friend of this class
    template<class T, class M>
    friend class AgentManager;   

private:
    // -- Members -- //
    /// The position of the agent 
    Position _pos;


public:
    // -- Constructor -- //
    /// Construct an agent
    /** \detail id, initial state and initial position have to be passed
     * \param id The id of this agent
     * \param initial_state The initial state 
     * \param initial_pos The initial position
     */
    __Agent(const IndexType id, const State initial_state, 
            const Position& initial_pos)
    :
        __Entity<Traits>(id, initial_state),
        _pos(initial_pos)    
    {}


protected:
    /// Set the position in an asynchronous update
    /**
     * \detail This function allows the agent manager to set the position of 
     *         the agent.
     * \param pos The new position of the agent
    */
    void set_pos(const Position& pos) {
        _pos = pos;
    };

public:
    // -- Getters -- //
    /// Return the current position of the agent
    Position position() const { 
        return _pos;
    };

};



/// Agent Specialisation for synchronous update
template<typename Traits, typename Space>
class __Agent<Traits, Space, typename std::enable_if_t<Traits::sync == true>>
:
    public __Entity<Traits>
{   
public:
    /// The type of the state
    using State = typename Traits::State;

    /// The type of the Position
    using Position = typename Space::SpaceVec;
    
    /// Make the agent manager a friend
    template<class T, class M>
    friend class AgentManager;   
private:
    /// The current position of the agent 
    Position _pos;

    /// The position buffer for the synchronous position update
    Position _pos_new;

public:
    // -- Constructor -- //
    /// Construct an agent
    /** \detail id, initial state and initial position have to be passed
     * \param id The id of this agent
     * \param initial_state The initial state 
     * \param initial_pos The initial position
     */
    __Agent(const IndexType id, const State& initial_state, 
            const Position& initial_pos)
    :
        __Entity<Traits>(id, initial_state),
        _pos(initial_pos),
        _pos_new(initial_pos)
    {}

protected:
    /// Set the position in a synchronous update
    /**
     * \detail This function allows the agent manager to set the position of 
     *         the agent.
     * \param pos The position to be written to the position buffer
    */
    void set_pos(const Position& pos) {
        _pos_new = pos;
    }

public:
    // -- Getters -- //
    /// Return the current position of the agent
    Position position() const { 
        return _pos;
    }
    
    /// Return the position buffer of the agent
    Position position_new() const {
        return _pos_new;
    }

    /// Update the position and the state 
    /** \detail Writes the buffer of state and position to the current state
     *          and position. This is necessary for the synchronous update mode
     *          only. 
     */
    void update () {
        // Update the state as defined in the Entity class
        __Entity<Traits>::update();

        // Update the position
        _pos = _pos_new;
    }
};

// end group AgentManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_AGENT_HH
