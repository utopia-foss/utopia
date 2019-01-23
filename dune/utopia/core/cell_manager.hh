#ifndef UTOPIA_CORE_MANAGER_HH
#define UTOPIA_CORE_MANAGER_HH

#include <type_traits>

// TODO Clean up includes
#include "../base.hh"
#include "../data_io/cfg_utils.hh"
#include "logging.hh"
#include "types.hh"

#include "cell_new.hh"          // NOTE Final name will be cell.hh
#include "grid_new.hh"          // NOTE Final name will be grid.hh
#include "neighborhoods_new.hh" // NOTE Final name will be neighborhood.hh


namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */



template<class CellTraits, class Model>
class CellManager {
public:
    /// The type of this CellManager
    using Self = CellManager<CellTraits, Model>;

    /// Type of the managed cells
    using Cell = __Cell<CellTraits>; // NOTE Use Cell eventually

    /// Type of the cell state
    using CellStateType = typename CellTraits::State;

    /// The space type this cell manager maps to
    using Space = typename Model::Space;

    /// Dimensionality of the space this cell manager is to discretize
    static constexpr DimType dim = Model::Space::dim;

    /// Grid type; this refers to the base type of the stored (derived) object
    using GridType = Grid<Space>;

    /// Neighborhood function used in public interface with (cell as argument)
    using NBFuncCell = std::function<const CellContainer<Cell>&(Cell&)>;

    /// Neighborhood function used in internal interface (ID as argument)
    using NBFuncID = Neighborhoods::NBFuncID<GridType>;


private:
    // -- Members ------------------------------------------------------------
    /// The logger (same as the model this manager resides in)
    std::shared_ptr<spdlog::logger> _log;

    /// The physical space the cells are to reside in
    std::shared_ptr<Space> _space;

    /// The grid that discretely maps cells into space
    std::shared_ptr<GridType> _grid;

    /// Storage container for cells
    CellContainer<Cell> _cells;

    /// Storage container for pre-calculated (!) cell neighbors
    std::vector<CellContainer<Cell>> _cell_neighbors;

    /// The currently chosen neighborhood mode, i.e. "moore", "vonNeumann", ...
    std::string _nb_mode;

    /// The currently chosen neighborhood function
    NBFuncID _nb_func;

public:
    /// Retrieve the given cell's neighbors
    /** \detail This member is a function that is changed depending on whether
      *         the cell neighbors are computed every time using the grid or
      *         whether the neighbors were calculated once and stored.
      */
    NBFuncCell neighbors_of;


public:
    // -- Constructors -------------------------------------------------------
    /// Construct a cell manager
    /** \detail With the model available, the CellManager can extract the
      *         required information from the model without the need to pass
      *         it explicitly. Furthermore, this constructor differs to the
      *         one with the `initial_state` parameter such that the way the
      *         initial state of the cells is determined can be controlled via
      *         the configuration.
      * 
      * \param  model       The model this CellManager belongs to  
      * \param  custom_cfg  A custom config node to use to use for grid and
      *                     cell setup. If not given, the model's configuration
      *                     is used to extract the required entries.
      */
    CellManager (Model& model,
                 const DataIO::Config& custom_cfg = {})
    :
        _log(model.get_logger()),
        _space(model.get_space()),
        _grid(setup_grid(model, custom_cfg)),
        _cells(setup_cells(model, custom_cfg)),
        _cell_neighbors(),
        _nb_mode(),
        _nb_func(get_nb_func("empty"))
    {
        // Set neighborhood function to one that throws an error
        neighbors_of = _nb_unset;
    }
    

    /// Construct a cell manager explicitly passing an initial cell state
    /** \param  model          The model this CellManager belongs to  
      * \param  initial_state  The initial state of the cells
      * \param  custom_cfg     A custom config node to use to use for grid and
      *                        cell setup. If not given, the model's
      *                        configuration is used to extract the required
      *                        entries.
      */
    CellManager (Model& model,
                 const CellStateType initial_state,
                 const DataIO::Config& custom_cfg = {})
    :
        _log(model.get_logger()),
        _space(model.get_space()),
        _grid(setup_grid(model, custom_cfg)),
        _cells(setup_cells(initial_state)),
        _cell_neighbors(),
        _nb_mode(),
        _nb_func(get_nb_func("empty"))
    {
        // Set neighborhood function to one that throws an error
        neighbors_of = _nb_unset;
    }


    /// -- Getters -----------------------------------------------------------
    /// Return pointer to the space, for convenience
    const std::shared_ptr<Space>& space () const {
        return _space;
    }

    /// Return const reference to the grid
    const std::shared_ptr<GridType>& grid () const {
        return _grid;
    }

    /// Return const reference to the managed CA cells
    const CellContainer<Cell>& cells () const {
        return _cells;
    }


