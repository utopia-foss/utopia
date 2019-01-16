#ifndef UTOPIA_CORE_MANAGER_HH
#define UTOPIA_CORE_MANAGER_HH

// TODO Clean up includes
#include "../base.hh"
#include "../data_io/cfg_utils.hh"
#include "logging.hh"

#include "space.hh"
#include "cell_new.hh" // NOTE Final name will be cell.hh
#include "grid_new.hh" // NOTE Final name will be grid.hh
#include "types.hh"


namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

template<class CellTraits, class Model>
class CellManager {
public:
    /// Type of the managed cells
    using Cell = __Cell<CellTraits>; // NOTE Use Cell eventually

    /// The space type this cell manager maps to
    using Space = typename Model::Space;

    /// Dimensionality of the space this cell manager is to discretize
    static constexpr DimType dim = Model::Space::dim;

private:
    // -- Members ------------------------------------------------------------
    /// The logger (same as the model this manager resides in)
    std::shared_ptr<spdlog::logger> _log;

    /// The physical space the cells are to reside in
    std::shared_ptr<Space> _space;

    /// The grid that discretely maps cells into space
    std::shared_ptr<Grid<Space>> _grid;

    /// Storage container for cells
    CellContainer<Cell> _cells;

public:
    // -- Constructors -------------------------------------------------------
    /// Construct a cell manager from the model it resides in
    CellManager (Model& model)
    :
        _log(model.get_logger()),
        _space(model.get_space()),
        _grid(setup_grid(model.get_cfg())),
        _cells(setup_cells(model.get_cfg()))
    {}


    /// -- Getters -----------------------------------------------------------
    /// Return pointer to the space, for convenience
    std::shared_ptr<Space> space () const {
        return _space;
    }

    /// Return const reference to the grid
    std::shared_ptr<Grid<Space>> grid () const {
        return _grid;
    }

    /// Return const reference to the managed CA cells
    const CellContainer<Cell>& cells () const {
        return _cells;
    }


    // -- Public interface ---------------------------------------------------
    /// Access the neighbors of a specific cell
    CellContainer<Cell>& neighbors_of (Cell& cell) {
        // TODO distinguish between case where this is calculated and the case
        //      where it was computed in the beginning and stored in each cell

        // Neighbors were already stored; can access private cell member
        return cell._neighbors;
        // NOTE Allowed to do this because we're friends <3
    }

private:
    // -- Setup functions ----------------------------------------------------
    /// Set up the grid discretization from config parameters
    std::shared_ptr<Grid<Space>> setup_grid(const DataIO::Config& cfg) {
        // Check if the required parameter nodes are available
        if (!cfg["grid"]) {
            throw std::invalid_argument("Missing entry 'grid' in the config "
                                        "node supplied to the CellManager!");
        }
        else if (!cfg["grid"]["shape"] or !cfg["grid"]["discretization"]) {
            throw std::invalid_argument("Missing one or more of the grid "
                                        "configuration entries 'shape' and "
                                        "'discretization'.");
        }

        // Get the parameters
        auto grid_shape = as_<GridShapeType<dim>>(cfg["grid"]["shape"]); 
        auto disc_type = as_str(cfg["grid"]["discretization"]);
        
        // Distinguish by discretization
        if (disc_type == "tri" or disc_type == "triangular") {
            return std::make_shared<TriangularGrid<Space>>(_space, grid_shape);
        }
        else if (disc_type == "rect" or disc_type == "rectangular") {
            return std::make_shared<RectangularGrid<Space>>(_space, grid_shape);
        }
        else if (disc_type == "hex" or disc_type == "hexagonal") {
            return std::make_shared<HexagonalGrid<Space>>(_space, grid_shape);
        }
        else {
            throw std::invalid_argument("Invalid value for grid "
                                        "'discretization' argument: '"
                                        + disc_type + "'! Need be 'tri', "
                                        "'rect', 'hex'.");
        }
    }

    /// Set up the cells according to the discretization
    CellContainer<Cell> setup_cells(const DataIO::Config& cfg) {
        CellContainer<Cell> cont;
        return cont;
    }
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_MANAGER_HH
