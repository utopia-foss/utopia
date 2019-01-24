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

    /// Neighborhood function used in internal interface (ID as argument)
    using NBFuncID = Neighborhoods::NBFuncID<GridType>;

    /// Neighborhood function used in public interface (with cell as argument)
    using NBFuncCell = std::function<CellContainer<Cell>(Cell&)>;


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

    /// The currently chosen neighborhood function (working on cell IDs)
    NBFuncID _nb_func_id;

    /// The currently chosen neighborhood function (working directly on cells)
    NBFuncCell _nb_func_cell;

    // TODO consider making the _nb_* members const; would simplify stuff


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
        _cell_neighbors()
    {
        // Use setup function to set up neighborhood-related members (_nb_*)
        setup_nb_funcs(model, custom_cfg);

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
        _space(model.get_space()),
        _grid(setup_grid(model, custom_cfg)),
        _cells(setup_cells(initial_state)),
        _cell_neighbors()
    {
        // Use setup function to set up neighborhood-related members (_nb_*)
        setup_nb_funcs(model, custom_cfg);

        _log->info("CellManager is all set up.");
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

    /// Return const reference to the neighborhood mode
    const std::string& nb_mode() {
        return _nb_mode;
    }


    // -- Public interface ---------------------------------------------------
    /// Retrieve the given cell's neighbors
    /** \detail The behaviour of this method is different depending on the
      *         choice of neighborhood.
      */
    CellContainer<Cell> neighbors_of(const Cell& cell) {
        return _nb_func_cell(cell);
    }


    /// Retrieve the given cell's neighbors
    /** \detail The behaviour of this method is different depending on the
      *         choice of neighborhood.
      */
    CellContainer<Cell> neighbors_of(std::shared_ptr<Cell> cell) {
        return _nb_func_cell(*cell);
    }


    /// Set the neighborhood mode
    void select_neighborhood(std::string nb_mode,
                             bool compute_and_store = false)
    {
        // TODO * Check if mode differs from the one currently set
        //      * Check if _cell_neighbors needs to be invalidated
        //      * If configured, re-calculate all cell neighbors
        //      * Set the corresponding neighbors_of method accordingly

        if (nb_mode != _nb_mode) {
            _log->info("Selecting neighborhood '{}' ...", nb_mode);

            // Retrieve the neighborhood function and store as member
            _nb_func_id = get_nb_func_id(nb_mode);

            // Adjust function object that the public interface calls
            if (nb_mode == "empty") {
                // No neighborhood set; raise exception upon call
                _nb_func_cell = _nb_unset;
            }
            else {
                // Compute the cell neighbors each time
                _nb_func_cell = _nb_compute_each_time;
            }

            // Clear the no-longer valid neighborhood relationships
            if (_cell_neighbors.size() > 0) {
                _cell_neighbors.clear();
                _log->debug("Cleared cell neighborhood cache.");
            }

            // Everything ok, now set the member variable
            _nb_mode = nb_mode;
            _log->debug("Successfully selected neighborhood '{}'.", _nb_mode);
        }
        else {
            _log->debug("Neighborhood mode already was '{}'; not changing.",
                        _nb_mode);
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
                   _nb_mode, _cells.size());

        // Clear cell neighbors container and pre-allocate space
        _cell_neighbors.clear();
        _cell_neighbors.reserve(_cells.size());

        // Compute cell neighbors and store them
        for (auto cell : _cells) {
            _cell_neighbors.push_back(neighbors_of(cell));
        }

        // Change access function to access the storage directly. Done.
        _nb_func_cell = _nb_from_cache;
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
            ret.emplace_back(std::shared_ptr<Cell>(_cells.at(id)));
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
            ret.emplace_back(std::shared_ptr<Cell>(_cells.at(id)));
        }

        return ret;
    }
    
    /// Retrieve a neighborhood function (that is working on IDs)
    /** \detail This function associates the neighborhood modes with functions
      *         in the Utopia::Neighborhoods namespace.
      */
    NBFuncID get_nb_func_id(const std::string& mode) {
        // NOTE Need to do this all manually due to use of dynamic polymorphism

        if (mode == "empty") {
            return Neighborhoods::AllAlone<GridType>;
        }
        else if (mode == "nearest") {
            if (_grid->structure() == "rectangular") {
                return Neighborhoods::Rectangular::Nearest<GridType>;
            }
            else {
                throw std::invalid_argument("No 'nearest' neighborhood "
                    "available for '" + _grid->structure() + "' grid!");
            }
        }
        // TODO add others here
        else {
            throw std::invalid_argument("No '" + mode + "' neighborhood "
                "available! Check the 'mode' argument of your neighborhood "
                "configuration.");
        }
    }
    
    // .. std::functions to call from neighbors_of ...........................
    /// Return the pre-computed neighbors of the given cell
    NBFuncCell _nb_from_cache = [this](Cell& cell) {
        return this->_cell_neighbors.at(cell.id());
    };

    /// Compute the neighbors for the given cell using the grid
    NBFuncCell _nb_compute_each_time = [this](Cell& cell) {
        // auto ids = _nb_func(cell.id(), grid());
        return this->cells_from_ids(this->_nb_func_id(cell.id(), *_grid));
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

    
    // TODO Make default constructor the default?
    // TODO Improve log messages

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
                "'cell_initialize_from' for setting up cells via a "
                "DataIO::Config& constructor or default constructor.");
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
    void setup_nb_funcs(Model& model, const DataIO::Config& custom_cfg) {
        // Determine which configuration to use
        auto cfg = model.get_cfg();

        if (custom_cfg.size() > 0) {
            _log->debug("Using custom config for neighborhood setup ...");
            cfg = custom_cfg;
        }
        else {
            _log->debug("Using '{}' model config for neighborhood setup ...",
                        model.get_name());
        }

        // Set neighborhood from config key, if available; else: empty
        if (cfg["neighborhood"]) {
            // Extract the desired values
            if (!cfg["neighborhood"]["mode"]) {
                throw std::invalid_argument("Missing key 'mode' in neighbor"
                                            "hood config! A typo perhaps?");
            }
            const auto nb_mode = as_str(cfg["neighborhood"]["mode"]);

            bool compute_nb = false;
            if (cfg["neighborhood"]["compute_and_store"]) {
                compute_nb = as_bool(cfg["neighborhood"]["compute_and_store"]);
            }

            // And call the public interface to setup all members
            select_neighborhood(nb_mode, compute_nb);
            return;
        }        
        
        _log->debug("No neighborhood configuration given; using empty.");
        select_neighborhood("empty");
        return;
    }
};


// end group CellManager
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_MANAGER_HH
