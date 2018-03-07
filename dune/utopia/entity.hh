#ifndef ENTITY_HH
#define ENTITY_HH

#include <dune/utopia/state.hh>

namespace Utopia
{

/// Base class for Cells and Individuals, containing information on State and Traits
/** \tparam StateContainer Container of states
 *  \tparam Tags Tags
 *  \tparam IndexType Type of Index
 */
template<typename T, bool sync, class Tags, typename IndexType>
class Entity: 
    public StateContainer<T, sync>, 
    public Tags 
{
public:

    /// Constructor. Define state_container, tags and an ID index 
    Entity (const T state, IndexType index):
        StateContainer<T, sync>(state),
        Tags(),
        _id(index)
    { }

    /// Return entity ID
    IndexType id() const { return _id; }

private:

    const IndexType _id;

};

} // namespace Utopia

#endif // ENTITY_HH