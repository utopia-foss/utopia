#ifndef UTOPIA_MODELS_ENVIRONMENT_ENVSTATEFUNCCOLLECTION_HH
#define UTOPIA_MODELS_ENVIRONMENT_ENVSTATEFUNCCOLLECTION_HH

#include "tools.hh"

namespace Utopia::Models::Environment {
namespace StateFunctionCollection {

/// Configuration node type alias
using Config = DataIO::Config;

// -- Helper functions --------------------------------------------------------

/// Create a rule function that uses a random number distribution
/** \details This constructs a mutable ``EnvStateFunc`` lambda, moving the
     *         ``dist`` into the capture.
     */
template<typename EnvModel, class DistType,
         class EnvStateFunc = typename EnvModel::EnvStateFunc>
EnvStateFunc
    build_rng_env_state_func(EnvModel& model,
                             DistType&& dist,
                             const std::string& param_name,
                             const ValMode& mode)
{
    // NOTE It is VITAL to move-construct the perfectly-forwarded dist into
    //      the lambda; otherwise it has to be stored outside, which is a
    //      real pita. Also, the lambda has to be declared mutable such
    //      that the captured object are allowed to be changed; again, this
    //      is only relevant for the distribution's internal state ...
    return
        [&model, param_name, mode, dist{std::move(dist)}]
        (const auto& env_cell) mutable {
            auto& env_state = env_cell->state;

            double current_value = 0.;
            if (mode == ValMode::Add) {
                current_value = env_state.get_env(param_name);
            }
            const double rn = dist(*model.get_rng());

            env_state.set_env(param_name, current_value + rn);
            return env_state;
        };
}

/** \addtogroup Environment
 *  \{
 */

// -- Environment state modification functions --------------------------------
// .. Keep these in alphabetical order and prefix with _esf_ ! .............
// NOTE The methods below do _not_ change any state, they just generate
//      a function object that does so at the desired point in time.

/// Creates a rule function for noisy parameter values
/**
 * \param param_name  The parameter to attach this environment function to
 * \param cfg
 *   \parblock
 *     Configuration for this environment function. Allows the following
 *     arguments:
 *
 *     - ``mode``: ``set`` (default) or ``add``
 *     - ``times``: Sequence of time points at which to invoke this
 *     - ``distribution``: The distribution type. For each value below, the
 *       corresponding additional parameters are required in ``cfg``:
 *         - ``normal``: ``mean`` and ``stddev``
 *         - ``poisson``: ``mean``
 *         - ``exponential``: ``lambda``
 *         - ``uniform``: ``interval`` (length 2 array)
 *         - ``uniform_int``: ``interval`` (length 2 array)
 *   \endparblock
 */
template<typename EnvModel,
         class EnvStateFunc = typename EnvModel::EnvStateFunc>
std::pair<EnvStateFunc, Update> esf_noise(const EnvModel& model,
                                          const std::string& param_name,
                                          const Config& cfg)
{
    const auto mode = extract_val_mode(cfg, "noise");
    const auto distribution = get_as<std::string>("distribution", cfg);

    // Depending on chosen distribution, construct it and build a rule
    // function using a reference to the newly created one...
    if (distribution == "normal") {
        const auto mean = get_as<double>("mean", cfg);
        const auto stddev = get_as<double>("stddev", cfg);
        std::normal_distribution<> dist(mean, stddev);

        auto esf = build_rng_env_state_func(model, std::move(dist), param_name,
                                            mode);
        return {esf, Update::sync};
    }
    else if (distribution == "poisson") {
        const auto mean = get_as<double>("mean", cfg);
        std::poisson_distribution<> dist(mean);

        auto esf = build_rng_env_state_func(model, std::move(dist), param_name,
                                            mode);
        return {esf, Update::sync};
    }
    else if (distribution == "exponential") {
        const auto lambda = get_as<double>("lambda", cfg);
        std::exponential_distribution<> dist(lambda);

        auto esf = build_rng_env_state_func(model, std::move(dist), param_name,
                                            mode);
        return {esf, Update::sync};
    }
    else if (distribution == "uniform_int") {
        auto interval = get_as<std::array<int, 2>>("interval", cfg);
        std::uniform_int_distribution<> dist(interval[0], interval[1]);

        auto esf = build_rng_env_state_func(model, std::move(dist), param_name,
                                            mode);
        return {esf, Update::sync};
    }
    else if (distribution == "uniform_real" or distribution == "uniform") {
        auto interval = get_as<std::array<double, 2>>("interval", cfg);
        std::uniform_real_distribution<> dist(interval[0], interval[1]);

        auto esf = build_rng_env_state_func(model, std::move(dist), param_name,
                                            mode);
        return {esf, Update::sync};
    }
    else {
        throw std::invalid_argument("No method implemented to resolve "
            "noise distribution '" + distribution + "'! Valid options: "
            "normal, poisson, uniform_int, uniform_real.");
    }
};

/// Creates a rule function for spatially linearly parameter values
/**
 * \param param_name  The parameter to attach this environment function to
 * \param cfg
 *   \parblock
 *     Configuration for this environment function. Allows the following
 *     arguments:
 *
 *     - ``mode``: ``set`` (default) or ``add``
 *     - ``times``: Sequence of time points
 *     - ``values_north_south``: Values at northern and souther boundary;
 *       uses linear interpolation in between.
 *   \endparblock
 * \param extent      The extent of space
 */
template<typename EnvModel, typename Extent,
         class EnvStateFunc = typename EnvModel::EnvStateFunc>
std::pair<EnvStateFunc, Update> esf_slope(const EnvModel&,
                             const std::string& param_name,
                             const Config& cfg,
                             const Extent& extent)
{
    const auto mode = extract_val_mode(cfg, "slope");

    const auto values_north_south =
        get_as<std::array<double, 2>>("values_north_south", cfg);

    EnvStateFunc esf =
        [param_name, mode, values_north_south, extent]
        (const auto& env_cell) mutable
    {
        auto& env_state = env_cell->state;

        // Use the relative position along y-dimension
        const double pos = (  env_state.position[1] / extent[1]);
        const double slope = values_north_south[0] - values_north_south[1];
        const double value = values_north_south[1] + pos * slope;

        double current_value = 0.;
        if (mode == ValMode::Add) {
            current_value = env_state.get_env(param_name);
        }
        env_state.set_env(param_name, current_value + value);
        return env_state;
    };

    return {esf, Update::sync};
};

/// Creates a rule function for spatial steps in the parameter values
/**
 * \param param_name  The parameter to attach this environment function to
 * \param cfg
 *   \parblock
 *     Configuration for this environment function. Allows the following
 *     arguments:
 *
 *     - ``mode``: ``set`` (default) or ``add``
 *     - ``times``: Sequence of time points
 *     - ``values_north_south``: Sequence of parameter values for the step
 *       heights, from north to south.
 *     - ``latitudes``: Sequence of latitudes of separation, from north to
 *       south
 *   \endparblock
 */
template<typename EnvModel,
         class EnvStateFunc = typename EnvModel::EnvStateFunc>
std::pair<EnvStateFunc, Update> esf_steps(const EnvModel&,
                             const std::string& param_name,
                             const Config& cfg)
{
    const auto mode = extract_val_mode(cfg, "steps");

    const auto latitudes =
        get_as<std::vector<double>>("latitudes", cfg, {0.5});
    const auto values_north_south =
        get_as<std::vector<double>>("values_north_south", cfg);

    if (latitudes.size() != values_north_south.size() - 1) {
        throw std::invalid_argument("The list of 'latitudes' and"
            " 'values_north_south' don't match in size. Sizes were "
            + std::to_string(latitudes.size()) + " and "
            + std::to_string(values_north_south.size()) +
            ". Values_north_south must have one element more that"
            " latitudes.");
    }

    EnvStateFunc esf =
        [param_name, mode, latitudes, values_north_south]
        (const auto& env_cell) mutable
    {
        auto& env_state = env_cell->state;
        double value = values_north_south[0];
        for (unsigned int i = 0; i < latitudes.size(); ++i) {
            if (env_state.position[1] > latitudes[i]) {
                break;
            }
            value = values_north_south[i+1];
        }

        double current_value = 0.;
        if (mode == ValMode::Add) {
            current_value = env_state.get_env(param_name);
        }
        env_state.set_env(param_name, current_value + value);
        return env_state;
    };

    return {esf, Update::sync};
};

/// Creates a rule function for spatially uniform parameter values
/**
 * \param param_name  The parameter to attach this environment function to
 * \param cfg
 *   \parblock
 *     Configuration for this environment function. Allows the following
 *     arguments:
 *
 *     - ``mode``: ``set`` (default) or ``add``
 *     - ``times``: Sequence of time points
 *     - ``value``: The scalar value to use
 *   \endparblock
 */
template<typename EnvModel,
         class EnvStateFunc = typename EnvModel::EnvStateFunc>
std::pair<EnvStateFunc, Update> esf_uniform(const EnvModel&,
                                            const std::string& param_name,
                                            const Config& cfg)
{
    ValMode mode;
    double value;

    // Extract configuration depending on whether cfg is scalar or mapping
    if (cfg.IsScalar()) {
        // Interpret as desiring to set to the given scalar value
        mode = ValMode::Set;
        value = cfg.as<double>();
    }
    else if (cfg.IsMap()) {
        mode = extract_val_mode(cfg, "uniform");
        value = get_as<double>("value", cfg);
    }
    else {
        throw std::invalid_argument("The configuration for environment "
            "function 'uniform' must be a scalar or a mapping!");
    }

    EnvStateFunc esf =
        [param_name, mode, value]
        (const auto& env_cell) mutable
    {
        auto& env_state = env_cell->state;

        double current_value = 0.;
        if (mode == ValMode::Add) {
            current_value = env_state.get_env(param_name);
        }

        env_state.set_env(param_name, current_value + value);
        return env_state;
    };

    return {esf, Update::sync};
}

// End group Environment
/**
 *  \}
 */

} // namespace StateFunctionCollection
} // namespace Utopia::Models::Environment

#endif
