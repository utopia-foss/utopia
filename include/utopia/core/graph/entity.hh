#ifndef UTOPIA_CORE_GRAPH_ENTITY_HH
#define UTOPIA_CORE_GRAPH_ENTITY_HH

#include "../tags.hh"
#include "../entity.hh"

namespace Utopia {
/**
 *  \addtogroup GraphEntity
 *  \{
 */

/// GraphEntityTraits are just another name for Utopia::EntityTraits
template<typename StateType,
         Update update_mode,
         bool use_def_state_constr=false,
         typename GraphEntityTags=EmptyTag,
         template<class> class CustomLinkContainers=NoCustomLinks>
using GraphEntityTraits = EntityTraits<StateType,
                                update_mode,
                                use_def_state_constr,
                                GraphEntityTags,
                                CustomLinkContainers>;


template<typename Traits>
class GraphEntity :
    public Entity<Traits>
{
public:
    /// The type of the state
    using State = typename Traits::State;

    /// The id counter that counts how many graph entities exist
    static inline IndexType id_counter = 0;

    /// Construct a graph entity with empty initial state
    GraphEntity()
    :
        Entity<Traits>(id_counter, State{})
    {
        ++id_counter;
    }

    /// Construct a graph entity with initial state
    GraphEntity(const State initial_state)
    :
        Entity<Traits>(id_counter, initial_state)
    {
        ++id_counter;
    }

    /// Copy-construct a graph entity
    GraphEntity(const GraphEntity& ge):
        Entity<Traits>(id_counter, ge.state) 
    {
        ++id_counter;
    };
};

// end group GraphEntity
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRAPH_ENTITY_HH
