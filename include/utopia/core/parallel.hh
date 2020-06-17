#ifndef UTOPIA_CORE_PARALLEL_HH
#define UTOPIA_CORE_PARALLEL_HH

#include <algorithm>
#include <exception>
#include <execution>

#include <utopia/core/logging.hh>
#include <utopia/data_io/cfg_utils.hh>

#if defined(HAVE_PARALLEL_STL) && defined(ENABLE_PARALLEL_STL)
#define UTOPIA_PARALLEL
#endif

namespace Utopia
{

/**
 *  \addtogroup Parallel
 *  \{
 */

/// Runtime execution policies
/**
 *  These policies directly relate to the C++ standard policies with the
 *  adjustment that they can be set at runtime and only apply if enabled by
 *  the build system.
 *
 *  \warning Depending on the nature of the parallelized operation, data races
 *           may occur when executing algorithms in parallel. Users themselves
 *           are responsible for avoiding data races!
 */
enum ExecPolicy
{
    /// Sequential (i.e., regular) execution
    /**
     *  If parallel features are disabled at runtime or compile-time, all
     *  parallel algorithms behave as if they are called with this policy.
     */
    seq,
    unseq,    //!< SIMD execution on single thread
    par,      //!< Parallel/multithreaded execution
    par_unseq //!< SIMD execution on multiple threads
};

/// Static information on the status of parallel execution
/**
 *  \note Using this class requires the Utopia core logger to be set up, which
 *        can conveniently done by calling Utopia::setup_loggers().
 */
class ParallelExecution
{
private:
    /// Runtime setting for parallel execution
    inline static bool _enabled = false;

    /// Fetch the core logger
    /**
     *  \return Valid shared pointer to the logger
     *  \throw runtime_error If logger does not exist
     */
    static std::shared_ptr<spdlog::logger> get_logger()
    {
        const auto log = spdlog::get(Utopia::log_core);
        if (not log)
        {
            throw std::runtime_error("Cannot fetch core logger!");
        }

        return log;
    }

public:
    /// Possible settings for parallel execution
    enum Setting
    {
        enabled, //!< Enable parallel execution
        disabled //!< Disable parallel execution
    };

    /// Initialize parallel features based on configuration setting
    /**
     *  \param cfg Parameter space config node
     *  \note If the required parameter is not found, parallel features are
     *        **disabled** by default.
     */
    static void init(const DataIO::Config& cfg)
    {
        bool setting = false;

        // Try fetching settings on parallel execution
        try
        {
            const YAML::Node& cfg_par = cfg["parallel_execution"];
            setting = get_as<bool>("enabled", cfg_par);
        }
        // NOTE: Pass if the node 'parallel_execution' cannot be found, but
        //       throw if the queried keys are not inside the existing node.
        catch (YAML::InvalidNode&)
        {
        }
        catch (...)
        {
            throw;
        }

        if (setting)
            set(Setting::enabled);
        else
            set(Setting::disabled);
    }

    /// Choose a setting for parallel execution at runtime
    /**
     *  This setting may be changed at any time during runtime. However,
     *  algorithms currently running will continue with the policy they have
     *  been started with.
     */
    static void set(const Setting value)
    {
        if (value == Setting::enabled)
            _enabled = true;
        else
            _enabled = false;

        const auto log = get_logger();
        std::string msg = fmt::format("Parallel execution {}",
                                      _enabled ? "enabled" : "disabled");

#ifdef UTOPIA_PARALLEL
        log->info(msg);
#else
        msg += ", but settings do NOT apply";
        if (_enabled)
            log->warn(msg);
        else
            log->debug(msg);
#endif
    }

    /// Query if parallel execution is currently enabled
    /**
     *  \note This value does *not* imply if parallel execution actually applies.
     *        If prerequisites are not met, parallel algorithms will default to
     *        sequential execution.
     */
    static bool is_enabled() { return _enabled; }

    /// Actually check if parallel features are applied at runtime
    /**
     *  \note This method is implemented for testing purposes only and should
     *        not be used to change model or algorithm behavior!
     */
    static bool is_applied()
    {
#ifdef UTOPIA_PARALLEL
        return _enabled;
#else
        return false;
#endif
    }
};

/// Call a function with an STL execution policy
/**
 *  This function takes a function object `f` as argument and calls it with
 *  the STL execution policy relating to the Utopia execution policy passed
 *  as argument. This only applies if parallel features are enabled at compile
 *  time.
 *
 *  If parallel execution was enabled at compile-time, this function actually
 *  compiles four *different* STL algorithms, one for each possible execution
 *  policy. This works because by definition of the STL they all have the same
 *  return type.
 *
 *  See
 *  https://stackoverflow.com/questions/52975114/different-execution-policies-at-runtime
 *  for the inspiration to this implementation.
 *
 *  \param policy Utopia execution policy
 *  \param f Function (object) taking an STL execution policy as single
 *           argument.
 */
template<class Func>
auto
exec_parallel([[maybe_unused]] const Utopia::ExecPolicy policy, Func&& f)
{
#ifdef UTOPIA_PARALLEL
    if (Utopia::ParallelExecution::is_enabled())
    {
        if (policy == Utopia::ExecPolicy::unseq)
            return f(std::execution::unseq);
        else if (policy == Utopia::ExecPolicy::par)
            return f(std::execution::par);
        else if (policy == Utopia::ExecPolicy::par_unseq)
            return f(std::execution::par_unseq);
    }
#endif
    return f(std::execution::seq);
}

/**
 *  \} // group Parallel
 */

} // namespace Utopia

