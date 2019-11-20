#ifndef UTOPIA_MODELS_ENVIRONMENT_HH
#define UTOPIA_MODELS_ENVIRONMENT_HH

// standard library includes
#include <random>
#include <unordered_map>

// third-party library includes

// Utopia-related includes
#include <utopia/core/model.hh>
#include <utopia/core/apply.hh>
#include <utopia/core/types.hh>
#include <utopia/core/cell_manager.hh>

// FuncBundle classes
#include "func_bundle.hh"

// Collections of environment functions
#include "env_param_func_collection.hh"
#include "env_state_func_collection.hh"


namespace Utopia {
namespace Models {
namespace Environment {

/** \addtogroup Environment
 *  \{
 *  \details For details on how this is to be used, consult the actual model
 *           documentation. The doxygen documentation here provides merely the
 *           API reference and information on the available parameters for each
 *           of the environment functions.
 */

using namespace ParameterFunctionCollection;
using namespace StateFunctionCollection;

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Base type for environment parameter
/** \details This class is meant to be derived from and used as a basis for the
  *         desired kind of environment.
  */
struct BaseEnvParam {
    virtual ~BaseEnvParam() = default;

    /// Get an environment variable
    virtual double get_env(const std::string&) const = 0;

    /// Set an environment variable
    virtual void set_env(const std::string&, const double&) = 0;
};

/// Dummy type for environment parameter
/** \details This class is meant to be derived from and used as a basis for the
  *         desired kind of environment.
  */
struct DummyEnvParam : BaseEnvParam
{
    DummyEnvParam(const DataIO::Config&) { }

    ~DummyEnvParam() = default;

    /// Getter
    double get_env(const std::string&) const override {
        throw std::invalid_argument("Accessing getter of the dummy type of "
                                    "EnvParam!");
    }

    /// Setter
    void set_env(const std::string&, const double&) override {
        throw std::invalid_argument("Accessing setter of the dummy type of "
                                    "EnvParam!");
    }
};

/// Base type for environment cell states
/** \details This class is meant to be derived from and used as a basis for the
  *         desired kind of environment.
  */
struct BaseEnvCellState {
    using SpaceVec = SpaceVecType<2>;

    /// Cached barycenter of the cell
    SpaceVec position;

    virtual ~BaseEnvCellState() = default;

    /// Get an environment variable
    virtual double get_env(const std::string&) const = 0;

    /// Set an environment variable
    virtual void set_env(const std::string&, const double&) = 0;
};

/// Dummy type for environment cell states
/** \details This class is meant to be derived from and used as a basis for the
  *         desired kind of environment.
  */
struct DummyEnvCellState : BaseEnvCellState
{
    DummyEnvCellState(const DataIO::Config&) { }

    ~DummyEnvCellState() = default;

    /// Getter
    double get_env(const std::string&) const override {
        throw std::invalid_argument("Accessing getter of the dummy type of "
                                    "EnvCellState!");
    }

