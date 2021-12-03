#ifndef UTOPIA_CORE_CELL_MANAGER_HH
#define UTOPIA_CORE_CELL_MANAGER_HH

#include <algorithm>
#include <type_traits>
#include <string_view>
#include <unordered_set>

#include <armadillo>

#include "logging.hh"
#include "types.hh"
#include "exceptions.hh"
#include "cell.hh"
#include "grids.hh"
#include "apply.hh"
#include "select.hh"


namespace Utopia {
/**
 *  \addtogroup CellManager
 *  \{
 */

/// Manages a physical space, its grid discretization, and cells on that grid
/** This class implements a common interface for working with cells as a
 *  representation of volumes of physical space. A typical use case is the
 *  cellular automaton.
 *
 *  To that end, a discretization of space is needed: the grid. This spatial
 *  discretization is handled by concrete classes derived from Utopia::Grid.
 *  The CellManager communicates with these objects solely via cell indices
 *  (which are the indices within the container of cells), making the grid
 *  implementation independent of the type of cells used.
 *
 *  \tparam CellTraits  Type traits of the cells used
 *  \tparam Model       Type of the model using this manager
 */
template<class CellTraits, class Model>
class CellManager {
public:
    /// The type of this CellManager
    using Self = CellManager<CellTraits, Model>;

    /// Type of the managed cells
    using Cell = Utopia::Cell<CellTraits>;

    /// Alias for entity type; part of the shared interface of entity managers
    using Entity = Cell;

    /// Type of the cell state
    using CellState = typename CellTraits::State;

    /// The space type this cell manager maps to
    using Space = typename Model::Space;

    /// Dimensionality of the space this cell manager is to discretize
    static constexpr DimType dim = Model::Space::dim;

    /// Grid type; this refers to the base type of the stored (derived) object
    using GridType = Grid<Space>;

    /// Type of vectors that represent a physical quantity
    using SpaceVec = SpaceVecType<dim>;

    /// Type of multi-index like arrays
    using MultiIndex = typename GridType::MultiIndex;

    /// Random number generator type
    using RNG = typename Model::RNG;

    /// Configuration node type
    using Config = typename Model::Config;

    /// Neighborhood function used in public interface (with cell as argument)
    using NBFuncCell = std::function<CellContainer<Cell>(const Cell&)>;

    /// The type of a rule function acting on cells of this cell manager
    /** This is a convenience type def that models can use to easily have this
      * type available.
      */
    using RuleFunc =
        typename std::function<CellState(const std::shared_ptr<Cell>&)>;

    /// The type of a void rule function acting on cells of this cell manager
    /** This is a convenience type def that models can use to easily have this
      * type available.
      * Void rule functions are only possible for asynchronous updates.
      */
    using VoidRuleFunc =
        typename std::function<void(const std::shared_ptr<Cell>&)>;


private:
    // -- Members -------------------------------------------------------------
    /// The logger (same as the model this manager resides in)
    const std::shared_ptr<spdlog::logger> _log;

    /// Cell manager configuration node
    const Config _cfg;

    /// The model's random number generator, used e.g. for cell construction
    const std::shared_ptr<RNG> _rng;

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
    /// Whether a warning about an empty neighbourhood was already emitted
    /** This is to prevent a gigantic amount of warnings being logged
      */
    bool _empty_nb_warning_emitted = false;


    // -- Constructors --------------------------------------------------------
    /// Construct a cell manager
    /** With the model available, the CellManager can extract the required
      * information from the model without the need to pass it explicitly.
      * Furthermore, this constructor differs to the one with the
      * ``initial_state`` parameter such that the way the initial state of the
      * cells is determined can be controlled via the configuration.
      *
      * \param  model       The model this CellManager belongs to
      * \param  custom_cfg  A custom config node to use to use for grid and
      *                     cell setup. If not given, the model's configuration
      *                     is used to extract the required entries.
      */
    CellManager (const Model& model,
                 const Config& custom_cfg = {})
    :
        _log(model.get_logger()),
        _cfg(setup_cfg(model, custom_cfg)),
        _rng(model.get_rng()),
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
    CellManager (const Model& model,
                 const CellState initial_state,
                 const Config& custom_cfg = {})
    :
        _log(model.get_logger()),
        _cfg(setup_cfg(model, custom_cfg)),
        _rng(model.get_rng()),
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
    /// Return the CellManager logger
    const auto& log () const {
        return _log;
    }

    /// Return the configuration used for building this cell manager
    const Config& cfg () const {
        return _cfg;
    }

    /// Return a reference to the shared random number generator
    const std::shared_ptr<RNG>& rng () const {
        return _rng;
    }

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

