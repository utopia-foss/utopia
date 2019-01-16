#ifndef UTOPIA_CORE_CELL_HH
#define UTOPIA_CORE_CELL_HH

// NOTE This file's final name will be cell.hh
#include "state.hh"
#include "tags.hh"
#include "space.hh"
#include "types.hh"

namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

template<typename StateType, bool is_sync=true, typename CellTags=EmptyTag>
struct CellTraits {
    using State = StateType;
    static constexpr bool sync = is_sync;
    using Tags = CellTags;
};


template<typename Traits>
class __Cell :   // NOTE Final name will be Cell
    public StateContainer<typename Traits::State, Traits::sync>,
    public Traits::Tags
{
public:
    /// The type of this cell
    using Self = __Cell<Traits>;

    /// The type of the state of this cell
    using State = typename Traits::State;

    /// Whether this cell is updated synchronously
    static constexpr bool sync = Traits::sync;

    /// The tags associated with this cell
    using Tags = typename Traits::Tags;


private:
    template<class Space, class CellManagerTraits>
    friend class CellManager;

    // -- Members -- //
    /// Variable holding the ID of the next to be initialized cell of this type
    static IndexType _next_id;

    /// ID of this cell
    const IndexType _id;

    /// Container for storing the neighborhood of this cell; need not be filled
    CellContainer<Self> _neighbors;


public:
    // -- Constructors -- //
    /// Construct with default state constructor
    __Cell()
    :
        StateContainer<State, sync>(), // FIXME
        Tags(),
        // Initialize ID by incrementing static member
        _id(_next_id++),
        // Initialize neighborhood empty
        _neighbors()
    {}

    /// Construct with initial state
    __Cell(const State state)
    :
        StateContainer<State, sync>(state),
        Tags(),
        // Initialize ID by incrementing static member
        _id(_next_id++),
        // Initialize neighborhood empty
        _neighbors()
    {}

    // -- Interface -- //
    /// Return entity ID
    IndexType id() const {
        return _id;
    }
};

template<typename Traits>
inline IndexType __Cell<Traits>::_next_id = 0;


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_CELL_HH
