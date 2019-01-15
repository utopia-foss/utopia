#ifndef UTOPIA_CORE_MANAGER_HH
#define UTOPIA_CORE_MANAGER_HH

// TODO Clean up includes
#include "../base.hh"
#include "../data_io/cfg_utils.hh"

#include "space.hh"
#include "cell_new.hh" // NOTE Final name will be cell.hh
#include "grid_new.hh" // NOTE Final name will be grid.hh
#include "types.hh"


namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

template<class Model, class CellTraits>
class CellManager {
public:
    /// Type of the managed cells
    using Cell = __Cell<CellTraits>; // NOTE Use Cell eventually

    /// The space type this cell manager maps to
    using Space = typename Model::Space;

    /// Dimension
    static constexpr DimType dim = Model::Space::dim;

private:
    // -- Members ------------------------------------------------------------
    /// The physical space the cells are to reside in
    std::shared_ptr<Space> _space;

    /// The grid that discretely maps cells into space
    Grid<Space> _grid;

    /// Storage container for cells
    CellContainer<Cell> _cells;

public:
    // -- Constructors -------------------------------------------------------
    /// Construct a cell manager from a space and a config node
    CellManager (const Space &space, const DataIO::Config &cfg)
    :
        _space(space),
        _grid(setup_grid(cfg)),
        _cells(setup_cells(cfg))
    {}

    /// Construct a cell manager from the model it resides in
    CellManager (Model &model)
    :
        CellManager(model.get_space(), model.get_cfg())
    {}

private:
    // -- Setup functions ----------------------------------------------------
    Grid<Space> setup_grid(DataIO::Config& cfg) {
        return Grid<Space>();
    }

    CellContainer<Cell> setup_cells(DataIO::Config& cfg) {

    }


    // -- Public interface ---------------------------------------------------
    /// Return const reference to the managed cells
    const CellContainer<Cell>& cells () const {
        return _cells;
    }

    CellContainer<Cell>& get_neighbors (Cell& cell) {
        // Access private cell member
        return cell._neighbors;
        // NOTE Can do this because we're friends <3
    }
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_MANAGER_HH