    /// Return const reference to the entities managed by this manager: cells
    const CellContainer<Cell>& entities () const {
        return cells();
    }


    // -- Public interface ----------------------------------------------------
    // .. Position-related ....................................................

    /// Returns the multi-index of the given cell
    /** \note Consult the documentation of the selected grid discretization to
      *       learn about the interpretation of the returned values.
      */
    MultiIndex midx_of(const Cell& cell) const {
        return _grid->midx_of(cell.id());
    }

    /// Returns the multi-index of the given cell
    /** \note Consult the documentation of the selected grid discretization to
      *       learn about the interpretation of the returned values.
      */
    MultiIndex midx_of(const std::shared_ptr<Cell>& cell) const {
        return _grid->midx_of(cell->id());
    }

    /// Returns the barycenter of the given cell
    SpaceVec barycenter_of(const Cell& cell) const {
        return _grid->barycenter_of(cell.id());
    }

    /// Returns the barycenter of the given cell
    SpaceVec barycenter_of(const std::shared_ptr<Cell>& cell) const {
        return _grid->barycenter_of(cell->id());
    }

    /// Returns the physical extent of the given cell
    SpaceVec extent_of(const Cell& cell) const {
        return _grid->extent_of(cell.id());
    }

    /// Returns the physical extent of the given cell
    SpaceVec extent_of(const std::shared_ptr<Cell>& cell) const {
        return _grid->extent_of(cell->id());
    }

    /// Returns a container of vertices of the given cell
    /** The vertices are the absolute coordinates that define the cell. For
      * example, a 2D square cell is the surface of a polygon defined by four
      * points.
      *
      * \note   Consult the documentation of the selected grid discretization
      *         to learn about the order of the returned values.
      */
    std::vector<SpaceVec> vertices_of(const Cell& cell) const {
        return _grid->vertices_of(cell.id());
    }

    /// Returns a container of vertices of the given cell
    /** \note Consult the documentation of the selected grid discretization to
      *       learn about the order of the returned values.
      */
    std::vector<SpaceVec> vertices_of(const std::shared_ptr<Cell>& cell) const
    {
        return _grid->vertices_of(cell->id());
    }

    /// Return the cell covering the given point in physical space
    /** Cells are interpreted as covering half-open intervals in space, i.e.,
      * including their low-value edges and excluding their high-value edges.
      * The special case of points on high-value edges for non-periodic space
      * behaves such that these points are associated with the cells at the
      * boundary.
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

    // .. Cell Selection ......................................................
    /// Retrieve a container of cells that are at a specified boundary
    /** Lets the grid compute the set of cell IDs at the boundary and then
      * converts them into pointers to cells. As the set is sorted by cell
      * IDs, the returned container is also sorted.
      *
      * \warning For a periodic space, an empty container is returned; no error
      *          or warning is emitted!
      *
      * \param
      * \parblock  Select which boundary to return the cell IDs of. If 'all',
      *            all boundary cells are returned. Other available values
      *            depend on the dimensionality of the grid:
      *                * 1D:  left, right
      *                * 2D:  bottom, top
      *                * 3D:  back, front
      */
    CellContainer<Cell> boundary_cells(const std::string& select="all") const {
        if (_space->periodic) {
            _log->warn("Selecting boundary cells (mode '{}') of a periodic "
                       "space will always return an empty container!", select);
        }

        return entity_pointers_from_ids(_grid->boundary_cells(select));
    }

    /// Select cells using the \ref Utopia::select_entities interface
    /** Returns a container of cells that were selected according to a certain
      * selection mode. This is done via the \ref Utopia::select_entities
      * interface.
      *
      * \tparam  mode    The selection mode
      *
      * \args    args    Forwarded to \ref Utopia::select_entities
      */
    template<SelectionMode mode, class... Args>
    CellContainer<Cell> select_cells(Args&&... args) const {
        return select_entities<mode>(*this, std::forward<Args>(args)...);
    }

    /// Select entities according to parameters specified in a configuration
    /** Via the ``mode`` key, one of the selection modes can be chosen; for
      * available oens, see \ref Utopia::SelectionMode.
      *
      * Depending on that mode, the other parameters are extracted from the
      * configuration. See \ref Utopia::select_entities for more info.
      *
      * \param  sel_cfg  The configuration node containing the expected
      *                  key-value pairs specifying the selection.
      */
    CellContainer<Cell> select_cells(const Config& sel_cfg) const {
        return select_entities(*this, sel_cfg);
    }


    // .. Setting the cell states .............................................

