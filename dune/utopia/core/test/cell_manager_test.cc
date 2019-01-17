#include <cassert>
#include <iostream>

#include <dune/utopia/core/cell_manager.hh>
#include <dune/utopia/core/logging.hh>
#include <dune/utopia/data_io/cfg_utils.hh>

// Import some types
using Utopia::DataIO::Config;
using Utopia::CellManager;


/// A cell state definition that is default-constructible
struct CellStateDC {
    double a_double;
    std::string a_string;
    bool a_bool;
};

/// A cell state definition that is config-constructible
struct CellStateCC {
    double a_double;
    std::string a_string;
    bool a_bool;

    CellStateCC(const Config& cfg)
    :
        a_double(Utopia::as_double(cfg["a_double"])),
        a_string(Utopia::as_str(cfg["a_string"])),
        a_bool(Utopia::as_bool(cfg["a_bool"]))
    {}
};

/// A cell state definition that is only explicitly constructible
struct CellStateEC {
    double a_double;
    std::string a_string;
    bool a_bool;

    CellStateEC(double d, std::string s, bool b)
    :
        a_double(d),
        a_string(s),
        a_bool(b)
    {}
};


// Create some cell traits definitions, i.e. bundling the cell traits together
// The second and third template parameters for sync and tags, respectively,
// are optional.
/// For a default-constructible cell state
using CellTraitsDC = Utopia::CellTraits<CellStateDC>;

/// For a config-constructible cell state
using CellTraitsCC = Utopia::CellTraits<CellStateCC>;

/// For an explicitly-constructible cell state
using CellTraitsEC = Utopia::CellTraits<CellStateEC>;



/// A mock model class to hold the cell manager
template<class CellTraits>
class MockModel {
public:
    // -- Members -- //
    using Space = Utopia::DefaultSpace;
    using CellStateType = typename CellTraits::State;

    const Config _cfg;
    std::shared_ptr<spdlog::logger> _log;

    const Space _space;

    CellManager<CellTraits, MockModel> _cm;


    // -- Constructors -- //
    /// Basic constructor
    MockModel(const std::string model_name, const Config& cfg)
    :
        _cfg(cfg),
        _log(setup_logger(model_name)),
        _space(setup_space()),
        _cm(*this)
    {}

    /// Constructor with initial cell state
    MockModel(const std::string model_name, const Config& cfg,
              const CellStateType cell_initial_state)
    :
        _cfg(cfg),
        _log(setup_logger(model_name)),
        _space(setup_space()),
        _cm(*this, cell_initial_state)
    {}


    // -- Other functions -- //

    std::shared_ptr<spdlog::logger> setup_logger(const std::string name) {
        auto logger = spdlog::get(name);
    
        if (not logger) {
            // Create it; spdlog throws an exception if it already exists
            logger = spdlog::stdout_color_mt(name);
        }
        logger->set_level(spdlog::level::debug);
        logger->set_pattern("%n  %^%l%$  %v");
        return logger;
    }


    Space setup_space() const {
        if (_cfg["space"]) {
            // Build a space with the given parameters
            return Space(_cfg["space"]);
        }
        else {
            // Use the default space
            return Space();
        }
    }

    /// Return a mock logger
    std::shared_ptr<spdlog::logger> get_logger() {
        return _log;
    }

    /// Return the space this model resides in
    std::shared_ptr<Space> get_space() const {
        return std::make_shared<Space>(_space);
    }

    /// Return the config node of this model
    Config get_cfg() const {
        return _cfg;
    }
};


template<typename err_t>
bool check_error_message(std::string desc,
                         std::function<void()> func,
                         std::string to_find) {
    std::cout << "Checking exceptions for case:  " << desc << std::endl;
    try {
        func();

        std::cerr << "Did not throw!" << std::endl;
        return false;
    }
    catch (err_t& e) {
        if (((std::string) e.what()).find(to_find) == -1) {
            std::cerr << "Did not throw expected error message!" << std::endl;
            std::cerr << "  Expected to find:  " << to_find << std::endl;
            std::cerr << "  But got         :  " << e.what() << std::endl;

            return false;
        }
    }
    catch (...) {
        std::cerr << "Threw error of unexpected type!" << std::endl;
        throw;
    }
    std::cout << "... success."
              << std::endl << std::endl;
    return true;
}



