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
        _space(cfg["space"]),
        _cm(*this)
    {}

    /// Constructor with initial cell state
    MockModel(const std::string model_name, const Config& cfg,
              const CellStateType cell_initial_state)
    :
        _cfg(cfg),
        _log(setup_logger(model_name)),
        _space(cfg["space"]),
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



// ----------------------------------------------------------------------------

int main(int argc, char *argv[]) {
    try {
        Dune::MPIHelper::instance(argc,argv);

        std::cout << "Getting config file ..." << std::endl;
        auto cfg = YAML::LoadFile("cell_manager_test.yml");

        std::cout << "------ Initializing ... mock models ------" << std::endl;
        
        // Initialize the mock model with default-constructible cell type
        std::cout << "--- ... default-constructible ..." << std::endl;
        MockModel<CellTraitsDC> mm_dc("mm_dc", cfg["default"]);
        std::cout << "success." << std::endl << std::endl;
        
        // Initialize the mock model with config-constructible cell type
        std::cout << "--- ... config-constructible ..." << std::endl;
        MockModel<CellTraitsCC> mm_cc("mm_cc", cfg["config"]);
        std::cout << "success." << std::endl << std::endl;
        
        // Initialize the mock model with config-constructible cell type
        std::cout << "--- ... explicitly constructible ..." << std::endl;
        const auto initial_state = CellStateEC(2.34, "foobar", true);
        MockModel<CellTraitsEC> mm_ec("mm_ec", cfg["explicit"], initial_state);
        std::cout << "success." << std::endl << std::endl;

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
