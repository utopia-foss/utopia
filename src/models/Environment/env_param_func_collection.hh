#ifndef UTOPIA_MODELS_ENVIRONMENT_ENVPARAMFUNCCOLLECTION_HH
#define UTOPIA_MODELS_ENVIRONMENT_ENVPARAMFUNCCOLLECTION_HH

#include <math.h>

#include "tools.hh"

namespace Utopia::Models::Environment {
namespace ParameterFunctionCollection {


/// Configuration node type alias
using Config = DataIO::Config;

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
         class EnvParamFuncBundle = typename EnvModel::EnvParamFuncBundle>
EnvParamFuncBundle epf_increment(const EnvModel& model,
                                 const std::string& param_name,
                                 const Config& cfg)
{
    using EnvParamFunc = typename EnvModel::EnvParamFunc;
    using Time = typename EnvModel::Time;

    // Extract parameters
    const auto epfb_name = "increment." + param_name;
    auto invoke_times_tuple = extract_times_and_initialization<Time>(cfg);

    const auto value = get_as<double>("value", cfg);
    EnvParamFunc epf = [&model, param_name, value] () mutable
    {
        return model.get_parameter(param_name) + value;
    };

    return EnvParamFuncBundle(epfb_name, param_name, epf, invoke_times_tuple);
}

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
 *     - ``invoke_at_initialization``: Whether to invoke at initialization.
 *                                     Default is False
 *   \endparblock
 */
template<typename EnvModel,
         class EnvParamFuncBundle = typename EnvModel::EnvParamFuncBundle>
EnvParamFuncBundle epf_rectangular(const EnvModel& model,
                                   const std::string& param_name,
                                   const Config& cfg)
{
    using EnvParamFunc = typename EnvModel::EnvParamFunc;
    using Time = typename EnvModel::Time;

    // Extract parameters
    const auto epfb_name = "rectangular." + param_name;

    auto invoke_times_tuple = extract_times_and_initialization<Time>(cfg);

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

    return EnvParamFuncBundle(epfb_name, param_name, epf, invoke_times_tuple);
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
         class EnvParamFuncBundle = typename EnvModel::EnvParamFuncBundle>
EnvParamFuncBundle epf_set(const EnvModel&,
                           const std::string& param_name,
                           const Config& cfg)
{
    using EnvParamFunc = typename EnvModel::EnvParamFunc;
    using Time = typename EnvModel::Time;

    // Extract parameters
    const auto epfb_name = "set." + param_name;

    auto invoke_times_tuple = extract_times_and_initialization<Time>(cfg);

    const auto value = get_as<double>("value", cfg);
    EnvParamFunc epf = [value] () mutable
    {
        return value;
    };

    return EnvParamFuncBundle(epfb_name, param_name, epf, invoke_times_tuple);
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
         class EnvParamFuncBundle = typename EnvModel::EnvParamFuncBundle>
EnvParamFuncBundle epf_sinusoidal(const EnvModel& model,
                                  const std::string& param_name,
                                  const Config& cfg)
{
    using EnvParamFunc = typename EnvModel::EnvParamFunc;
    using Time = typename EnvModel::Time;

    // Extract parameters
    const auto epfb_name = "sinusoidal." + param_name;

    auto times_pair = extract_times<Time>(cfg);
    if (cfg["invoke_at_initialization"]) {
        throw std::invalid_argument("The `invoke_at_initialization` argument "
                    "is not available for the `sinusoidal` environment "
                    "parameter function!");
    }

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

    return EnvParamFuncBundle(epfb_name, param_name, epf, 
                              false, times_pair.first, times_pair.second);
}

// end group Environment
/**
 *  \}
 */

} // namespace ParameterFunctionCollection
} // namespace Utopia::Models::Environment

#endif