namespace std
{

/**
 *  \addtogroup Algorithm STL Algorithm Overloads
 *  \brief Overloads for selecting execution policies at runtime
 *  \ingroup Parallel
 *
 *  These algorithms are overloads of the respective
 *  [STL algorithms](https://en.cppreference.com/w/cpp/algorithm)
 *  where the `ExecutionPolicy` template parameter has been replaced by
 *  the Utopia::ExecPolicy runtime parameter. Developers can use these overloads
 *  to have the Utopia \ref Parallel "Parallel Facilities" decide the actual
 *  execution policy at runtime. This is done by inserting the algorithm
 *  into the Utopia::exec_parallel function.
 *
 *  Using these overloads, developers still have to take care of potential
 *  data races, as if they are always executed with the intended execution
 *  policy.
 *
 *  ### How to add more algorithms
 *
 *  To add another algorithm overload, have a look at the original algorithm
 *  signature. A list of STL algorithms can be found
 *  [here](https://en.cppreference.com/w/cpp/algorithm). Copy the signature
 *  including the `ExecutionPolicy`. Remove the `ExecutionPolicy` template
 *  parameter and replace the `ExecutionPolicy` argument with
 *  Utopia::ExecPolicy. Then call Utopia::exec_parallel with the
 *  Utopia::ExecPolicy as first argument and a generic lambda as second
 *  argument. The lambda captures by reference (`[&]`), takes an STL policy
 *  as argument, and calls the intended STL algorithm with this policy and
 *  all other arguments. Depending on the algorithm return type, make sure to
 *  return the STL algorithm return value.
 *
 *  Example:
 *  \code{.cc}
 *  #include <utopia/core/parallel.hh>
 *
 *  // Inject into STL namespace for true overload
 *  namespace std {
 *
 *  // Same template signature as sequential `do_stuff`, plus exec policy
 *  template<class InputIt, class OutputIt>
 *  OutputIt
 *  do_stuff(const Utopia::ExecPolicy policy,  // <-- Runtime policy
 *           InputIt first,
 *           InputIt last,
 *           OutputIt d_first)
 *  {
 *
 *      return Utopia::exec_parallel(
 *          policy,  // <-- Runtime policy passed to this function
 *          [&](auto stl_policy) {  // <-- Compile-time policy selected
 *              // Now call actual STL algorithm, minding the return!
 *              return std::do_stuff(stl_policy, first, last, d_first);
 *      });
 *  }
 *
 *  } // namespace std
 *  \endcode
 *
 *  \{
 */

/// Copy the input range to a new range
/**
 *  See https://en.cppreference.com/w/cpp/algorithm/copy
 */
template<class InputIt, class OutputIt>
OutputIt
copy(const Utopia::ExecPolicy policy,
     InputIt first,
     InputIt last,
     OutputIt d_first)
{
    return Utopia::exec_parallel(policy, [&](auto stl_policy) {
        return std::copy(stl_policy, first, last, d_first);
    });
}

/// Apply a function to a range
/**
 *  See https://en.cppreference.com/w/cpp/algorithm/for_each
 */
template<class InputIt, class UnaryFunction>
void
for_each(const Utopia::ExecPolicy policy,
         InputIt first,
         InputIt last,
         UnaryFunction f)
{
    // Funky syntax lets the magic happen
    return Utopia::exec_parallel(policy, [&](auto stl_policy) {
        std::for_each(stl_policy, first, last, f);
    });
}

/// Apply a unary operator to a range and store the result in a new range
/**
 *  See https://en.cppreference.com/w/cpp/algorithm/transform
 */
template<class InputIt, class OutputIt, class UnaryOperation>
OutputIt
transform(const Utopia::ExecPolicy policy,
          InputIt first1,
          InputIt last1,
          OutputIt d_first,
          UnaryOperation unary_op)
{
    return Utopia::exec_parallel(policy, [&](auto stl_policy) {
        return std::transform(stl_policy, first1, last1, d_first, unary_op);
    });
}

/// Apply a binary operator to two ranges and store the result in a new range
/**
 *  See https://en.cppreference.com/w/cpp/algorithm/transform
 */
template<class InputIt1, class InputIt2, class OutputIt, class BinaryOperation>
OutputIt
transform(const Utopia::ExecPolicy policy,
          InputIt1 first1,
          InputIt1 last1,
          InputIt2 first2,
          OutputIt d_first,
          BinaryOperation binary_op)
{
    return Utopia::exec_parallel(policy, [&](auto stl_policy) {
        return std::transform(
            stl_policy, first1, last1, first2, d_first, binary_op);
    });
}

/**
 *  \}
 */

} // namespace std

#endif // UTOPIA_CORE_PARALLEL_HH