    /// Set all cell states using information from a HDF5 file
    /** Using armadillo functionality, this loads the data from the given HDF5
      * file into an array of ``ElementT``. It then iterates over all
      * cells, looks up the multi index, and passes it to the ``setter_func``,
      * which has the responsibility of setting the cell state accordingly.
      *
      * The ``setter_func`` should accept as first argument a reference to the
      * cell (more accurately: ``const std::shared_ptr<Cell>&``) and as second
      * argument the value from the loaded dataset.
      *
      * \note Currently only supports 2D grids and cells with Update::manual.
      *
      * \tparam ElemenT     The type that armadillo should load the data as.
      *                     Need be supported by armadillo.
      * \tparam SetterFunc  Type of the setter function, e.g. std::function<…>
      *
      * \param hdf5_file    The hdf5 file to load the data from. Can be an
      *                     absolute path or relative to the working directory.
      * \param dset_path    The path within the HDF5 file that points to the
      *                     dataset to load data from.
      * \param setter_func  The ``void(cell, value)`` function that sets the
      *                     cell state given the value from the loaded dataset
      *                     corresponding to the cells' multi-index.
      */
    template<class ElementT=double, class SetterFunc>
    void set_cell_states(const std::string& hdf5_file,
                         const std::string& dset_path,
                         const SetterFunc& setter_func)
    {
        static_assert(dim == 2, "Loading cell state is only supported in 2D!");
        static_assert(CellTraits::mode == Update::manual,
                      "Setting cell states for cell update modes other than "
                      "Update::manual is currently not supported.");
        // TODO Once passing of additional arguments to rule functions becomes
        //      possible, remove this restriction and use apply_rule below!

        _log->debug("Setting cell states using HDF5 data ...");
        _log->debug("  File:          {}", hdf5_file);
        _log->debug("  Dataset path:  {}", dset_path);

        // Load the array data
        arma::Mat<ElementT> data;
        data.load(arma::hdf5_name(hdf5_file, dset_path,
                                  arma::hdf5_opts::trans));
        // NOTE Need to load transposed; otherwise the index-access below would
        //      be reversed. This is to account for the different ordering
        //      between armadillo and CellManager coordinates.
        //      Thus: n_rows corresponds to N_x, n_cols to N_y!

        // Check that data was loaded
        if (not data.size()) {
            throw std::runtime_error("Failed loading HDF5 data! Is the file "
                                     "used by another program?");
        }

        // Check against grid shape
        const auto grid_shape = _grid->shape();
        if (data.n_rows != grid_shape[0] or data.n_cols != grid_shape[1]) {
            throw std::invalid_argument("Shape mismatch between loaded data ("
                + std::to_string(data.n_rows) + ", "
                + std::to_string(data.n_cols) + ") and grid ("
                + std::to_string(grid_shape[0]) + ", "
                + std::to_string(grid_shape[1]) + ")!");
        }

        // Go over all cells, extract multi index, and let the setter function
        // set the cell state.
        for (const auto& cell : _cells) {
            const auto midx = this->midx_of(cell);
            setter_func(cell, data(midx[0], midx[1]));
        }

        _log->debug("Cell states set successfully.");
    }

    // .. Neighborhood-related ................................................

    /// Return the currently selected neighborhood mode
    /** \note This is a shortcut that accesses the value set in the grid.
      */
    const NBMode& nb_mode () const {
        return _grid->nb_mode();
    }

    /// Return the (maximum) size of the currently selected neighborhood
    /** \note This is a shortcut that accesses the value computed in the grid.
      */
    auto nb_size () const {
        return _grid->nb_size();
    }

    /// Retrieve the given cell's neighbors
    /** The behaviour of this method is different depending on the choice of
      * neighborhood.
      */
    CellContainer<Cell> neighbors_of(const Cell& cell) const {
        return _nb_func(cell);
    }

    /// Retrieve the given cell's neighbors
    /** The behaviour of this method is different depending on the choice of
      * neighborhood.
      */
    CellContainer<Cell> neighbors_of(const std::shared_ptr<Cell>& cell) const {
        return _nb_func(*cell);
    }