// ----------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    try {
        Dune::MPIHelper::instance(argc,argv);

        std::cout << "Getting config file ..." << std::endl;
        auto cfg = YAML::LoadFile("cell_manager_test.yml");
        std::cout << "Success." << std::endl << std::endl;


        std::cout << "------ Testing mock model initialization via ... ------"
                  << std::endl;
        
        // Initialize the mock model with default-constructible cell type
        std::cout << "... default-constructible state" << std::endl;
        MockModel<CellTraitsDC> mm_dc("mm_dc", cfg["default"]);
        std::cout << "Success." << std::endl << std::endl;
        
        // Initialize the mock model with config-constructible cell type
        std::cout << "... DataIO::Config-constructible state" << std::endl;
        MockModel<CellTraitsCC> mm_cc("mm_cc", cfg["config"]);
        std::cout << "Success." << std::endl << std::endl;
        
        // Initialize the mock model with config-constructible cell type
        std::cout << "... only explicitly constructible state" << std::endl;
        const auto initial_state = CellStateEC(2.34, "foobar", true);
        MockModel<CellTraitsEC> mm_ec("mm_ec", cfg["explicit"],
                                      initial_state);
        std::cout << "Success." << std::endl << std::endl;


        // TODO Test passing of custom config


        std::cout << "------ Testing init. of discretizations ... ------"
                  << std::endl;
        MockModel<CellTraitsDC> mm_dc_rect("mm_dc_rect", cfg["default_rect"]);
        MockModel<CellTraitsDC> mm_dc_hex("mm_dc_hex", cfg["default_hex"]);
        MockModel<CellTraitsDC> mm_dc_tri("mm_dc_tri", cfg["default_tri"]);

        std::cout << "Success." << std::endl << std::endl;


        std::cout << "------ Testing member access ... ------" << std::endl;
        // Check that parameters were passed correctly
        auto cm = mm_ec._cm;  // All mm_* should have the same parameters.

        auto space = cm.space();  // shared pointer
        auto grid = cm.grid();    // shared pointer
        auto cells = cm.cells();  // const reference
        
        assert(space->dim == 2);
        assert(space->periodic == true);
        assert(space->extent[0] == 42.);
        assert(space->extent[1] == 42.);

        assert(grid->shape()[0] == 23);
        assert(grid->shape()[1] == 32);

        assert(cells.size() == (23*32));
        assert(cells[0]->state().a_double == 2.34);
        assert(cells[0]->state().a_string == "foobar");
        assert(cells[0]->state().a_bool == true);

        std::cout << "Success." << std::endl << std::endl;
        

        std::cout << "------ Testing error messages ------" << std::endl;
        assert(check_error_message<std::invalid_argument>(
            "missing_grid_cfg",
            [&](){
                MockModel<CellTraitsEC> mm_ec("missing_grid_cfg",
                                              cfg["missing_grid_cfg"],
                                              initial_state);
            }, "Missing entry 'grid' in the config"));

        assert(check_error_message<std::invalid_argument>(
            "missing_grid_cfg2",
            [&](){
                MockModel<CellTraitsEC> mm_ec("missing_grid_cfg2",
                                              cfg["missing_grid_cfg2"],
                                              initial_state);
            }, "Missing one or both of the grid configuration entries"));

        assert(check_error_message<std::invalid_argument>(
            "missing_grid_cfg3",
            [&](){
                MockModel<CellTraitsEC> mm_ec("missing_grid_cfg3",
                                              cfg["missing_grid_cfg3"],
                                              initial_state);
            }, "Missing one or both of the grid configuration entries"));

        assert(check_error_message<std::invalid_argument>(
            "bad_grid_cfg",
            [&](){
                MockModel<CellTraitsEC> mm_ec("bad_grid_cfg",
                                              cfg["bad_grid_cfg"],
                                              initial_state);
            }, "Invalid value for grid 'discretization' argument: 'not_a"));

        assert(check_error_message<std::invalid_argument>(
            "missing_cell_init1",
            [&](){
                MockModel<CellTraitsCC> mm_cc("missing_cell_init1",
                                              cfg["missing_cell_init1"]);
            }, "Missing required configuration key 'cell_initialize_from'"));

        assert(check_error_message<std::invalid_argument>(
            "bad_cell_init1",
            [&](){
                MockModel<CellTraitsCC> mm_cc("bad_cell_init1",
                                              cfg["bad_cell_init1"]);
            }, "No valid constructor for the cells' initial state"));

        assert(check_error_message<std::invalid_argument>(
            "bad_cell_init2",
            [&](){
                MockModel<CellTraitsCC> mm_cc("bad_cell_init2",
                                              cfg["bad_cell_init2"]);
            }, "No valid constructor for the cells' initial state"));

        assert(check_error_message<std::invalid_argument>(
            "bad_cell_init3",
            [&](){
                MockModel<CellTraitsCC> mm_cc("bad_cell_init3",
                                              cfg["bad_cell_init3"]);
            }, "from a config node but a node with the key 'cell_initial_"));
        
        std::cout << "Success." << std::endl << std::endl;

        std::cout << "Total success." << std::endl << std::endl;
        return 0;
    }
    catch (std::exception& e) {
        std::cerr << "Exception occured: " << e.what() << std::endl;
        // NOTE cannot call cleanup here because the scope is not shared
        return 1;
    }
    catch (...) {
        std::cout << "Exception occurred!" << std::endl;
        // NOTE cannot call cleanup here because the scope is not shared
        return 1;
    }
}
