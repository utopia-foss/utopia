#ifndef UTOPIA_MODELS_ENVIRONMENT_ENVPARAMFUNCCOLLECTION_HH
#define UTOPIA_MODELS_ENVIRONMENT_ENVPARAMFUNCCOLLECTION_HH

#include <math.h>

#include "tools.hh"

namespace Utopia::Models::Environment {
namespace ParameterFunctionCollection {


/// Configuration node type alias
using Config = DataIO::Config;

// -- Helper functions --------------------------------------------------------

/// Create a rule function that uses a random number distribution
/** \detail This constructs a mutable ``EnvParamFunc`` lambda, moving the
     *         ``dist`` into the capture.
     */
template<typename EnvModel, class DistType,
         class EnvParamFunc = typename EnvModel::EnvParamFunc>
EnvParamFunc
    build_rng_env_param_func(const EnvModel& model,
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
        [&model, param_name, mode, dist{std::move(dist)}] () mutable {
            double current_value = 0.;
            if (mode == ValMode::Add) {
                current_value = model.get_parameter(param_name);
            }
            const double rn = dist(*model.get_rng());

            return current_value + rn;
        };
}

/** 
 * \addtogroup Environment
 * \{
 */

// -- Environment parameter modification functions ----------------------------
// .. Keep these in alphabetical order and prefix with _epf_ ! .............
// NOTE The methods below do _not_ change any state, they just generate
//      a function object that does so at the desired point in time.

/// Creates a rule function for incrementing parameter values
/** 
 * \param param_name  The parameter to attach this environment function to
 * \param cfg
 *   \parblock
 *     Configuration for this environment function. Allows the following
 *     arguments:
 *
 *     - ``times``: Sequence of time points at which to invoke this. if
 *                  None, always incrementing
 *     - ``value``: The incrementor
 *     - ``invoke_at_initialization``: Whether to invoke at initialization.
 *                                     Default is False
 *   \endparblock
 */
template<typename EnvModel,
         class EnvParamFunc = typename EnvModel::EnvParamFunc>
EnvParamFunc epf_increment(const EnvModel& model, const std::string param_name,
                           const Config& cfg)
{
    const auto value = get_as<double>("value", cfg);
    EnvParamFunc epf = [&model, param_name, value] () mutable
    {
        return model.get_parameter(param_name) + value;
    };

    return epf;
}

/// Creates a rule function for random parameter values
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
         class EnvParamFunc = typename EnvModel::EnvParamFunc>
EnvParamFunc epf_random(const EnvModel& model, const std::string& param_name,
                       const Config& cfg)
{
    const auto mode = extract_val_mode(cfg, "random");
    const auto distribution = get_as<std::string>("distribution", cfg);

    // Depending on chosen distribution, construct it and build a rule
    // function using a reference to the newly created one...
    if (distribution == "normal") {
        const auto mean = get_as<double>("mean", cfg);
        const auto stddev = get_as<double>("stddev", cfg);
        std::normal_distribution<> dist(mean, stddev);

        return build_rng_env_param_func(model, std::move(dist), param_name,
                                        mode);
    }
    else if (distribution == "poisson") {
        const auto mean = get_as<double>("mean", cfg);
        std::poisson_distribution<> dist(mean);
        
        return build_rng_env_param_func(model, std::move(dist), param_name,
                                        mode);
    }
    else if (distribution == "exponential") {
        const auto lambda = get_as<double>("lambda", cfg);
        std::exponential_distribution<> dist(lambda);
        
        return build_rng_env_param_func(model, std::move(dist), param_name,
                                        mode);
    }
    else if (distribution == "uniform_int") {
        auto interval = get_as<std::array<int, 2>>("interval", cfg);
        std::uniform_int_distribution<> dist(interval[0], interval[1]);
        
        return build_rng_env_param_func(model, std::move(dist), param_name,
                                        mode);
    }
    else if (distribution == "uniform_real" or distribution == "uniform") {
        auto interval = get_as<std::array<double, 2>>("interval", cfg);
        std::uniform_real_distribution<> dist(interval[0], interval[1]);

        return build_rng_env_param_func(model, std::move(dist), param_name,
                                        mode);
    }
    else {
        throw std::invalid_argument("No method implemented to resolve "
            "noise distribution '" + distribution + "'! Valid options: "
            "normal, poisson, uniform_int, uniform_real.");
    }
};

/// Creates a rule function for rectangular function like parameter values
/** Rectangular shaped parameters. 
 * 
 * \param param_name  The parameter to attach this environment function to
 * \param cfg
 *   \parblock
 *     Configuration for this environment function. Allows the following
 *     arguments:
 * 
 *     - ``times``: Sequence of time points at which to invoke this. if
 *                  None, always setting
 *     - ``offset``: The minimum value. Optional, default 0.
 *     - ``amplitude``: The amplitude above minimum value
 *     - ``period``: The period
 *     - ``time_in_max``: The number of steps in max value. Optional, 
 *                        default half period.
 *     - ``phase``: The phase at first invocation in fraction of period length.
 *                  E.g.: 0.5 for initialization at half period.
 *                  Optional, default 0 equiv. in high value.
 * 
 *   \endparblock
 */
template<typename EnvModel,
         class EnvParamFunc = typename EnvModel::EnvParamFunc>
EnvParamFunc epf_rectangular(const EnvModel& model, const Config& cfg)
{
    using Time = typename EnvModel::Time;

    double min_value = get_as<double>("offset", cfg, 0.);
    double max_value = get_as<double>("amplitude", cfg) + min_value;
    Time period = get_as<Time>("period", cfg);
    Time time_in_max = get_as<Time>("time_in_max", cfg, period / 2.);
    double phase = get_as<double>("phase", cfg, 0) * period;

    // Check parameter
    if (time_in_max > period) {
        throw std::invalid_argument("The `time_in_max` argument cannot be "
                    "larger than the `period` argument in rectangular "
                    "environment parameter function!");
    }
    if (phase < 0 or phase > period) {
        throw std::invalid_argument("The `phase` argument was not in interval "
                    "[0., 1.]!");
    }

    // get starting time
    Time time_start = extract_time_start<Time>(cfg);

    // Build function
    EnvParamFunc epf = [&model, max_value, min_value,
                        period, time_in_max, phase, time_start] () mutable
    {
        auto time = (model.get_time()+1 - time_start) % period;
        if (time >= phase and time < time_in_max + phase) {
            return max_value;
        }
        return min_value;
    };

    return epf;
}

/// Creates a rule function for setting a parameter value
/** 
 * \param param_name  The parameter to attach this environment function to
 * \param cfg
 *   \parblock
 *     Configuration for this environment function. Allows the following
 *     arguments:
 * 
 *     - ``times``: Sequence of time points at which to invoke this. if
 *                  None, always setting
 *     - ``value``: The new value
 *     - ``invoke_at_initialization``: Whether to invoke at initialization.
 *                                     Default is False
 *   \endparblock
 */
template<typename EnvModel,
         class EnvParamFunc = typename EnvModel::EnvParamFunc>
EnvParamFunc epf_set(const EnvModel&, const Config& cfg)
{
    const auto value = get_as<double>("value", cfg);
    EnvParamFunc epf = [value] () mutable
    {
        return value;
    };

    return epf;
}

/// Creates a rule function for sinusoidal parameter values
/** 
 * \param param_name  The parameter to attach this environment function to
 * \param cfg
 *   \parblock
 *     Configuration for this environment function. Allows the following
 *     arguments:
 * 
 *     - ``times``: Sequence of time points at which to invoke this. if
 *                  None, always setting
 *     - ``period``: The period of the sinus
 *     - ``amplitude``: The amplitude
 *     - ``phase``: The phase shift at the point of first invokation. In
 *                  multiples of pi. 1 = pi or 180 degree shift. 
 *                  Optional, default 0. 
 *     - ``offset``: Offset of the mean value over the full period. 
 *                   Optional, default 0.
 * 
 *      \note Cannot be invoked at initialization.
 *   \endparblock
 */
template<typename EnvModel,
         class EnvParamFunc = typename EnvModel::EnvParamFunc>
EnvParamFunc epf_sinusoidal(const EnvModel& model, const Config& cfg)
{
    using Time = typename EnvModel::Time;

    const auto period = get_as<double>("period", cfg);
    const auto amplitude = get_as<double>("amplitude", cfg);
    double phase = 0;
    if (cfg["phase"]) {
        phase = get_as<double>("phase", cfg);
    }
    double offset = 0.;
    if (cfg["offset"]) {
        offset = get_as<double>("offset", cfg);
    }
    
    // get starting time
    Time time_start = extract_time_start<Time>(cfg);

    EnvParamFunc epf = [&model, period, amplitude, 
                        phase, offset, time_start] () mutable
    {   
        double x = (model.get_time() + 1 - time_start)/period * 2*M_PI;
        return offset + amplitude * sin(x + phase * M_PI);
    };

    return epf;
}

// end group Environment
/**
 *  \}
 */

} // namespace ParameterFunctionCollection
} // namespace Utopia::Models::Environment

#endif
