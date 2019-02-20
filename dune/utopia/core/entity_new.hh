#ifndef UTOPIA_CORE_ENTITY_HH
#define UTOPIA_CORE_ENTITY_HH

// NOTE This file's final name will be entity.hh

#include "state.hh"
#include "tags.hh"
#include "space.hh"
#include "types.hh"

namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

/// Default struct for EntityTraits; use this one if there are no custom links
/** \tparam  EntityContainerType The type of the entity container. Note that the
  *                             entity container type need not be known to pass
  *                             this struct to the EntityTraits. The entity 
  *                             takes care of setting the EntityContainerType.
  */
template<typename EntityContainerType>
struct NoCustomLinks {};


/// The entity traits struct gathers types to be used for specializing an entity
/** \tparam  StateType    Type of the entitys' state container
  * \tparam  update_mode  The update mode of the entitys, sync or async
  * \tparam  use_def_state_constr   Whether to use the default constructor to
  *                       construct the entity's state. If false (default), a
  *                       constructor with DataIO::Config& as argument has to
  *                       be implemented for StateType.
  * \tparam  Tags         Custom entity tags
  * \tparam  CustomLinkContainers  Template template parameter to specify the
  *                       types of custom links. This construct is specialized
  *                       with the actual type of the entity container by the
  *                       EntityManager, so there's no need to know the entity
  *                       container type beforehand. To define your own custom
  *                       links, pass a template to a struct whose members are
  *                       containers of the objects you want to link to.
  */
template<typename StateType,
         UpdateMode update_mode,
         bool use_def_state_constr=false,
         typename EntityTags=EmptyTag,
         template<class> class CustomLinkContainers=NoCustomLinks>
struct EntityTraits {
    /// Type of the entitys' state container
    using State = StateType;

    /// Whether the entitys should be synchronously updated
    static constexpr bool sync = static_cast<bool>(update_mode);

    /// Whether to use the default constructor for constructing a entity state
    static constexpr bool use_default_state_constructor = use_def_state_constr;

    /// Custom entity tags
    using Tags = EntityTags;

    /// Template template parameter to specify type of custom links
    template<class EntityContainerType>
    using CustomLinks = CustomLinkContainers<EntityContainerType>;
};


/// An entity is a slightly specialized state container
/** \detail  It can be extended with the use of tags and can be associated with
  *          so-called "custom links". These specializations are carried into
  *          the entity by means of the EntityTraits struct.
  *          An entity is embedded into the EntityManager, where the 
  *          discretization allows assigning a position in space to the entity. 
  *          The entity itself does not know anything about that ...
  */
template<typename Traits>
class __Entity :   // NOTE Final name will be Entity
    public StateContainer<typename Traits::State, Traits::sync>,
    public Traits::Tags
{
public:
    /// The type of this entity
    using Self = __Entity<Traits>;

    /// The type of the state of this entity
    using State = typename Traits::State;

    /// Whether this entity is updated synchronously
    static constexpr bool sync = Traits::sync;

    /// The tags associated with this entity
    using Tags = typename Traits::Tags;

    /// A construct that has as members entity containers for custom-linked entitys
    using CustomLinkContainers = typename Traits::template CustomLinks<EntityContainer<Self>>;


private:
    // -- Members -- //
    /// ID of this entity
    const IndexType _id;

    /// Container for storing the _custom_ links of this entity
    CustomLinkContainers _custom_links;


public:
    // -- Constructors -- //
    /// Construct an entity
    __Entity(const IndexType id, const State initial_state)
    :
        // Store arguments and initialize Tags via default constructor
        StateContainer<State, sync>(initial_state),
        Tags(),
        _id(id),
        // Initialize custom links empty, i.e. with its default constructor
        _custom_links()
    {}


    // -- Public interface -- //
    /// Return const reference to entity ID
    const IndexType& id() const {
        return _id;
    }

    /// Return reference to custom link containers
    CustomLinkContainers& custom_links() {
        return _custom_links;
    }
    
    /// Return const reference to custom link containers
    const CustomLinkContainers& custom_links() const {
        return _custom_links;
    }
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_ENTITY_HH
