#ifndef UTOPIA_MODELS_ENVIRONMENT_FUNCBUNDLE_HH
#define UTOPIA_MODELS_ENVIRONMENT_FUNCBUNDLE_HH

namespace Utopia::Models::Environment {
namespace FuncBundle {

/** \addtogroup Environment
 *  \{
 */

/// A bundle wrapping a (iterative) function with metadata
/** \details This gathers a function alongside some metadata into a custom
 *          construct. The function is ment to be applied within an iterative
 *          model.
 */
template<typename Func, typename Time>
struct FuncBundle {
    std::string name;       /// The name of the function
    Func func;              /// The function
    bool invoke_at_initialization;  /// Whether to invoke at initialization
    bool invoke_always;     /// Whether to invoke in every timestep
    std::set<Time> times;   /// When to invoke

    FuncBundle(std::string name, Func func,
            bool invoke_at_initialization = true, bool invoke_always = true,
            std::set<Time> times = {})
    :
        name(name),
        func(func),
        invoke_at_initialization(invoke_at_initialization),
        invoke_always(invoke_always),
        times(times)
    {}
};

/// A bundle wrapping a (iterative) function with metadata
/** \details This gathers a function alongside some metadata into a custom
 *          construct. The function is ment to be applied within an iterative
 *          model.
 */
template<typename Func, typename Time>
struct ParamFuncBundle : FuncBundle<Func, Time> {
    /// The name of the parameter to which to apply the function
    std::string param_name;

    ParamFuncBundle(std::string name, Func func, std::string param_name,
            bool invoke_at_initialization = true, bool invoke_always = true,
            std::set<Time> times = {})
    :
        FuncBundle<Func, Time>(name, func, invoke_at_initialization,
                invoke_always, times),
        param_name(param_name)
    {}

    ParamFuncBundle(std::string name, Func func, std::string param_name,
            std::tuple<bool, bool, std::set<Time>> invoke_times_tuple)
    :
        ParamFuncBundle<Func, Time>(name, func, param_name,
                std::get<0>(invoke_times_tuple),
                std::get<1>(invoke_times_tuple),
                std::get<2>(invoke_times_tuple))
    {}
};

/// A bundle wrapping a (rule-)function with metadata
/** \details This gathers a rule function alongside some metadata into a custom
 *          construct. It can (optional) carry its own cellcontainer or a Config
 *          how to select cells from the cell manager.
 */
template<typename RuleFunc, typename Time, typename CellContainer>
struct RuleFuncBundle : FuncBundle<RuleFunc, Time> {
    /// The update mode of the RuleFunc
    Update update;

    // Select a subset of cells
    /// Whether the selection of cells is fixed
    bool fix_selection;
    /// Cell container over which to apply the func, optional
    CellContainer cell_selection;
    /// Config node that is passed to the select_cells of the cell manager
    DataIO::Config select_cfg;

    RuleFuncBundle(std::string name, RuleFunc func,
            Update update = Update::sync, bool invoke_at_initialization = true,
            bool invoke_always = true, std::set<Time> times = {},
            bool fix_selection = false,
            const CellContainer& cell_selection = {},
            const DataIO::Config& select_cfg = {})
    :
        FuncBundle<RuleFunc, Time>(name, func, invoke_at_initialization,
                invoke_always, times),
        update(update),
        fix_selection(fix_selection),
        cell_selection(cell_selection),
        select_cfg(select_cfg)
    { }

    RuleFuncBundle(std::string name, RuleFunc func, Update update,
            bool invoke_at_initialization,
            std::pair<bool, std::set<Time>> times_pair,
            std::tuple<bool, CellContainer, DataIO::Config> select_tuple)
    :
        RuleFuncBundle<RuleFunc, Time, CellContainer>(name, func, update,
                invoke_at_initialization, times_pair.first, times_pair.second,
                std::get<bool>(select_tuple),
                std::get<CellContainer>(select_tuple),
                std::get<DataIO::Config>(select_tuple))
    {}
};

// End group Environment
/**
 *  \}
 */

} // namespace FuncBundle
} // namespace Utopia::Models::Environment

#endif
