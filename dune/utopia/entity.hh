#ifndef ENTITY_HH
#define ENTITY_HH

namespace Utopia
{

/// Base class for Cells and Individuals, containing information on State and Traits
/** \tparam StateContainer Container of states
 *  \tparam Tags Tags
 *  \tparam IndexType Type of Index
 */
template<class StateContainer, class Tags, typename IndexType>
class Entity:
    public StateContainer,
    public Tags 
{
public:

    /// Constructor. Define state_container, tags and an ID index 
    Entity (const StateContainer state_container, const Tags tags, IndexType index):
        StateContainer(state_container),
        Tags(tags),
        _id(index)
    { }

    /// Return entity ID
    IndexType id() const { return _id; }

private:

    const IndexType _id;

};

} // namespace Utopia

#endif // ENTITY_HH
