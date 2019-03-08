#ifndef UTOPIA_CORE_TEST_CELL_MANAGER_TEST_HH
#define UTOPIA_CORE_TEST_CELL_MANAGER_TEST_HH

#include <utopia/core/cell_manager.hh>

using Utopia::CellManager;
using Utopia::DataIO::Config;

/// A mock model class to hold the cell manager
template<class CellTraits>
class MockModel {
public:
    // -- Members -- //
    using Space = Utopia::DefaultSpace;
    using CellStateType = typename CellTraits::State;
    using RNG = std::mt19937;

    const std::string _name;
    const Config _cfg;
    const std::shared_ptr<RNG> _rng;
    std::shared_ptr<spdlog::logger> _log;

    Space _space;

    CellManager<CellTraits, MockModel> _cm;


    // -- Constructors -- //
    /// Basic constructor
    MockModel(const std::string model_name, const Config& cfg)
    :
        _name(model_name),
        _cfg(cfg),
        _rng(std::make_shared<RNG>(42)),
        _log(setup_logger(model_name)),
        _space(setup_space()),
        _cm(*this)
    {}

    /// Constructor with initial cell state
    MockModel(const std::string model_name, const Config& cfg,
              const CellStateType cell_initial_state)
    :
        _name(model_name),
        _cfg(cfg),
        _rng(std::make_shared<RNG>(42)),
        _log(setup_logger(model_name)),
        _space(setup_space()),
        _cm(*this, cell_initial_state)
    {}


    // -- Setup functions (needed because pseudo parent is not used) -- //

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

    // -- Other functions, mirroring model interface -- //
    /// Return a mock logger
    std::shared_ptr<spdlog::logger> get_logger() {
        return _log;
    }

    /// Return the space this model resides in
    std::shared_ptr<Space> get_space() {
        return std::make_shared<Space>(_space);
    }

    /// Return the config node of this model
    Config get_cfg() const {
        return _cfg;
    }

    /// Return the config node of this model
    std::shared_ptr<RNG> get_rng() const {
        return _rng;
    }

    /// Return the name of this model instance
    std::string get_name() const {
        return _name;
    }
};


#endif // UTOPIA_CORE_TEST_CELL_MANAGER_TEST_HH
