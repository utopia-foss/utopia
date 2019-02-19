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
         bool is_sync=true, 
         typename AgentTags=EmptyTag,
         template<class> class CustomLinkContainers=NoCustomLinks>
using AgentTraits = EntityTraits<StateType, 
                                is_sync, 
                                AgentTag, 
                                CustomLinkContainers>;



/// An agent is a slightly specialized state container
/** \detail  It can be extended with the use of tags and can be associated with
  *          so-called "custom links". These specializations are carried into
  *          the agent by means of the AgentTraits struct.
  *          An agent is embedded into the AgentManager, where the discretization
  *          allows assigning a position in space to the agent. The agent itself
  *          does not know anything about that ...
  */
template<typename Traits>
class __Agent :   // NOTE Final name will be Agent
    public __Entity<Traits>
{
public:
    /// The type of the state
    using State = typename Traits::State;

    // -- Constructors -- //
    /// Construct an agent
    __Agent(const IndexType id, const State initial_state)
    :
        __Entity<Traits>(id, initial_state)
    {}
};


// end group AgentManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_AGENT_HH
