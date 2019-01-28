#ifndef UTOPIA_CORE_MANAGER_HH
#define UTOPIA_CORE_MANAGER_HH

#include <type_traits>

// TODO Clean up includes
#include "../base.hh"
#include "../data_io/cfg_utils.hh"
#include "logging.hh"
#include "types.hh"

#include "cell_new.hh"          // NOTE Final name will be cell.hh
#include "grids.hh"


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
    static constexpr std::size_t dim = Model::Space::dim;

    /// Grid type; this refers to the base type of the stored (derived) object
    using GridType = Grid<Space>;

    /// Neighborhood function used in public interface (with cell as argument)
    using NBFuncCell = std::function<CellContainer<Cell>(const Cell&)>;


private:
    // -- Members ------------------------------------------------------------
    /// The logger (same as the model this manager resides in)
    const std::shared_ptr<spdlog::logger> _log;

    /// Cell manager configuration node
    const DataIO::Config _cfg;

    /// The physical space the cells are to reside in
    const std::shared_ptr<Space> _space;

    /// The grid that discretely maps cells into space
    const std::shared_ptr<GridType> _grid;

    /// Storage container for cells
    CellContainer<Cell> _cells;

    /// Storage container for pre-calculated (!) cell neighbors
    std::vector<CellContainer<Cell>> _cell_neighbors;

    /// The currently chosen neighborhood mode, i.e. "moore", "vonNeumann", ...
    NBMode _nb_mode;

    /// The currently chosen neighborhood function (working directly on cells)
    NBFuncCell _nb_func;


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
        _cfg(setup_cfg(model, custom_cfg)),
        _space(model.get_space()),
        _grid(setup_grid()),
        _cells(setup_cells()),
        _cell_neighbors(),
        _nb_mode(NBMode::empty)
    {
        // Set default value for _nb_func
        _nb_func = _nb_compute_each_time_empty;

        // Use setup function to set up neighborhood from configuration
        setup_nb_funcs();

        _log->info("CellManager is all set up.");
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
        _cfg(setup_cfg(model, custom_cfg)),
        _space(model.get_space()),
        _grid(setup_grid()),
        _cells(setup_cells(initial_state)),
        _cell_neighbors(),
        _nb_mode(NBMode::empty)
    {
        // Set default value for _nb_func
        _nb_func = _nb_compute_each_time_empty;

        // Use setup function to set up neighborhood from configuration
        setup_nb_funcs();

        _log->info("CellManager is all set up.");
    }


    /// -- Getters -----------------------------------------------------------
    /// Return pointer to the space, for convenience
    const std::shared_ptr<Space>& space () const {
        return _space;
    }

    /// Return const reference to the grid
    std::shared_ptr<const GridType> grid () const {
        return _grid;
    }

    /// Return const reference to the managed CA cells
    const CellContainer<Cell>& cells () const {
        return _cells;
    }

    /// Return const reference to the neighborhood mode
    NBMode nb_mode() {
        return _nb_mode;
    }


    // -- Public interface ---------------------------------------------------
    /// Retrieve the given cell's neighbors
    /** \detail The behaviour of this method is different depending on the
      *         choice of neighborhood.
      */
    CellContainer<Cell> neighbors_of(const Cell& cell) const {
        return _nb_func(cell);
    }


    /// Retrieve the given cell's neighbors
    /** \detail The behaviour of this method is different depending on the
      *         choice of neighborhood.
      */
    CellContainer<Cell> neighbors_of(const std::shared_ptr<Cell>& cell) const {
        return _nb_func(*cell);
    }


    /// Set the neighborhood mode
    void select_neighborhood(std::string nb_mode,
                             bool compute_and_store = false) {
        if (not nb_mode_from_string.count(nb_mode)) {
            throw std::invalid_argument("Could not translate given value for "
                "neighborhood mode ('" + nb_mode + "') to valid enum entry!");
        }

        select_neighborhood(nb_mode_from_string.at(nb_mode),
                            compute_and_store);
    }

    /// Set the neighborhood mode
    void select_neighborhood(NBMode nb_mode,
                             bool compute_and_store = false)
    {
        // Only change the neighborhood, if it is different to the existing
        // one or if it is set to be empty
        if ((nb_mode != _nb_mode) or (nb_mode == NBMode::empty)) {
            _log->info("Selecting '{}' neighborhood ...",
                       nb_mode_to_string.at(nb_mode));

            // Tell the grid which mode to use
            _grid->select_neighborhood(nb_mode);

            // Adjust function object that the public interface calls
            if (nb_mode == NBMode::empty) {
                // Issue a warning alongside the neighborhood calculation
                _nb_func = _nb_compute_each_time_empty;
            }
            else {
                // Compute the cell neighbors each time
                _nb_func = _nb_compute_each_time;
            }

            // Clear the no-longer valid neighborhood relationships
            if (_cell_neighbors.size() > 0) {
                _cell_neighbors.clear();
                _log->debug("Cleared cell neighborhood cache.");
            }

            // Everything ok, now set the member variable
            _nb_mode = nb_mode;
            _log->debug("Successfully selected '{}' neighborhood.",
                        nb_mode_to_string.at(_nb_mode));
        }
        else {
            _log->debug("Neighborhood was already set to '{}'; not changing.",
                        nb_mode_to_string.at(_nb_mode));
        }

        // Still allow to compute the neighbors, regardless of all the above
        if (compute_and_store) {
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
                   nb_mode_to_string.at(_nb_mode), _cells.size());

        // Clear cell neighbors container and pre-allocate space
        _cell_neighbors.clear();
        _cell_neighbors.reserve(_cells.size());

        // Compute cell neighbors and store them
        for (auto cell : _cells) {
            _cell_neighbors.push_back(neighbors_of(cell));
        }

        // Change access function to access the storage directly. Done.
        _nb_func = _nb_from_cache;
        _log->info("Computed and stored cell neighbors.");
    }


private:
    // -- Helper functions ---------------------------------------------------
    // ...


    // -- Helpers for Neighbors interface ------------------------------------

    /// Given a container of cell IDs, convert it to container of cell pointers
    CellContainer<Cell> cells_from_ids(IndexContainer& ids) {
        // Initialize container to be returned and fix it in size
        CellContainer<Cell> ret;
        ret.reserve(ids.size());

        for (const auto& id : ids) {
            ret.emplace_back(std::shared_ptr<Cell>(_cells[id]));
        }

        return ret;
    }

    /// Given a container of cell IDs, convert it to container of cell pointers
    /** \detail The rvalue reference version of this method allows calling with
      *         a temporary object.
      */ 
    CellContainer<Cell> cells_from_ids(IndexContainer&& ids) {
        // Initialize container to be returned and fix it in size
        CellContainer<Cell> ret;
        ret.reserve(ids.size());

        for (const auto& id : ids) {
            ret.emplace_back(std::shared_ptr<Cell>(_cells[id]));
        }

        return ret;
    }
    
    // .. std::functions to call from neighbors_of ...........................
    /// Return the pre-computed neighbors of the given cell
    NBFuncCell _nb_from_cache = [this](const Cell& cell) {
        return this->_cell_neighbors[cell.id()];
    };

    /// Compute the neighbors for the given cell using the grid
    NBFuncCell _nb_compute_each_time = [this](const Cell& cell) {
        return this->cells_from_ids(this->_grid->neighbors_of(cell.id()));
    };

    /// Compute the neighbors for the given cell using the grid
    NBFuncCell _nb_compute_each_time_empty = [this](const Cell& cell) {
        this->_log->warn("No neighborhood selected! Calls to the "
            "CellManager::neighbors_of method will always return an empty "
            "container.");
        return this->cells_from_ids(this->_grid->neighbors_of(cell.id()));
    };


    // -- Setup functions ----------------------------------------------------

    /// Set up the cell manager configuration member
    /** \detail This function determines whether to use a custom configuration
      *         or the one provided by the model this CellManager belongs to
      */
    DataIO::Config setup_cfg(Model& model, const DataIO::Config& custom_cfg) {
        auto cfg = model.get_cfg();

        if (custom_cfg.size() > 0) {
            _log->debug("Using custom config for cell manager setup ...");
            cfg = custom_cfg;
        }
        else {
            _log->debug("Using '{}' model's configuration for cell manager "
                        "setup ... ", model.get_name());
        }
        return cfg;
    }

    /// Set up the grid discretization
    std::shared_ptr<GridType> setup_grid() {
        // Check if the required parameter nodes are available
        if (not _cfg["grid"]) {
            throw std::invalid_argument("Missing entry 'grid' in the "
                "configuration node supplied to the CellManager! Check that "
                "the model configuration includes such an entry.");
        }
        else if (not _cfg["grid"]["shape"] or not _cfg["grid"]["structure"]) {
            throw std::invalid_argument("Missing one or both of the grid "
                "configuration entries 'shape' and 'structure'.");
        }
        
        // Get the parameters: shape and structure type
        const auto shape = as_<GridShapeType<dim>>(_cfg["grid"]["shape"]); 
        auto structure = as_str(_cfg["grid"]["structure"]);

        _log->info("Setting up {}ly structured grid discretization ...",
                   structure);
        // TODO inform about shape?
        
        // Create the respective grids, distinguishing by structure
        // TODO consider passing config node to make more arguments available
        if (structure == "triangular") {
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
                "values: 'rectangular', 'hexagonal', 'triangular'");
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
    CellContainer<Cell> setup_cells() {
        // Find out the cell initialization mode
        if (not _cfg["cell_initialize_from"]) {
            throw std::invalid_argument("Missing required configuration key "
                "'cell_initialize_from' for setting up cells via a "
                "DataIO::Config& constructor or default constructor.");
        }
        const auto cell_init_from = as_str(_cfg["cell_initialize_from"]);

        _log->info("Creating initial cell state using '{}' constructor ...",
                   cell_init_from);

        // Find out if the initial state is constructible via a config node and
        // setup the cells with that information, if configured to do so.
        if constexpr (std::is_constructible<CellStateType, DataIO::Config&>()){
            // Find out if this constructor was set to be used
            if (cell_init_from == "config") {
                // Yes. Should now check if the required config parameters were
                // also provided and add helpful error message
                if (not _cfg["cell_initial_state"]) {
                    throw std::invalid_argument("Was configured to create the "
                        "initial cell state from a config node but a node "
                        "with the key 'cell_initial_state' was not provided!");
                }

                // Everything ok. Create state object and pass it on ...
                return setup_cells(CellStateType(_cfg["cell_initial_state"]));
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


    /// Setup the neighborhood functions using config entries
    void setup_nb_funcs() {
        // Set neighborhood from config key, if available; else: empty
        if (_cfg["neighborhood"]) {
            const auto nb_cfg = _cfg["neighborhood"];

            // Extract the desired values
            if (not nb_cfg["mode"]) {
                throw std::invalid_argument("Missing key 'mode' in neighbor"
                                            "hood config! A typo perhaps?");
            }
            const auto nb_mode = as_str(nb_cfg["mode"]);

            bool compute_nb = false;
            if (nb_cfg["compute_and_store"]) {
                compute_nb = as_bool(nb_cfg["compute_and_store"]);
            }

            // And call the public interface to setup all members
            select_neighborhood(nb_mode, compute_nb);
            return;
        }        
        
        _log->debug("No neighborhood configuration given; using empty.");
        select_neighborhood(NBMode::empty);
        return;
    }
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_MANAGER_HH
