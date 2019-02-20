#ifndef UTOPIA_CORE_CELL_HH
#define UTOPIA_CORE_CELL_HH

// NOTE This file's final name will be cell.hh

#include "state.hh"
#include "tags.hh"
#include "space.hh"
#include "types.hh"
#include "entity_new.hh"       // NOTE Replace with entity.hh in final version

namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

///
/**
 * \ref Utopia::EntityTraits
 */
template<typename StateType, 
         UpdateMode update_mode,
         bool use_def_state_constr=false,
         typename CellTags=EmptyTag,
         template<class> class CustomLinkContainers=NoCustomLinks>
using CellTraits = EntityTraits<StateType, 
                                update_mode,
                                use_def_state_constr,
                                CellTags, 
                                CustomLinkContainers>;



/// A cell is a slightly specialized state container
/** \detail  It can be extended with the use of tags and can be associated with
  *          so-called "custom links". These specializations are carried into
  *          the cell by means of the CellTraits struct.
  *          A cell is embedded into the CellManager, where the discretization
  *          allows assigning a position in space to the cell. The cell itself
  *          does not know anything about that ...
  */
template<typename Traits>
class __Cell :   // NOTE Final name will be Cell
    public __Entity<Traits>
{
public:
    /// The type of the state
    using State = typename Traits::State;

    /// Construct a cell
    __Cell(const IndexType id, const State initial_state)
    :
        __Entity<Traits>(id, initial_state)
    {}
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_CELL_HH
