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

    /// Type of the cell state
    using CellStateType = typename CellTraits::State;

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
    // TODO Consider making unique?!

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
        _cells(setup_cells())
    {}
    // TODO make possible to pass initial state explicitly


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
        const auto shape = as_<GridShapeType<dim>>(cfg["grid"]["shape"]); 
        auto disc_type = as_str(cfg["grid"]["discretization"]);
        
        // Distinguish by discretization
        if (disc_type == "tri" or disc_type == "triangular") {
            return std::make_shared<TriangularGrid<Space>>(_space, shape);
        }
        else if (disc_type == "rect" or disc_type == "rectangular") {
            return std::make_shared<RectangularGrid<Space>>(_space, shape);
        }
        else if (disc_type == "hex" or disc_type == "hexagonal") {
            return std::make_shared<HexagonalGrid<Space>>(_space, shape);
        }
        else {
            throw std::invalid_argument("Invalid value for grid "
                                        "'discretization' argument: '"
                                        + disc_type + "'! Need be 'tri', "
                                        "'rect', 'hex'.");
        }
    }


    /// Set up the cells container
    CellContainer<Cell> setup_cells() {
        CellContainer<Cell> cont;

        // Make sure the static Cell ID counter starts at 0
        if (Cell::_next_id != 0) {
            throw std::runtime_error("An instance of a cell of the same type "
                                     "as it is used in the CellManager was "
                                     "already initialized somewhere!");
        }

        // Construct all the cells using the default
        // TODO consider using auto-loop based on _grid and then providing the
        //      cells' IDs explicitly
        for (IndexType i=0; i<_grid->num_cells(); i++) {
            // Emplace new element using default constructor
            cont.emplace_back(std::make_shared<Cell>());
        }

        return cont;
    }

    // TODO add a setup function that allows setting up cell state via cfg node
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_MANAGER_HH