    // -- Public interface ---------------------------------------------------
    /// Set the neighborhood mode
    void set_neighborhood_mode(std::string nb_mode,
                               bool compute_neighbors_now = false)
    {
        // TODO * Check if mode differs from the one currently set
        //      * Check if _cell_neighbors needs to be invalidated
        //      * If configured, re-calculate all cell neighbors
        //      * Set the corresponding neighbors_of method accordingly

        if (nb_mode != _nb_mode) {
            _log->info("Setting neighborhood mode to '{}' ...", nb_mode);

            // Retrieve the neighborhood function and store as member
            _nb_func = get_nb_func(nb_mode);

            // Adjust the frontend methods

            // Clear the no-longer valid neighborhood relationships
            if (_cell_neighbors.size() > 0) {
                _cell_neighbors.clear();
                _log->debug("Cleared cell neighborhood cache.");
            }

            // Everything ok, now set the member variable
            _nb_mode = nb_mode;
            _log->info("Set neighborhood mode to '{}'.", _nb_mode);
        }
        else {
            _log->debug("Neighborhood mode already was '{}'. Not changing.",
                        _nb_mode);
        }

        if (compute_neighbors_now) {
            compute_cell_neighbors();
        }
    }

    /// Compute (and store) all cells' neighbors
    /** \detail After this function was called, the cell neighbors will be
      *         returned from the storage container rather than re-calculated
      *         for every access.
      */
    void compute_cell_neighbors() {
        _log->info("Computing and storing '{}' neighbors of all {} cells ...",
                   _nb_mode, _cells.size());

        // TODO implement the below
        // Compute cell neighbors

        // Store cell neighbors in _cell_neighbors member

        // Change access function to access the storage rather than computing

        _log->info("Computed and stored cell neighbors.");
    }


private:
    // -- Helper functions ---------------------------------------------------
    /// Given a container of cell IDs, convert it to container of cell pointers
    CellContainer<Cell> cells_from_ids(IndexContainer& ids) {
        // Initialize container to be returned and fix it in size
        CellContainer<Cell> ret;
        ret.reserve(ids.size());

        for (const auto& id : ids) {
            ret.emplace_back(std::shared_ptr<Cell>(_cells.at(id)));
        }

        return ret;
    }

    /// Given a container of cell IDs, convert it to container of cell pointers
    /** \detail
      */ 
    CellContainer<Cell> cells_from_ids(IndexContainer&& ids) {
        // Initialize container to be returned and fix it in size
        CellContainer<Cell> ret;
        ret.reserve(ids.size());

        for (const auto& id : ids) {
            ret.emplace_back(std::shared_ptr<Cell>(_cells.at(id)));
        }

        return ret;
    }


    // -- Functions to use for getting neighbors -----------------------------
    /// Return the pre-computed neighbors of the given cell
    NBFuncCell _nb_from_cache = [this](Cell& cell) {
        return this->_cell_neighbors.at(cell.id());
    };

    /// Compute the neighbors for the given cell using the grid
    NBFuncCell _nb_compute_each_time = [this](Cell& cell) {
        // auto ids = _nb_func(cell.id(), grid());
        return cells_from_ids(_nb_func(cell.id(), grid()));
    };

    /// Compute the neighbors for the given cell using the grid
    NBFuncCell _nb_unset = [](Cell&) {
        throw std::runtime_error("Cannot compute neighborhood because the "
            "neighborhood mode was not set! Make sure to specify it either "
            "at initialization of the CellManager or by calling the "
            "`set_neighborhood_mode` method.");

        // Need return statement (here with empty cell container) to fit type
        return CellContainer<Cell>();
    };


    // -- Setup functions ----------------------------------------------------
    /// Set up the grid discretization from config parameters
    std::shared_ptr<GridType> setup_grid(Model& model,
                                         const DataIO::Config& custom_cfg)
    {
        // Determine which configuration to use
        auto cfg = model.get_cfg();

        if (custom_cfg.size() > 0) {
            _log->debug("Using custom config for grid setup ...");
            cfg = custom_cfg;
        }
        else {
            _log->debug("Using '{}' model config for grid setup ...",
                        model.get_name());
        }

        // Check if the required parameter nodes are available
        if (!cfg["grid"]) {
            throw std::invalid_argument("Missing entry 'grid' in the "
                "configuration node supplied to the CellManager! Check that "
                "the model configuration includes such an entry.");
        }
        else if (!cfg["grid"]["shape"] or !cfg["grid"]["structure"]) {
            throw std::invalid_argument("Missing one or both of the grid "
                "configuration entries 'shape' and 'structure'.");
        }
        
        // Get the parameters: shape and structure type
        const auto shape = as_<GridShapeType<dim>>(cfg["grid"]["shape"]); 
        auto structure = as_str(cfg["grid"]["structure"]);

        _log->info("Setting up {}ly structured grid discretization ...",
                   structure);
        // TODO inform about shape?
        
        // Create the respective grids, distinguishing by structure
        // TODO consider passing config node to make more arguments available
        if (structure == "triagonal") {
            using GridSpec = TriangularGrid<Space>;
            return std::make_shared<GridSpec>(_space, shape);
        }
        else if (structure == "rectangular") {
            using GridSpec = RectangularGrid<Space>;
            return std::make_shared<GridSpec>(_space, shape);
        }
        else if (structure == "hexagonal") {
            using GridSpec = HexagonalGrid<Space>;
            return std::make_shared<GridSpec>(_space, shape);
        }
        else {
            throw std::invalid_argument("Invalid value for grid "
                "'structure' argument: '" + structure + "'! Allowed "
                "values: 'rectangular', 'hexagonal', 'triagonal'");
        }
    }


