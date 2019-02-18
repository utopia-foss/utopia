#ifndef UTOPIA_CORE_MANAGER_HH
#define UTOPIA_CORE_MANAGER_HH

#include <type_traits>

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
    static constexpr DimType dim = Model::Space::dim;

    /// Grid type; this refers to the base type of the stored (derived) object
    using GridType = Grid<Space>;

    /// Neighborhood function used in public interface (with cell as argument)
    using NBFuncCell = std::function<CellContainer<Cell>(const Cell&)>;

    /// Type of vectors that represent a physical quantity
    using SpaceVec = SpaceVecType<dim>;


private:
    // -- Members -------------------------------------------------------------
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

    /// The currently chosen neighborhood function (working directly on cells)
    NBFuncCell _nb_func;


public:
    // -- Constructors --------------------------------------------------------
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
        _cell_neighbors()
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
        _cell_neighbors()
    {
        // Set default value for _nb_func
        _nb_func = _nb_compute_each_time_empty;

        // Use setup function to set up neighborhood from configuration
        setup_nb_funcs();

        _log->info("CellManager is all set up.");
    }


    /// -- Getters ------------------------------------------------------------
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


    // -- Public interface ----------------------------------------------------
    // .. Position-related ....................................................

    /// Returns the multi-index of the given cell
    /** \note Consult the documentation of the selected grid discretization to
      *       learn about the interpretation of the returned values.
      */
    auto midx_of(const Cell& cell) const {
        return _grid->midx_of(cell.id());
    }
    
    /// Returns the multi-index of the given cell
    /** \note Consult the documentation of the selected grid discretization to
      *       learn about the interpretation of the returned values.
      */
    auto midx_of(const std::shared_ptr<Cell>& cell) const {
        return _grid->midx_of(cell->id());
    }

    /// Returns the barycenter of the given cell
    auto barycenter_of(const Cell& cell) const {
        return _grid->barycenter_of(cell.id());
    }
    
    /// Returns the barycenter of the given cell
    auto barycenter_of(const std::shared_ptr<Cell>& cell) const {
        return _grid->barycenter_of(cell->id());
    }

    /// Returns the physical extent of the given cell
    auto extent_of(const Cell& cell) const {
        return _grid->extent_of(cell.id());
    }
    
    /// Returns the physical extent of the given cell
    auto extent_of(const std::shared_ptr<Cell>& cell) const {
        return _grid->extent_of(cell->id());
    }

    /// Returns a container of vertices of the given cell
    /** \note Consult the documentation of the selected grid discretization to
      *       learn about the order of the returned values.
      */
    auto vertices_of(const Cell& cell) const {
        return _grid->vertices_of(cell.id());
    }
    
    /// Returns a container of vertices of the given cell
    /** \note Consult the documentation of the selected grid discretization to
      *       learn about the order of the returned values.
      */
    auto vertices_of(const std::shared_ptr<Cell>& cell) const {
        return _grid->vertices_of(cell->id());
    }

    /// Return the cell covering the given point in physical space
    /** \detail Cells are interpreted as covering half-open intervals in space,
      *         i.e., including their low-value edges and excluding their high-
      *         value edges.
      *         The special case of points on high-value edges for non-periodic
      *         space behaves such that these points are associated with the
      *         cells at the boundary.
      *
      * \note   For non-periodic space, a check is performed whether the given
      *         point is inside the physical space associated with the grid. If
      *         that is not the case, an error is raised.
      *         For periodic space, the given position is mapped back into the
      *         physical space, thus always returning a valid cell.
      */
    const std::shared_ptr<Cell>& cell_at(const SpaceVec& pos) const {
        return _cells[_grid->cell_at(pos)];
    }

    /// Retrieve a container of cells that are at a specified boundary
    /** \note   For a periodic space, an empty container is returned; no error
      *         or warning is emitted!
      *
      * \detail Lets the grid compute the set of cell IDs at the boundary and
      *         then converts them into pointers to cells. As the set is sorted
      *         by cell IDs, the returned container is also sorted.
      *
      * \param  Select which boundary to return the cell IDs of. If 'all',
      *         all boundary cells are returned. Other available values depend
      *         on the dimensionality of the grid:
      *                1D:  left, right
      *                2D:  bottom, top
      *                3D:  back, front
      */
    CellContainer<Cell> boundary_cells(std::string select="all") const {
        return cells_from_ids(_grid->boundary_cells(select));
    }


    // .. Neighborhood-related ................................................
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

    /// Select the neighborhood and all parameters fully from a config node
    void select_neighborhood(const DataIO::Config& nb_cfg) {
        // Extract the desired values
        if (not nb_cfg["mode"]) {
            throw std::invalid_argument("Missing key 'mode' in neighborhood "
                "configuration! Perhaps a typo in 'neighborhood'?");
        }
        const auto nb_mode = as_str(nb_cfg["mode"]);

        bool compute_nb = false;
        if (nb_cfg["compute_and_store"]) {
            compute_nb = as_bool(nb_cfg["compute_and_store"]);
        }

        // Call the string-based selection function, passing through the whole
        // config node. The fact that the above two keys are also present in
        // the node is not a problem.
        select_neighborhood(nb_mode, compute_nb, nb_cfg);
    }

    /// Select the neighborhood mode using a string for the mode argument
    void select_neighborhood(const std::string nb_mode,
                             const bool compute_and_store = false,
                             const DataIO::Config& nb_params = {})
    {
        // Check if the string is valid
        if (not nb_mode_map.count(nb_mode)) {
            throw std::invalid_argument("Could not translate given value for "
                "neighborhood mode ('" + nb_mode + "') to valid enum entry!");
        }

        // Translate string; pass all other arguments through
        select_neighborhood(nb_mode_map.at(nb_mode),
                            compute_and_store, nb_params);
    }

    /// Set the neighborhood mode
    /** \param nb_mode            The name of the neighborhood to select
      * \param compute_and_store  Whether to directly compute all neighbors
      *                           and henceforth use the buffer to get these
      *                           neighbors.
      */
    void select_neighborhood(const NBMode nb_mode,
                             const bool compute_and_store = false,
                             const DataIO::Config& nb_params = {})
    {
        // Only change the neighborhood if it is different to the one already
        // set in the grid or if it is set to be empty
        if ((nb_mode != _grid->nb_mode()) or (nb_mode == NBMode::empty)) {
            _log->info("Selecting '{}' neighborhood ...",
                       nb_mode_to_string(nb_mode));

            // Tell the grid which mode to use
            _grid->select_neighborhood(nb_mode, nb_params);

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

            _log->debug("Successfully selected '{}' neighborhood.",
                        nb_mode_to_string(_grid->nb_mode()));
        }
        else {
            _log->debug("Neighborhood was already set to '{}'; not changing.",
                        nb_mode_to_string(_grid->nb_mode()));
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
                   nb_mode_to_string(_grid->nb_mode()), _cells.size());

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

    /// Return the currently selected neighborhood mode
    /** \note This is a shortcut that accesses the value set in the grid.
      */
    const NBMode& nb_mode () const {
        return _grid->nb_mode();
    }


private:
    // -- Helper functions ----------------------------------------------------
    // ...


    // -- Helpers for Neighbors interface -------------------------------------

    /// Given a container of cell IDs, convert it to container of cell pointers
    template<class IndexContainer>
    CellContainer<Cell> cells_from_ids(IndexContainer&& ids) const {
        // Initialize container to be returned and fix it in size
        CellContainer<Cell> ret;
        ret.reserve(ids.size());

        for (const auto& id : ids) {
            ret.emplace_back(std::shared_ptr<Cell>(_cells[id]));
        }

        return ret;
    }

    
    // .. std::functions to call from neighbors_of ............................
    
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


    // -- Setup functions -----------------------------------------------------

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
        else if (not _cfg["grid"]["structure"]) {
            throw std::invalid_argument("Missing required grid configuration "
                                        "entry 'structure'!");
        }
        
        // Get the structure parameter
        auto structure = as_str(_cfg["grid"]["structure"]);

        _log->info("Setting up grid discretization with '{}' cells ...",
                   structure);
        
        // Create the respective grids, distinguishing by structure
        if (structure == "triangular") {
            using GridSpec = TriangularGrid<Space>;
            return std::make_shared<GridSpec>(_space, _cfg["grid"]);
        }
        else if (structure == "square") {
            using GridSpec = SquareGrid<Space>;
            return std::make_shared<GridSpec>(_space, _cfg["grid"]);
        }
        else if (structure == "hexagonal") {
            using GridSpec = HexagonalGrid<Space>;
            return std::make_shared<GridSpec>(_space, _cfg["grid"]);
        }
        else {
            throw std::invalid_argument("Invalid value for grid "
                "'structure' argument: '" + structure + "'! Allowed "
                "values: 'square', 'hexagonal', 'triangular'");
        }
    }


    /// Set up the cells container
    CellContainer<Cell> setup_cells(const CellStateType initial_state) {
        CellContainer<Cell> cont;

        // Construct all the cells
        for (IndexType i=0; i<_grid->num_cells(); i++) {
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
      *         are not possible or the configuration was invalid, a run time
      *         error message is emitted.
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
        // If there is a neighborhood key, use it to set up the neighborhood
        if (_cfg["neighborhood"]) {
            _log->debug("Setting up neighborhood from config entry ...");
            select_neighborhood(_cfg["neighborhood"]);
            return;
        }
        // else: Use empty.
        _log->debug("No neighborhood configuration given; using empty.");
        select_neighborhood(NBMode::empty);
    }
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_MANAGER_HH
