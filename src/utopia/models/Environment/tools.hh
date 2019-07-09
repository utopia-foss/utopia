#ifndef UTOPIA_MODELS_ENVIRONMENT_TOOLS_HH
#define UTOPIA_MODELS_ENVIRONMENT_TOOLS_HH

namespace Utopia::Models::Environment {

/** \addtogroup Environment
 *  \{
 */
/// Value calculation mode
enum class ValMode {
    /// Set a value, discarding the current state
    Set,

    /// Add the new value to the existing value
    Add
};

/// Given a configuration node, extract the value mode
ValMode extract_val_mode(const DataIO::Config& cfg,
                         const std::string& context)
{
    const auto mode_key = get_as<std::string>("mode", cfg);

    if (mode_key == "add") {
        return ValMode::Add;
    }
    else if (mode_key == "set") {
        return ValMode::Set;
    }

    throw std::invalid_argument("The `mode` argument for configuration of "
        "environment function " + context + " can be 'add' or 'set', but "
        "was '" + mode_key + "'!");
}

/// Given a configuration, extracts the set of times at which to invoke
/// environment functions
template <typename Time>
std::pair<bool, std::set<Time>> extract_times(const DataIO::Config& cfg) {
    bool invoke_always = true;
    std::set<Time> times;

    if (not cfg.IsMap()) {
        // Already return here
        return {invoke_always, times};
    }

    // Extract information from configuration
    if (cfg["times"]) {
        invoke_always = false;
        auto times_list = get_as<std::vector<Time>>("times", cfg);
        // TODO Consider wrapping negative values around

        // Make sure negative times or 0 is not included
        // NOTE 0 may not be included because the environment state functions
        //      are invoked separately for this time.
        times_list.erase(
            std::remove_if(times_list.begin(), times_list.end(),
                           [](auto& t){ return (t <= 0); }),
            times_list.end()
        );

        // Populate the set; this will impose ordering
        times.insert(times_list.begin(), times_list.end());
    }

    return {invoke_always, times};
}

/// Given a configuration, extracts the set of times at which to invoke
/// environment functions and whether to invoke them at initialization
template <typename Time>
std::tuple<bool, bool, std::set<Time>>
    extract_times_and_initialization(const DataIO::Config& cfg)
{
    bool invoke_at_initialization = get_as<bool>("invoke_at_initialization",
                                                 cfg);

    auto [invoke_always, times] = extract_times<Time>(cfg);

    return {invoke_at_initialization, invoke_always, times};
}

/// Given a configuration, extracts the time of first function invocation
template <typename Time>
Time extract_time_start(const DataIO::Config& cfg) {
    // get starting time
    auto times_list = get_as<std::vector<Time>>("times", cfg, {0});
    auto time_start = times_list.front();
    for (auto t : times_list) {
        if (t < time_start and t >= 0)
            time_start = t;
    }

    return time_start;
}


// End group Environment
/**
 *  \}
 */
    
} // namespace Utopia::Models::Environment

#endif
