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

/// Default struct for CellTraits; use this one if there are no custom links
/** \tparam  CellContainerType  The type of the cell container. Note that the
  *                             cell container type need not be known to pass
  *                             this struct to the CellTraits. The cell takes
  *                             care of setting the CellContainerType.
  */
template<typename CellContainerType>
struct NoCustomLinks {};



/// The cell traits struct gathers types to be used for specializing a Cell
/** \tparam  StateType    Type of the cells' state container
  * \tparam  update_mode  The update mode of the cells, sync or async
  * \tparam  use_def_state_constr   Whether to use the default constructor to
  *                       construct the cell's state. If false (default), a
  *                       constructor with DataIO::Config& as argument has to
  *                       be implemented for StateType.
  * \tparam  Tags         Custom cell tags
  * \tparam  CustomLinkContainers  Template template parameter to specify the
  *                       types of custom links. This construct is specialized
  *                       with the actual type of the cell container by the
  *                       CellManager, so there's no need to know the cell
  *                       container type beforehand. To define your own custom
  *                       links, pass a template to a struct whose members are
  *                       containers of the objects you want to link to.
  */
template<typename StateType,
         UpdateMode update_mode,
         bool use_def_state_constr=false,
         typename CellTags=EmptyTag,
         template<class> class CustomLinkContainers=NoCustomLinks>
struct CellTraits {
    /// Type of the cells' state container
    using State = StateType;

    /// Whether the cells should be synchronously updated
    static constexpr bool sync = static_cast<bool>(update_mode);

    /// Whether to use the default constructor for constructing a cell state
    static constexpr bool use_default_state_constructor = use_def_state_constr;

    /// Custom cell tags
    using Tags = CellTags;

    /// Template template parameter to specify type of custom links
    template<class CellContainerType>
    using CustomLinks = CustomLinkContainers<CellContainerType>;
};



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

    /// A construct that has as members cell containers for custom-linked cells
    using CustomLinkContainers = typename Traits::template CustomLinks<CellContainer<Self>>;


private:
    // -- Members -------------------------------------------------------------
    /// ID of this cell
    const IndexType _id;

    /// Container for storing the _custom_ links of this cell
    CustomLinkContainers _custom_links;


public:
    // -- Constructors --------------------------------------------------------
    /// Construct a cell
    __Cell(const IndexType id, const State initial_state)
    :
        // Store arguments and initialize Tags via default constructor
        StateContainer<State, sync>(initial_state),
        Tags(),
        _id(id),
        // Initialize custom links empty, i.e. with its default constructor
        _custom_links()
    {}


    // -- Public interface ----------------------------------------------------
    /// Return const reference to cell ID
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

#endif // UTOPIA_CORE_CELL_HH
