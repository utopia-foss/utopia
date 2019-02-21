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

///
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
  *          allows assigning a position in space to the agent. The agent itself
  *          does not know anything about that ...
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

    /// Make the agent manager a friend
    template<class T, class M>
    friend class AgentManager;   

private:
    /// Position
    Position _pos;


public:
    // -- Constructors -- //
    /// Construct an agent
    __Agent(const IndexType id, const State initial_state, 
            const Position& initial_pos)
    :
        __Entity<Traits>(id, initial_state),
        _pos(initial_pos)    
    {}

    __Agent(const IndexType id, const State initial_state)
    :
        __Entity<Traits>(id, initial_state),
        _pos()    
    {}


protected:
    // set the position
    void set_pos(const Position& pos) {
        _pos = pos;
    };

public:
    // get the position
    Position position ( ) { return _pos;};

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
    /// Position
    Position _pos;

    /// Position buffer
    Position _pos_new;

public:
    // -- Constructors -- //
    /// Construct an agent
    __Agent(const IndexType id, const State& initial_state, 
            const Position& initial_pos)
    :
        __Entity<Traits>(id, initial_state),
        _pos(initial_pos)
    {}

    __Agent(const IndexType id, const State& initial_state)
    :
        __Entity<Traits>(id, initial_state),
        _pos()
    {}

protected:
    // set the position
    void set_pos(const Position& pos) {
        _pos_new = pos;
    }

public:
    // get the position
    Position position() { 
        return _pos;
    }

    // update the position and the state 
    void update () {
        // Update the state as defined in the Entity class
        __Entity<Traits>::update();

        // update the position
        _pos = _pos_new;
    }
};

// end group AgentManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_AGENT_HH
