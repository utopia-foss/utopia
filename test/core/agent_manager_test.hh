#ifndef UTOPIA_TEST_AGENT_MANAGER_TEST_HH
#define UTOPIA_TEST_AGENT_MANAGER_TEST_HH

#include <utopia/core/model.hh>
#include <utopia/core/agent_manager.hh>

namespace Utopia {
namespace Test {
namespace AgentManager {

using DataIO::Config;
using Utopia::get_as;

/// An agent state definition that is default-constructible
struct AgentStateDC {
    double a_double;
    std::string a_string;
    bool a_bool;
};

/// An agent state definition that is config-constructible
struct AgentStateCC {
    double a_double;
    std::string a_string;
    bool a_bool;

    AgentStateCC(const Config& cfg)
    :
        a_double(get_as<double>("a_double", cfg)),
        a_string(get_as<std::string>("a_string", cfg)),
        a_bool(get_as<bool>("a_bool", cfg))
    {}
};

/// An agent state definition that is config-constructible and has an RNG
struct AgentStateRC {
    double a_double;
    std::string a_string;
    bool a_bool;

    template<class RNG>
    AgentStateRC(const Config& cfg, const std::shared_ptr<RNG>& rng)
    :
        a_double(get_as<double>("a_double", cfg)),
        a_string(get_as<std::string>("a_string", cfg)),
        a_bool(get_as<bool>("a_bool", cfg))
    {
        // Do something with the rng
        std::uniform_real_distribution<double> dist(0., a_double);
        a_bool = dist(*rng);
    }
};

/// An agent state definition that is only explicitly constructible
struct AgentStateEC {
    double a_double;
    std::string a_string;
    bool a_bool;

    AgentStateEC(double d, std::string s, bool b)
    :
        a_double(d),
        a_string(s),
        a_bool(b)
    {}
};


/// A custom links definition
template<typename AgentContainer>
struct TestLinks {
    /// A container of other agents that are "followed" by this agent ...
    AgentContainer following;
};


// Create some agent traits definitions.
// The second template parameter is for the update mode.
// The third for whether a default constructor is to be used.

/// For a default-constructible agent state
using AgentTraitsDC = Utopia::AgentTraits<AgentStateDC, Update::sync,
                                          true>;  // use default constructor

/// For a config-constructible agent state
using AgentTraitsCC = Utopia::AgentTraits<AgentStateCC, Update::sync>;

/// For a config-constructible agent state (with RNG) 
using AgentTraitsRC = Utopia::AgentTraits<AgentStateCC, Update::sync>;

/// For an explicitly-constructible agent state
using AgentTraitsEC = Utopia::AgentTraits<AgentStateEC, Update::sync>;

/// Agent traits with custom links
using AgentTraitsCL = Utopia::AgentTraits<AgentStateDC,
                                          Update::sync,
                                          true,    // use default constructor
                                          Utopia::EmptyTag,
                                          TestLinks>;

/// For a config-constructible agent state with synchronous update dynamics
using AgentTraitsCC_sync = Utopia::AgentTraits<AgentStateCC, Update::sync>;

/// For a config-constructible agent state with asynchronous update dynamics
using AgentTraitsCC_async = Utopia::AgentTraits<AgentStateCC, Update::async>;


/// A mock model class to hold the agent manager
template<class AgentTraits>
class MockModel {
public:
    using Space = Utopia::DefaultSpace;
    using AgentStateType = typename AgentTraits::State;
    using RNG = std::mt19937;
    static constexpr DimType dim = Space::dim;
    using SpaceVec = SpaceVecType<dim>;

    const std::string _name;
    const Config _cfg;
    const std::shared_ptr<RNG> _rng;
    std::shared_ptr<spdlog::logger> _log;

    std::shared_ptr<Space> _space;

    // The public agent manager (for easier testing access)
    Utopia::AgentManager<AgentTraits, MockModel> _am;


    // -- Constructors -- //
    /// Basic constructor
    MockModel(
        const std::string model_name,
        const Config& cfg,
        const Config& custom_am_cfg = {})
    :
        _name(model_name),
        _cfg(cfg),
        _rng(std::make_shared<RNG>(42)),
        _log(setup_logger(model_name)),
        _space(setup_space()),
        _am(*this, custom_am_cfg)
    {}

    /// Constructor with initial agent state
    MockModel(
        const std::string model_name,
        const Config& cfg,
        const AgentStateType agent_initial_state,
        const Config& custom_am_cfg = {})
    :
        _name(model_name),
        _cfg(cfg),
        _rng(std::make_shared<RNG>(42)),
        _log(setup_logger(model_name)),
        _space(setup_space()),
        _am(*this, agent_initial_state, custom_am_cfg)
    {}


    // -- Setup functions (needed because pseudo parent is not used) -- //

    std::shared_ptr<spdlog::logger> setup_logger(const std::string name) const
    {
        auto logger = spdlog::get(name);
    
        if (not logger) {
            // Create it; spdlog throws an exception if it already exists
            logger = spdlog::stdout_color_mt(name);
        }
        logger->set_level(spdlog::level::debug);
        logger->set_pattern("%n  %^%l%$  %v");
        return logger;
    }


    std::shared_ptr<Space> setup_space() const {
        if (_cfg["space"]) {
            // Build a space with the given parameters
            return std::make_shared<Space>(_cfg["space"]);
        }
        else {
            // Use the default space
            return std::make_shared<Space>();
        }
    }

    // -- Other functions, mirroring model interface -- //
    /// Return a mock logger
    std::shared_ptr<spdlog::logger> get_logger() const {
        return _log;
    }

    /// Return the space this model resides in
    std::shared_ptr<Space> get_space() const {
        return _space;
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

} // namespace AgentManager
} // namespace Test
} // namespace Utopia


#endif // UTOPIA_TEST_AGENT_MANAGER_TEST_HH
