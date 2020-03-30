#ifndef UTOPIA_CORE_GRAPH_ENTITY_HH
#define UTOPIA_CORE_GRAPH_ENTITY_HH

#include "../tags.hh"
#include "../types.hh"
#include "../entity.hh"

namespace Utopia {
/**
 *  \ingroup Entity Graph
 *  \addtogroup GraphEntity
 *  \{
 */

/// GraphEntityTraits are mainly just another name for Utopia::EntityTraits
/** The only difference is that the update_mode is restricted as default to
 *  Update::manual and a default constructor is required.
 */
template<typename StateType,
         typename GraphEntityTags=EmptyTag,
         template<class> class CustomLinkContainers=NoCustomLinks>
using GraphEntityTraits = EntityTraits<StateType,
                                Update::manual,
                                false,          // use_def_state_constr
                                GraphEntityTags,
                                CustomLinkContainers>;


/// A graph entity is a slightly specialized state container
/** It can be extended with the use of tags and can be associated with
  * so-called "custom links". These specializations are carried into
  * the graph entity by means of the GraphEntityTraits struct.
  * A graph entity should be used as the base class for a Vertex and
  * an Edge that is placed on a boost::graph graph object.
  *
  * \tparam Traits  Valid Utopia::EntityTraits, describing the type of graph
  *                 entity
  */
template<typename Traits>
class GraphEntity :
    public Entity<GraphEntity<Traits>, Traits>
{
public:
    /// The type of this GraphEntity
    using Self = GraphEntity<Traits>;

    /// The type of the state
    using State = typename Traits::State;

private:
    /// The id counter that counts how many graph entities exist
    static inline IndexType id_counter = 0;

public:
    /// Construct a graph entity with empty initial state
    GraphEntity()
    :
        Entity<Self, Traits>(id_counter++, State{})
    {};

    /// Construct a graph entity with initial state
    GraphEntity(const State& initial_state)
    :
        Entity<Self, Traits>(id_counter++, initial_state)
    {};

    /// Copy-construct a graph entity
    GraphEntity(const GraphEntity& ge):
        Entity<Self, Traits>(id_counter++, ge.state)
    {};

    /// Copy-assign a graph entity
    GraphEntity& operator=(GraphEntity ge){
        std::swap(this->state, ge.state);
        std::swap(this->custom_links(), ge.custom_links());
        return *this;
    }

    /// Get the id counter
    IndexType get_id_counter(){
        return id_counter;
    }
};

// end group GraphEntity
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_GRAPH_ENTITY_HH