    /// Set up the cells container
    CellContainer<Cell> setup_cells(const CellStateType initial_state) {
        CellContainer<Cell> cont;

        // Construct all the cells using the default
        // TODO consider using some construct provided by _grid
        for (IndexType i=0; i<_grid->num_cells(); i++) {
            // Emplace new element using default constructor
            cont.emplace_back(std::make_shared<Cell>(i, initial_state));
        }

        // Done. Shrink it.
        cont.shrink_to_fit();
        _log->info("Populated cell container with {:d} cells.", cont.size());

        return cont;
    }

    
    /// Set up cells container via initial state from config or default constr
    /** \detail This function creates an initial state object and then passes
      *         over to setup_cells(initial_state). It checks whether the
      *         CellStateType is constructible via a config node and if the
      *         config entries to construct it are available. It can fall back
      *         to try the default constructor to construct the object. If both
      *         are not possible, a compile-time error message is emitted.
      */
    CellContainer<Cell> setup_cells(Model& model,
                                    const DataIO::Config& custom_cfg)
    {
        // Determine which configuration to use
        auto cfg = model.get_cfg();

        if (custom_cfg.size() > 0) {
            _log->debug("Using custom config for cell setup ...");
            cfg = custom_cfg;
        }
        else {
            _log->debug("Using '{}' model config for cell setup ...",
                        model.get_name());
        }

        // Find out the cell initialization mode
        if (!cfg["cell_initialize_from"]) {
            throw std::invalid_argument("Missing required configuration key "
                "'cell_initialize_from' for setting up cells via the "
                "configuration.");
        }
        const auto cell_init_from = as_str(cfg["cell_initialize_from"]);

        _log->info("Creating initial cell state using '{}' constructor ...",
                   cell_init_from);

        // Find out if the initial state is constructible via a config node and
        // setup the cells with that information, if configured to do so.
        if constexpr (std::is_constructible<CellStateType, DataIO::Config&>()){
            // Find out if this constructor was set to be used
            if (cell_init_from == "config") {
                // Yes. Should now check if the required config parameters were
                // also provided and add helpful error message
                if (!cfg["cell_initial_state"]) {
                    throw std::invalid_argument("Was configured to create the "
                        "initial cell state from a config node but a node "
                        "with the key 'cell_initial_state' was not provided!");
                }

                // Everything ok. Create state object and pass it on ...
                return setup_cells(CellStateType(cfg["cell_initial_state"]));
            }
            // else: do not return but continue with the rest of the function,
            // i.e. trying the other constructors
        }
        
        // Either not Config-constructible or not configured to do so.

        // TODO could add a case here where the cell state constructor takes
        //      care of setting up each _individual_ cell such that cells can
        //      have varying initial states.

        // Last resort: Can and should the default constructor be used?
        if constexpr (std::is_default_constructible<CellStateType>()) {
            if (cell_init_from == "default") {
                // Construct a cell state and pass it on
                return setup_cells(CellStateType{});
            }
        }
        
        // If we reached this point, construction does not work.
        // TODO Consider a compile-time error message if possible?!
        throw std::invalid_argument("No valid constructor for the cells' "
            "initial state was available! Check that the config parameter "
            "'cell_initialize_from' is valid (was: '" + cell_init_from + "', "
            "may be 'config' or 'default') and make sure CellTraits::State is "
            "constructible via the chosen way: "
            "This requires either `const Utopia::DataIO::Config&` as argument "
            "or being default-constructible, respectively. Alternatively, "
            "pass the initial state directly to the CellManager constructor.");
    }


    /// Retrieve a neighborhood function from its mode
    NBFuncID get_nb_func(std::string name) {
        if (name == "empty") {
            return Neighborhoods::AllAlone<GridType>;
        }
        // TODO add others here
        else {
            throw std::invalid_argument("Invalid nb_func name '"+name+"'!");
        }
    }
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_MANAGER_HH
