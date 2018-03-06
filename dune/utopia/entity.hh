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
template<typename S,typename T, bool sync, class Tags, typename IndexType, std::size_t custom_neighborhood_count = 0>
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
    
    /// Return const reference to neighborhoods
    const std::array<std::vector<std::shared_ptr<Entity>>,custom_neighborhood_count>& neighborhoods () const { return _neighborhoods; }

    /// Return reference to neighborhoods
    std::array<std::vector<std::shared_ptr<Entity>>,custom_neighborhood_count>& neighborhoods () { return _neighborhoods; }

    

private:

    const IndexType _id;
    
    //! Custom neighborhood storage
    std::array<std::vector<std::shared_ptr<Cell>>,custom_neighborhood_count> _neighborhoods;
    
    

};

} // namespace Utopia

#endif // ENTITY_HH