    /// Setter
    void set_env(const std::string&, const double&) override {
        throw std::invalid_argument("Accessing setter of the dummy type of "
                                    "EnvCellState!");
    }
};


/// Type helper to define types used by the model
using ModelTypes = Utopia::ModelTypes<>;


// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The Environment model provides a non-uniform, dynamic parameter background
/** This happens by coupling to another model's CellManager. Additionally,
 *  global parameters can also be changed.
 *
 * \tparam EnvParam       The parameter type of the environment
 * \tparam EnvCellState   The cell state type of the environment cells
 * \tparam standalone     Whether to have the model as a standalone model
 */
template<typename EnvParam=DummyEnvParam,
         typename EnvCellState=DummyEnvCellState,
         bool standalone=false>
class Environment:
    public Model<Environment<EnvParam, EnvCellState, standalone>, ModelTypes>
{
public:
    /// Type of this class
    using Self = Environment<EnvParam, EnvCellState, standalone>;

    /// The type of the Model base class of this derived class
    using Base = Model<Self, ModelTypes>;

    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename Base::DataGroup;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Cell traits specialization using the state type
    using CellTraits = Utopia::CellTraits<EnvCellState, Update::manual>;

    /// The type of the cell manager
    using CellManager = Utopia::CellManager<CellTraits, Self>;

    /// The type of the EnvCellContainer
    using EnvCellContainer = CellContainer<typename CellManager::Cell>;

    /// The type of the model time
    using Time = typename Base::Time;

    /// The type of the environment parameter functions
    using EnvParamFunc = typename std::function<double()>;

    /// The type wrapping EnvParamFunc with metadata
    using EnvParamFuncBundle = FuncBundle::ParamFuncBundle<EnvParamFunc, Time>;

    /// The type of the environment state functions; basically a rule function
    using EnvStateFunc = typename CellManager::RuleFunc;

    /// The type wrapping EnvStateFunc with metadata
    using EnvStateFuncBundle = FuncBundle::RuleFuncBundle<EnvStateFunc, Time,
                                                          EnvCellContainer>;

    /// Configuration node type alias
    using Config = DataIO::Config;

private:
    /// A dummy CellManager type that is used in standalone mode
    struct DummyCellManager {
        /// Always returns an empty node
        DataIO::Config cfg() const {
            return {};
        }
    };

    // -- Members -------------------------------------------------------------
    // Base members: _time, _name, _cfg, _hdfgrp, _rng, _monitor, _space

    /// The cell manager
    CellManager _cm;

    /// The environment parameters
    EnvParam _params;

    /// Container of functions that are invoked every time step
    /** Functions changing a parameter*/
    std::vector<EnvParamFuncBundle> _env_param_funcs;

    /// Container of rule functions that are invoked once at initialisation
    /** Functions changing a state*/
    std::vector<EnvStateFuncBundle> _init_env_state_funcs;

    /// Container of rule functions that are invoked every time step
    /** Functions changing a state*/
    std::vector<EnvStateFuncBundle> _env_state_funcs;

    // .. Datasets ............................................................
    /// Dynamically generated map of datasets of cell states
    std::unordered_map<std::string, std::shared_ptr<DataSet>> _dsets_state;

    /// Dynamically generated map of datasets of parameters
    std::unordered_map<std::string, std::shared_ptr<DataSet>> _dsets_param;


public:
    // -- Model Setup ---------------------------------------------------------
    /// Construct the Environment model associated to a CellManager
    /** \param name     Name of this model instance
     *  \param parent   The parent model this model instance resides in
     *  \param associate_cm    The CellManager of the associate (i.e. parent)
     *                         model. The Environment model's CellManager will
     *                         use the configuration of that CellManager.
     */
    template<class ParentModel, class CellManager>
    Environment (const std::string name,
                 ParentModel& parent,
                 CellManager&& associate_cm)
    :
        // Initialize first via base model
        Base(name, parent),

        // Set up the internal CellManager
        _cm(*this, associate_cm.cfg()),
        // NOTE If called from the standalone constructor, the .cfg() call
        //      always returns an empty configuration which leads to this
        //      internal CellManager being set up from _cfg["cell_manager"]

        //set up the environment params
        _params(this->_cfg),

        // Set up environment function bundle containers empty
        _env_param_funcs{},
        _init_env_state_funcs{},
        _env_state_funcs{},

        // Map of datasets; empty until states are set to be tracked
        _dsets_state{},
        _dsets_param{}
    {
        // Associate the CellManager's cells with each other
        if constexpr (not std::is_same<CellManager, DummyCellManager>()) {
            for (std::size_t i=0; i < _cm.cells().size(); i++) {
                associate_cm.cells()[i]->custom_links().env = _cm.cells()[i];
            }

            this->_log->info("Associated '{}' cells with those of the parent "
                             "model '{}'.", this->_name, parent.get_name());
        }
        else if constexpr (not standalone) {
            // Only allow coupled models without coupled cellmanagers when
            // a EnvCellState is the dummy type
            static_assert(std::is_same<EnvCellState, DummyEnvCellState>(),
                          "The cm for association of environment cells cannot "
                          "be the DummyCellManager! \n"
                          "Setup the model with type DummyEnvCellState for "
                          "standalone model or pass a cell manager for "
                          "associated model.");

            this->_log->info("Setting up '{}' as coupled model without "
                             "associated cell manager.", this->_name);
        }
        else {
            this->_log->info("Setting up '{}' as standalone model ...",
                             this->_name);
        }

        // Check inheritance of EnvCellState; needed for position cache
        static_assert(std::is_base_of<BaseEnvCellState, EnvCellState>::value,
                      "The model's EnvCellState must derive from "
                      "Utopia::Models::Environment::BaseEnvCellState!");

        // Check inheritance of EnvParam; needed for getters and setters
        static_assert(std::is_base_of<BaseEnvParam, EnvParam>::value,
                      "The model's EnvParam must derive from "
                      "Utopia::Models::Environment::BaseEnvParam!");

        // Store positions
        apply_rule<Update::sync>(
            [this](const auto& cell){
                auto& state = cell->state;
                state.position = this->_cm.barycenter_of(cell);
                return state;
            },
            this->_cm.cells()
        );
        this->_log->debug("Cell barycenters cached.");


        // Now set up the actual environment parameter functions
        if (this->_cfg["env_param_funcs"]) {
            setup_env_param_funcs(this->_cfg["env_param_funcs"]);
        }

        // Apply the env_param_funcs for initialization
        this->_log->debug("Applying {} initial environment param function{} ...",
                         _env_param_funcs.size(),
                         _env_param_funcs.size() != 1 ? "s" : "");
        for (auto& epfb : _env_param_funcs) {
            if (epfb.invoke_at_initialization) {
                apply_env_param_func(epfb, true);
            }
        }

        // Now set up the actual environment state functions
        if (this->_cfg["init_env_state_funcs"]) {
            setup_env_state_funcs<true>(this->_cfg["init_env_state_funcs"]);
        }
        if (this->_cfg["env_state_funcs"]) {
            setup_env_state_funcs<false>(this->_cfg["env_state_funcs"]);
        }

        // Apply the env_state_funcs for initialization
        this->_log->info("Applying {} initial environment state function{} ...",
                         _init_env_state_funcs.size(),
                         _init_env_state_funcs.size() != 1 ? "s" : "");
        for (auto& esfb : _init_env_state_funcs) {
            apply_env_state_func(esfb);
        }

        this->_log->info("{} set up.", this->_name);
    }

    /// Construct Environment without associated CellManager
    /** \note This constructor can be used to set up a Environment as a
      *       standalone model or with only the EnvParam.
      *       In the case of standalone the ``standalone`` template parameter
      *       needs to be set to ``true``.
      *       In the the other case the EnvCellState needs to be the default
      *       dummy type ``DummyEnvCellState`` and the ``standalone`` template
      *       parameter needs to be ``false`` (default).
      */
    template <typename ParentModel>
    Environment (const std::string name, ParentModel& parent)
    :
        // Use existing constructor, passing a _DummyCellManager instance that
        // ensures that no configuration is carried over.
        Environment(name, parent, DummyCellManager())
    { }


    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Iterate a single step
    void perform_step () {
        for (auto& epfb : _env_param_funcs) {
            apply_env_param_func(epfb);
        }
        for (auto& esfb : _env_state_funcs) {
            apply_env_state_func(esfb);
        }
    }


    /// Monitor model information
    void monitor () {
    }


    /// Write data
    /** For all parameters and states  registered for writing, writes the
     *  values to the corresponding dataset.
     *
     *  \note To register keys, use ::track_parameter and ::track_state method,
     *        respectively
     */
    void write_data () {
        // write parameters
        for (auto& [key, dset] : _dsets_param) {
            dset->write(_params.get_env(key));
        }

        // write states
        for (auto& param_dset_pair : _dsets_state) {
            const auto key = std::get<0>(param_dset_pair);
            auto& dset = std::get<1>(param_dset_pair);

            dset->write(_cm.cells().begin(), _cm.cells().end(),
                [key](const auto& cell) {
                    return cell->state.get_env(key);
                }
            );
        }
    }


    // Getters and setters ....................................................
    /// Return the current value of the parameter with param_name
    double get_parameter(const std::string& param_name) const {
        return this->_params.get_env(param_name);
    }

    /// Return a const reference to the cell manager
    const auto& cm() const {
        return this->_cm;
    }

    // .. Environment Function Bundle Handling ................................
    /// Add a rule function at the end of the sequence of parameter functions
    /** \param epfb      The EnvParamFuncBundle of environment parameter
     *                   function that is to be added and its metadata.
     */
    template<class EPFB>
    void add_env_param_func(EPFB&& epfb) {
        _env_param_funcs.push_back(epfb);
        this->_log->debug("Added environment param function '{}'.", epfb.name);
    }

    /// Add a param function at the end of the sequence of env functions
    /** \param epf      EnvParamFunc that is applied to all cm.cells()
     *  \param times_tuple  invoke_at_initialization, invoke_always,
     *                      set of times when to invoke the function
     */
    void add_env_param_func(const std::string& name, const EnvParamFunc& epf,
            const std::string& param_name,
            std::tuple<bool, bool, std::set<Time>> times_tuple)
    {
        add_env_param_func(
            EnvParamFuncBundle(name, epf, param_name, times_tuple)
        );
    }

    /// Add a param function at the end of the sequence of env functions
    /** \param epf      EnvParamFunc that is applied to all cm.cells()
     *  \param cfg      From which invoke_at_initialization and times to invoke
     *                  the function is extracted
     */
    void add_env_param_func_from_cfg(const std::string& name,
            const EnvParamFunc& epf, const std::string& param_name,
            const Config& cfg = {})
    {
        auto invoke_times_tuple = extract_times_and_initialization<Time>(cfg);
        add_env_param_func(name, epf, param_name, invoke_times_tuple);
    }

    /// Add a rule function at the end of the sequence of state functions
    /** \param esfb  The EnvStateFuncBundle of environment state
     *                   function that is to be added and its metadata.
     */
    template<bool add_to_initial=false, class ESFB>
    void add_env_state_func(ESFB&& esfb) {
        if constexpr (add_to_initial) {
            _init_env_state_funcs.push_back(esfb);
        }
        else {
            _env_state_funcs.push_back(esfb);
        }
        this->_log->debug("Added {}environment function '{}'.",
                          add_to_initial ? "initial " : "", esfb.name);
    }

    /// Add a rule function at the end of the sequence of environment functions
    /** \param esf      EnvStateFunc that is applied to all cm.cells()
     *  \param update   The Update mode to use with apply_rule(esf, cm.cells)
     *  \param times_pair   invoke_always, Set of times at which to invoke
     *                      function.
     *  \param select_cfg   Config passed to _cm.select_cells
     */
    template<bool add_to_initial=false>
    void add_env_state_func(const std::string& name,
            const EnvStateFunc& esf, const Update& update = Update::sync,
            std::pair<bool, std::set<Time>> times_pair = {true, {}},
            Config select_cfg = {})
    {
        // resolve select option
        bool fix_selection;
        EnvCellContainer cell_selection = {};
        if (not select_cfg.IsNull()) {
            std::string generate = get_as<std::string>("generate", select_cfg);
            if (generate == "once") {
                fix_selection = true;
            }
            else if (generate == "always") {
                fix_selection = false;
            }
            else {
                throw std::invalid_argument("Key 'generate' in 'select' "
                        "feature of environment state function '"
                        + name + "' must be 'once' to fix selection or "
                        "'always' to generate it on the fly, but was '"
                        + generate + "'!");
            }

            if (fix_selection) {
                this->_log->debug("Generating a selection of cells and fixing "
                                  "it for the rule '{}'.", name);
                cell_selection = _cm.select_cells(select_cfg);
            }
        }
        else {
            fix_selection = false;
        }

        add_env_state_func<add_to_initial>(
            EnvStateFuncBundle(name, esf, update, add_to_initial, times_pair,
                               {fix_selection, cell_selection, select_cfg})
        );
    }

    /// Add a rule function at the end of the sequence of environment functions
    /** \param esf      EnvStateFunc that is applied to all cm.cells()
     *  \param update   The Update mode to use with apply_rule(esf, cm.cells)
     *  \param cfg      The config of the state_func. Here, times and select are
     *                  extracted; All other entries ignored.
     *                  Defaults are invoke always and select all cells.
     */
    template<bool add_to_initial=false>
    void add_env_state_func_from_cfg(
        const std::string& name,
        const EnvStateFunc& esf,
        const Update& update = Update::sync,
        const Config& cfg = {})
    {
        auto times_pair = extract_times<Time>(cfg);
        Config select_cfg = {};
        if (cfg.IsMap()) {
            select_cfg = get_as<Config>("select", cfg, {});
        }

        add_env_state_func<add_to_initial>(name, esf, update, times_pair,
                                           select_cfg);
    }

    /// Mark a parameter as being tracked, i.e. store its data in write_data
    void track_parameter(const std::string& key) {
        if (_dsets_param.find(key) != _dsets_param.end()) {
            throw std::invalid_argument("Parameter '" + key + "' is already "
                                        "being tracked!");
        }
        _dsets_param.insert({key, this->create_dset(key, {})});
    }

    /// Track multiple parameters
    /** \details Invokes ::track_state for each entry
      */
    void track_parameters(const std::vector<std::string>& keys) {
        for (const auto& key : keys) {
            track_parameter(key);
        }
    }

    /// Mark a state as being tracked, i.e. store its data in write_data
    void track_state(const std::string& key) {
        if (_dsets_state.find(key) != _dsets_state.end()) {
            throw std::invalid_argument("State '" + key + "' is already "
                                        "being tracked!");
        }
        _dsets_state.insert({key, this->create_cm_dset(key, _cm)});
    }

    /// Track multiple states
    /** \details Invokes ::track_state for each entry
      */
    void track_states(const std::vector<std::string>& keys) {
        for (const auto& key : keys) {
            track_state(key);
        }
    }

private:
    /// Construct the rule funcs sequencefor EnvParam from cfg
    void setup_env_param_funcs(const Config& cfg) {
        this->_log->info("Setting up environment param function sequence "
                         "from {} configuration entr{} ...",
                         cfg.size(), cfg.size() != 1 ? "ies" : "y");

        // For zombie or empty configurations, return empty container
        if (not cfg or not cfg.size()) {
            return;
        }
        // Otherwise, require a sequence
        if (not cfg.IsSequence()) {
            throw std::invalid_argument("The config for initializing the "
                "environment functions must be a sequence!");
        }

        // Iterate over the sequence of mappings
        for (const auto& epfs : cfg) {
            // epfs.IsMap() == true
            // The top `epfs` keys are now the names of the desired environment
            // functions. Iterate over those ...
            for (const auto& epf_pair : epfs) {
                // epf_pair is a pair of (key node, value node)
                // Find out the name of the rule function
                const auto epf_name = epf_pair.first.as<std::string>();

                this->_log->trace("  Function name:  {}", epf_name);

                // Now iterate over the (param name, param cfg) pairs
                for (const auto& kv_pair : epf_pair.second) {
                    // Get the parameter name and configuration
                    const auto param_name = kv_pair.first.as<std::string>();
                    const auto& epf_cfg = kv_pair.second;

                    // Distinguish by name of rule function
                    if (epf_name == "increment") {
                        auto epf = epf_increment(*this, param_name, epf_cfg);
                        add_env_param_func_from_cfg(
                            epf_name+"."+param_name, epf, param_name, epf_cfg
                        );
                    }
                    else if (epf_name == "random") {
                        auto epf = epf_random(*this, param_name, epf_cfg);
                        add_env_param_func_from_cfg(
                            epf_name+"."+param_name, epf, param_name, epf_cfg
                        );
                    }
                    else if (epf_name == "rectangular") {
                        auto epf = epf_rectangular(*this, epf_cfg);
                        add_env_param_func_from_cfg(
                            epf_name+"."+param_name, epf, param_name, epf_cfg
                        );
                    }
                    else if (epf_name == "set") {
                        auto epf = epf_set(*this, epf_cfg);
                        add_env_param_func_from_cfg(
                            epf_name+"."+param_name, epf, param_name, epf_cfg
                        );
                    }
                    else if (epf_name == "sinusoidal") {
                        auto epf = epf_sinusoidal(*this, epf_cfg);
                        add_env_param_func_from_cfg(
                            epf_name+"."+param_name, epf, param_name, epf_cfg
                        );
                    }
                    // .. can add more rule functions here (alphabetic order).
                    //    Add also in invalid_argument message below ..
                    else if (epf_name != "void") {
                        throw std::invalid_argument("No environment parameter "
                            "function '" + epf_name + "' available to "
                            "construct! Choose from: increment, random, "
                            "rectangular, set, sinusoidal.");
                    }

                    this->_log->debug("Added '{}' environment parameter "
                        "function for parameter '{}'.", epf_name, param_name);
                }
            }
        }
    };

    /// Construct the rule funcs sequence from cfg
    template<bool add_to_initial=false>
    void setup_env_state_funcs(const Config& cfg) {
        this->_log->info("Setting up {}environment state function sequence "
                         "from {} configuration entr{} ...",
                         add_to_initial ? "initial " : "",
                         cfg.size(), cfg.size() != 1 ? "ies" : "y");

        // For zombie or empty configurations, return empty container
        if (not cfg or not cfg.size()) {
            return;
        }
        // Otherwise, require a sequence
        if (not cfg.IsSequence()) {
            throw std::invalid_argument("The config for initializing the "
                "environment state functions must be a sequence!");
        }

        // Iterate over the sequence of mappings
        for (const auto& esfs : cfg) {
            // esfs.IsMap() == true
            // The top `esfs` keys are now the names of the desired environment
            // functions. Iterate over those ...
            for (const auto& esf_pair : esfs) {
                // esf_pair is a pair of (key node, value node)
                // Find out the name of the rule function
                const auto esf_name = esf_pair.first.as<std::string>();

                this->_log->trace("  Function name:  {}", esf_name);

                // Now iterate over the (param name, param cfg) pairs
                for (const auto& kv_pair : esf_pair.second) {
                    // Get the parameter name and configuration
                    const auto param_name = kv_pair.first.as<std::string>();
                    const auto& esf_cfg = kv_pair.second;

                    // Distinguish by name of rule function
                    if (esf_name == "noise") {
                        auto esf_update_pair = esf_noise(*this, param_name,
                                                         esf_cfg);
                        add_env_state_func_from_cfg<add_to_initial>(
                            "noise."+param_name, esf_update_pair.first,
                            esf_update_pair.second, esf_cfg
                        );
                    }
                    else if (esf_name == "slope") {
                        auto esf_update_pair = esf_slope(*this, param_name,
                                                         esf_cfg,
                                                         _cm.space()->extent);
                        add_env_state_func_from_cfg<add_to_initial>(
                            "slope."+param_name, esf_update_pair.first,
                            esf_update_pair.second, esf_cfg
                        );
                    }
                    else if (esf_name == "steps") {
                        auto esf_update_pair = esf_steps(*this, param_name,
                                                         esf_cfg);
                        add_env_state_func_from_cfg<add_to_initial>(
                            "steps."+param_name, esf_update_pair.first,
                            esf_update_pair.second, esf_cfg
                        );
                    }
                    else if (esf_name == "uniform") {
                        auto esf_update_pair = esf_uniform(*this, param_name,
                                                           esf_cfg);
                        add_env_state_func_from_cfg<add_to_initial>(
                            "uniform."+param_name, esf_update_pair.first,
                            esf_update_pair.second, esf_cfg
                        );
                    }
                    // .. can add more rule functions here ..
                    else if (esf_name != "void") {
                        throw std::invalid_argument("No environment state "
                            "function '" + esf_name + "' available to "
                            "construct! Choose from: noise, slope, steps, "
                            "uniform.");
                    }

                    this->_log->trace("Added '{}' environment state"
                        "function for parameter '{}'.", esf_name, param_name);
                }
            }
        }
    };


    /// Apply a given environment parameter function
    /** \param esf   EnvParamFunc that is applied */
    template<class EPFB>
    void apply_env_param_func(EPFB&& epfb, bool initialization = false) {
        // Check whether to invoke
        if (initialization) {
            if (not epfb.invoke_at_initialization) {
                this->_log->trace("Not invoking environment function '{}' at "
                                  "initialization.", epfb.name);
                return;
            }
        }
        else if (not epfb.invoke_always) {
            // Compare to first element of the times set
            // NOTE This approach has a low and constant complexity as no tree
            //      traversal in the set takes place. This, however, relies on
            //      the ordering of the set and that the first element is
            //      never smaller than (current time + 1), which would lead to
            //      clogging of the erasure ...
            if (    epfb.times.size()
                and *epfb.times.begin() == (this->_time + 1))
            {
                // Invoke at this time; pop element corresponding to this time
                epfb.times.erase(epfb.times.begin());
            }
            else {
                this->_log->trace("Not invoking environment function '{}' in "
                                  "this iteration.", epfb.name);
                return;
            }
        }

        this->_log->debug("Applying environment parameter function '{}' ...",
                          epfb.name);
        this->_params.set_env(epfb.param_name, epfb.func());
    }

    /// Apply a given environment state function
    /** \param esf   EnvStateFunc that is applied to _cm.cells() */
    template<class ESFB>
    void apply_env_state_func(ESFB&& esfb) {
        // Check whether to invoke
        if (not esfb.invoke_always) {
            // Compare to first element of the times set
            // NOTE This approach has a low and constant complexity as no tree
            //      traversal in the set takes place. This, however, relies on
            //      the ordering of the set and that the first element is
            //      never smaller than (current time + 1), which would lead to
            //      clogging of the erasure ...
            if (    esfb.times.size()
                and *esfb.times.begin() == (this->_time + 1))
            {
                // Invoke at this time; pop element corresponding to this time
                esfb.times.erase(esfb.times.begin());
            }
            else {
                this->_log->trace("Not invoking environment function '{}' in "
                                  "this iteration.", esfb.name);
                return;
            }
        }

        this->_log->debug("Applying environment state function '{}' ...",
                          esfb.name);

        // get EnvCellContainer over which to apply the function
        EnvCellContainer cell_selection = {};
        if (esfb.fix_selection) {
            cell_selection = esfb.cell_selection;
        }
        else if (not esfb.select_cfg.IsNull()) {
            this->_log->debug("Generating a new selection of cells for "
                              "environment state function '{}' ...", esfb.name);
            cell_selection = _cm.select_cells(esfb.select_cfg);
        }
        else {
            cell_selection = _cm.cells();
        }

        this->_log->debug("Applying to {} cells ...",
                          (cell_selection.size() == _cm.cells().size() ?
                           "all" : std::to_string(cell_selection.size())));

        // Need to distinguish by update mode
        if (esfb.update == Update::sync) {
            apply_rule<Update::sync>(esfb.func, cell_selection);
        }
        else if (esfb.update == Update::async) {
            apply_rule<Update::async>(esfb.func, cell_selection, *this->_rng);
        }
        else {
            // Throw in case the Update enum gets extended and an unexpected
            // value is passed ...
            throw std::invalid_argument("Unsupported `update` argument!");
        }
    }

    // .. Helper functions ....................................................
};

// End group Environment
/**
 *  \}
 */

} // namespace Environment
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_ENVIRONMENT_HH