    /// Select the neighborhood and all parameters fully from a config node
    /** If this method is used to set up the neighborhood, the following keys
      * will be read and parsed:
      *
      *     - ``mode`` (required): the neighborhood mode
      *     - ``compute_and_store`` (optional, default: ``true``): whether to
      *       directly compute all neighbors and henceforth use the buffer to
      *       get these neighbors.
      *
      * Subsequently, these values are passed to the appropriate overload
      * of the select_neighborhood function.
      *
      * \param nb_cfg   The neighborhood configuration node. Apart from the
      *                 keys that are parsed here, the full config node is
      *                 passed along further.
      */
    void select_neighborhood(const Config& nb_cfg) {
        // Extract the desired values
        if (not nb_cfg["mode"]) {
            throw KeyError("mode", nb_cfg, "Could not select neighborhood!");
        }
        const auto nb_mode = get_as<std::string>("mode", nb_cfg);
        const bool compute = get_as<bool>("compute_and_store", nb_cfg, true);

        // Call the string-based selection function, passing through the whole
        // config node. The fact that the above two keys are also present in
        // the node is not a problem.
        select_neighborhood(nb_mode, compute, nb_cfg);
    }

    /// Select the neighborhood mode using a string for the mode argument
    /** Resolves a string-type ``nb_mode`` to NBmode, then invokes the
      * appropriate overload.
      *
      * \param nb_mode            The name of the neighborhood to select
      * \param compute_and_store  Whether to directly compute all neighbors
      *                           and henceforth use the buffer to get these
      *                           neighbors. Default: ``true``.
      * \param nb_params          Passed on to the Grid::select_neighborhood
      *                           method, carrying additional parameters.
      */
    void select_neighborhood(const std::string& nb_mode,
                             const bool compute_and_store = true,
                             const Config& nb_params = {})
    {
        // Check if the string is valid
        if (not nb_mode_map.count(nb_mode)) {
            throw std::invalid_argument("Got unexpected neighborhood mode '"
                + nb_mode + "'! Available modes: empty, vonNeumann, Moore, "
                "hexagonal.");
        }

        // Translate string; pass all other arguments through
        select_neighborhood(nb_mode_map.at(nb_mode),
                            compute_and_store, nb_params);
    }

    /// Set the neighborhood mode
    /** \param nb_mode            The name of the neighborhood to select
      * \param compute_and_store  Whether to directly compute all neighbors
      *                           and henceforth use the buffer to get these
      *                           neighbors. Default: ``true``.
      * \param nb_params          Passed on to the Grid::select_neighborhood
      *                           method, carrying additional parameters.
      */
    void select_neighborhood(const NBMode& nb_mode,
                             const bool compute_and_store = true,
                             const Config& nb_params = {})
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

            _log->debug("Successfully selected '{}' neighborhood (size: {}).",
                        nb_mode_to_string(_grid->nb_mode()), nb_size());
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
    /** After this function was called, the cell neighbors will be returned
      * from the storage container rather than re-calculated for every access.
      */
    void compute_cell_neighbors() {
        _log->info("Computing and storing '{}' neighbors of all {} cells ...",
                   nb_mode_to_string(_grid->nb_mode()), _cells.size());

        // Clear cell neighbors container and pre-allocate space
        _cell_neighbors.clear();
        _cell_neighbors.reserve(_cells.size());

        // Compute cell neighbors and store them
        for (const auto& cell : _cells) {
            _cell_neighbors.push_back(neighbors_of(cell));
        }

        // Change access function to access the storage directly. Done.
        _nb_func = _nb_from_cache;
        _log->info("Computed and stored cell neighbors.");
    }


    // -- Public Helpers ------------------------------------------------------

    /// Given a container IDs, convert it to container of entity pointers
    /** \TODO This should move into a common manager base class
      */
    template<class IndexContainer>
    CellContainer<Cell> entity_pointers_from_ids(IndexContainer&& ids) const {
        CellContainer<Cell> ret;
        ret.reserve(ids.size());

        for (const auto& id : ids) {
            ret.emplace_back(std::shared_ptr<Cell>(_cells[id]));
        }

        return ret;
    }

private:
    // -- Private Helpers -----------------------------------------------------
    // ...


    // -- std::functions to call from neighbors_of ----------------------------

    /// Return the pre-computed neighbors of the given cell
    NBFuncCell _nb_from_cache = [this](const Cell& cell) {
        return this->_cell_neighbors[cell.id()];
    };

    /// Compute the neighbors for the given cell using the grid
    NBFuncCell _nb_compute_each_time = [this](const Cell& cell) {
        return
            this->entity_pointers_from_ids(
                this->_grid->neighbors_of(cell.id()));
    };

    /// Compute the neighbors for the given cell using the grid
    NBFuncCell _nb_compute_each_time_empty = [this](const Cell& cell) {
        if (not this->_empty_nb_warning_emitted) {
            this->_log->warn("No neighborhood selected! Calls to the "
                "CellManager::neighbors_of method will always return an empty "
                "container. There will be no further warning.");
            this->_empty_nb_warning_emitted = true;
        }
        return
            this->entity_pointers_from_ids(
                this->_grid->neighbors_of(cell.id()));
    };


    // -- Setup functions -----------------------------------------------------

    /// Set up the cell manager configuration member
    /** This function determines whether to use a custom configuration or the
      * one provided by the model this CellManager belongs to
      */
    Config setup_cfg(const Model& model, const Config& custom_cfg) const {
        Config cfg;

        if (custom_cfg.size() > 0) {
            _log->debug("Using custom config for cell manager setup ...");
            cfg = custom_cfg;
        }
        else {
            _log->debug("Using '{}' model's configuration for cell manager "
                        "setup ... ", model.get_name());

            if (not model.get_cfg()["cell_manager"]) {
                throw std::invalid_argument("Missing config entry "
                    "'cell_manager' in model configuration! Either specify "
                    "that key or pass a custom configuration node to the "
                    "CellManager constructor.");
            }
            cfg = model.get_cfg()["cell_manager"];
        }
        return cfg;
    }

    /// Set up the grid discretization
    std::shared_ptr<GridType> setup_grid() const {
        // Check if the required parameter nodes are available
        // TODO Throw KeyErrors here instead!
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
        auto structure = get_as<std::string>("structure", _cfg["grid"]);

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

    /// Set up the cells container using an explicitly passed initial state
    CellContainer<Cell> setup_cells(const CellState& initial_state) const {
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

    /// Set up cells container via config or default constructor
    /** If no explicit initial state is given, this setup function is called.
      * There are three modes: If the \ref CellTraits are set such that the
      * default constructor of the cell state is to be used, that constructor
      * is required and is called for each cell.
      *
      * Otherwise, the CellState needs to be constructible via a
      * ``const Config&`` argument, which gets passed the config entry
      * ``cell_params`` from the CellManager's configuration. If a constructor
      * with the signature
      * ``(const Config&, const std::shared_ptr<RNG>&)`` is supported,
      * that constructor is called instead.
      *
      * \note   If the constructor for the cell state has an RNG available
      *         it is called anew for _each_ cell; otherwise, an initial state
      *         is constructed _once_ and used for all cells.
      */
    CellContainer<Cell> setup_cells() const {
        // Distinguish depending on constructor.
        // Is the default constructor to be used?
        if constexpr (CellTraits::use_default_state_constructor) {
            static_assert(
                std::is_default_constructible<CellState>(),
                "CellTraits were configured to use the default constructor to "
                "create cell states, but the CellState is not "
                "default-constructible! Either implement such a constructor, "
                "unset the flag in the CellTraits, or pass an explicit "
                "initial cell state to the CellManager."
            );

            _log->info("Setting up cells using default constructor ...");

            // Create the initial state (same for all cells)
            return setup_cells(CellState());
        }

        // Is there a constructor available that allows passing the RNG?
        else if constexpr (std::is_constructible<CellState,
                                                 const Config&,
                                                 const std::shared_ptr<RNG>&
                                                 >())
        {
            _log->info("Setting up cells using config constructor (with RNG) "
                       "...");

            // Extract the configuration parameter
            if (not _cfg["cell_params"]) {
                throw std::invalid_argument("CellManager is missing the "
                    "configuration entry 'cell_params' to set up the cells' "
                    "initial states!");
            }
            const auto cell_params = _cfg["cell_params"];

            // The cell container to be populated
            CellContainer<Cell> cont;

            // Populate the container, creating the cell state anew each time
            for (IndexType i=0; i<_grid->num_cells(); i++) {
                cont.emplace_back(
                    std::make_shared<Cell>(i, CellState(cell_params, _rng))
                );
            }
            // Done. Shrink it.
            cont.shrink_to_fit();
            _log->info("Populated cell container with {:d} cells.",
                       cont.size());
            return cont;
        }

        // As default, require a Config constructor
        else {
            static_assert(
                std::is_constructible<CellState, const Config&>(),
                "CellManager::CellState needs to be constructible using "
                "const Config& as only argument. Either implement "
                "such a constructor, pass an explicit initial cell state to "
                "the CellManager, or set the CellTraits such that a default "
                "constructor is to be used."
            );

            _log->info("Setting up cells using config constructor ...");

            // Extract the configuration parameter
            if (not _cfg["cell_params"]) {
                throw std::invalid_argument("CellManager is missing the "
                    "configuration entry 'cell_params' to set up the cells' "
                    "initial states!");
            }

            // Create the initial state (same for all cells)
            return setup_cells(CellState(_cfg["cell_params"]));
        }
        // This point is never reached.
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

#endif // UTOPIA_CORE_CELL_MANAGER_HH
