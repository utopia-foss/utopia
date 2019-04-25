#ifndef UTOPIA_MODELS_ENVIRONMENT_HH
#define UTOPIA_MODELS_ENVIRONMENT_HH
// TODO Adjust above include guard (and at bottom of file)

// standard library includes
#include <random>
#include <unordered_map>

// third-party library includes

// Utopia-related includes
#include <utopia/core/model.hh>
#include <utopia/core/apply.hh>
#include <utopia/core/types.hh>
#include <utopia/core/cell_manager.hh>


namespace Utopia {
namespace Models {
namespace Environment {

// ++ Type definitions ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// Base type for environment cell states
/** \detail This class is meant to be derived from and used as a basis for the
  *         desired kind of environment.
  */
struct BaseEnvCellState {
    using SpaceVec = Utopia::SpaceVecType<2>;

    /// Cached barycenter of the cell
    SpaceVec position;

    virtual ~BaseEnvCellState() = default;

    /// Get an environment variable
    virtual double get_env(const std::string&) const = 0;

    /// Set an environment variable
    virtual void set_env(const std::string&, const double&) = 0;
};


/// Type helper to define types used by the model
using ModelTypes = Utopia::ModelTypes<>;


// ++ Model definition ++++++++++++++++++++++++++++++++++++++++++++++++++++++++

/// The Environment Model
/** \detail A model for a non-uniform parameter background, coupled to another
 *          model's CellManager.
 * 
 * \tparam EnvCellState   The cell state type of the environment cells
 * \tparam associate      Whether to associate with the CellManager of the
 *                        parent model. Use ``false`` for running this model
 *                        as a standalone model and ``true`` when desiring
 *                        cells from another model being linked to the cells of
 *                        the Environment model
 */
template<typename EnvCellState, bool associate=true>
class Environment:
    public Model<Environment<EnvCellState, associate>, ModelTypes>
{
public:
    /// Type of this class
    using Self = Environment<EnvCellState, associate>;

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

    /// The type of the environment functions; basically a rule function
    using EnvFunc = typename CellManager::RuleFunc;

    /// The type of the model time
    using Time = typename Base::Time;

    /// Configuration node type alias
    using Config = DataIO::Config;

    /// The environment function bundle
    /** \detail This gathers the environment function alongside some metadata
      *         into a custom construct.
     */
    struct EnvFuncBundle {
        std::string name;
        EnvFunc func;
        Update update;
        bool invoke_always;
        std::set<Time> times;

        EnvFuncBundle(std::string name,
                      EnvFunc func,
                      Update update = Update::sync,
                      bool invoke_always = true,
                      std::set<Time> times = {})
        :
            name(name),
            func(func),
            update(update),
            invoke_always(invoke_always),
            times(times)
        {}

        EnvFuncBundle(std::string name,
                      EnvFunc func,
                      Update update = Update::sync,
                      std::pair<bool, std::set<Time>> times_pair = {true, {}})
        :
            EnvFuncBundle(name, func, update,
                          times_pair.first, times_pair.second)
        {}
    };

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

    /// Container of rule functions that are invoked once at initialisation
    std::vector<EnvFuncBundle> _init_env_funcs;

    /// Container of rule functions that are invoked every time step
    std::vector<EnvFuncBundle> _env_funcs;

    // .. Datasets ............................................................
    /// Dynamically generated map of datasets
    std::unordered_map<std::string, std::shared_ptr<DataSet>> _dsets;


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

        // Set up environment function bundle containers empty
        _init_env_funcs{},
        _env_funcs{},

        // Map of datasets; empty until parameters are set to be tracked
        _dsets{}
    {
        // Associate the CellManager's cells with each other
        if constexpr (associate) {
            for (std::size_t i=0; i < _cm.cells().size(); i++) {
                associate_cm.cells()[i]->custom_links().env = _cm.cells()[i];
            }
                
            this->_log->info("Associated '{}' cells with those of the parent "
                             "model '{}'.", this->_name, parent.get_name());
        }
        else {
            // Only allow standalone mode when a DummyCellManager is passed
            static_assert(std::is_same<CellManager, DummyCellManager>(),
                "Do not pass a CellManager when desiring to instantiate the "
                "Environment model in standalone mode!");
            // NOTE Can consider to remove this restriction. Leaving it here
            //      now for safety ...

            this->_log->info("Setting up '{}' as standalone model ...",
                             this->_name);
        }

        // Check inheritance of EnvCellState; needed for position cache
        static_assert(std::is_base_of<BaseEnvCellState, EnvCellState>::value,
                      "The model's EnvCellState must derive from "
                      "Utopia::Models::Environment::BaseEnvCellState!");

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

        // Now set up the actual environment functions
        if (this->_cfg["init_env_funcs"]) {
            setup_env_funcs<true>(this->_cfg["init_env_funcs"]);
        }
        if (this->_cfg["env_funcs"]) {
            setup_env_funcs<false>(this->_cfg["env_funcs"]);
        }

        // Apply the env_funcs for initialization
        this->_log->info("Applying {} initial environment function{} ...",
                         _init_env_funcs.size(),
                         _init_env_funcs.size() != 1 ? "s" : "");
        for (auto& efb : _init_env_funcs) {
            apply_env_func(efb);
        }

        this->_log->info("{} set up.", this->_name);
    }

    /// Construct Environment without associated CellManager
    /** \note This constructor can be used to set up a Environment as a
      *       standalone model. The ``associate`` template parameter needs to
      *       be set to ``false`` for that.
      */
    template<class ParentModel,
             class = typename std::enable_if_t<not associate, ParentModel>>
    Environment (const std::string name, ParentModel& parent)
    :
        // Use existing constructor, passing a _DummyCellManager instance that
        // ensures that no configuration is carried over.
        Environment(name, parent, DummyCellManager())
    {}


    // -- Public Interface ----------------------------------------------------
    // .. Simulation Control ..................................................

    /// Iterate a single step
    void perform_step () {
        for (auto& efb : _env_funcs) {
            apply_env_func(efb);
        }
    }


    /// Monitor model information
    void monitor () {
    }


    /// Write data
    /** For all parameters registered for writing, writes the parameter values
     *  to the corresponding dataset.
     * 
     *  \note To register keys, use ::track_parameter method
     */
    void write_data () {
        for (auto& param_dset_pair : _dsets) {
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
    /// Add a rule function at the end of the sequence of environment functions
    /** \param ef      EnvFunc that is applied to all cm.cells()
     *  \param update  The Update mode to use with apply_rule(ef, cm.cells)
     */
    template<bool add_to_initial=false>
    void add_env_func(const std::string& name,
                      const EnvFunc& ef,
                      const Update& update = Update::sync,
                      const std::set<Time>& times = {})
    {
        add_env_func<add_to_initial>(
            EnvFuncBundle(name, ef, update, bool(times.size()), times)
        );
    }

    /// Mark a parameter as being tracked, i.e. store its data in write_data
    void track_parameter(const std::string& key) {
        if (_dsets.find(key) != _dsets.end()) {
            throw std::invalid_argument("Parameter '" + key + "' is already "
                                        "being tracked!");
        }
        _dsets.insert({key, this->create_cm_dset(key, _cm)});
    }

    /// Track multiple parameters
    /** \detail Invokes ::track_parameter for each entry
      */
    void track_parameters(const std::vector<std::string>& keys) {
        for (const auto& key : keys) {
            track_parameter(key); 
        }
    }

private:
    // .. Environment Function Bundle Handling ................................
    /// Add a rule function at the end of the sequence of environment functions
    /** \param ef_pair  The pair of rule function and update mode that is to
     *                  be added as an Environment function
     */
    template<bool add_to_initial=false, class EFB>
    void add_env_func(EFB&& efb) {
        if constexpr (add_to_initial) {
            _init_env_funcs.push_back(efb);
        }
        else {
            _env_funcs.push_back(efb);
        }
        this->_log->debug("Added {}environment function '{}'.",
                          add_to_initial ? "initial " : "", efb.name);
    }

    /// Construct the rule funcs sequence from cfg
    template<bool add_to_initial=false>
    void setup_env_funcs(const Config& cfg) {
        this->_log->info("Setting up {}environment function sequence from "
                         "{} configuration entr{} ...",
                         add_to_initial ? "initial " : "",
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
        for (const auto& efs : cfg) {
            // efs.IsMap() == true
            // The top `efs` keys are now the names of the desired environment
            // functions. Iterate over those ...
            for (const auto& ef_pair : efs) {
                // ef_pair is a pair of (key node, value node)
                // Find out the name of the rule function
                const auto ef_name = ef_pair.first.as<std::string>();

                this->_log->trace("  Function name:  {}", ef_name);

                // Now iterate over the (param name, param cfg) pairs
                for (const auto& kv_pair : ef_pair.second) {
                    // Get the parameter name and configuration
                    const auto param_name = kv_pair.first.as<std::string>();
                    const auto& ef_cfg = kv_pair.second;
                    
                    this->_log->trace("  Parameter:      {}", param_name);

                    // Distinguish by name of rule function
                    if (ef_name == "noise") {
                        add_env_func<add_to_initial>(
                            _ef_noise(param_name, ef_cfg)
                        );
                    }
                    else if (ef_name == "slope") {
                        add_env_func<add_to_initial>(
                            _ef_slope(param_name, ef_cfg)
                        );
                    }
                    else if (ef_name == "steps") {
                        add_env_func<add_to_initial>(
                            _ef_steps(param_name, ef_cfg)
                        );
                    }
                    else if (ef_name == "uniform") {
                        add_env_func<add_to_initial>(
                            _ef_uniform(param_name, ef_cfg)
                        );
                    }
                    // .. can add more rule functions here ..
                    else if (ef_name != "void") {
                        throw std::invalid_argument("No environment function '"
                            + ef_name + "' available to construct! Choose "
                            "from: noise, slope, steps, uniform.");
                    }
                }
            }
        }
    };


    /// Apply a given environment function
    /** \param ef   EnvFunc that is applied to _cm.cells()
     *  \param update   The Update mode to use with apply_rule(ef, cm.cells)
     */
    template<class EFB>
    void apply_env_func(EFB&& efb) {
        // Check whether to invoke
        if (not efb.invoke_always) {
            // Compare to first element of the times set
            // NOTE This approach has a low and constant complexity as no tree
            //      traversal in the set takes place. This, however, relies on
            //      the ordering of the set and that the first element is
            //      never smaller than (current time + 1), which would lead to
            //      clogging of the erasure ...
            if (efb.times.size() and *efb.times.begin() == (this->_time + 1)) {
                // Invoke at this time; pop element corresponding to this time
                efb.times.erase(efb.times.begin());
            }
            else {
                this->_log->trace("Not invoking environment function '{}' in "
                                  "this iteration.", efb.name);
                return;
            }
        }

        this->_log->debug("Applying environment function '{}' ...", efb.name);

        if (efb.update == Update::sync) {
            apply_rule<Update::sync>(efb.func, _cm.cells());
        }
        else if (efb.update == Update::async) {
            apply_rule<Update::async>(efb.func, _cm.cells(), *this->_rng);
        }
        else {
            // Throw in case the Update enum gets extended and an unexpected
            // value is passed ...
            throw std::invalid_argument("Unsupported `update` argument!");
        }
    }

    // .. Helper functions ....................................................

    /// Value calculation mode
    enum class ValMode {Set, Add};

    /// Given a configuration node, extract the value mode
    ValMode extract_val_mode(const Config& cfg,
                             const std::string& context) const
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
    /** \note If ``times`` was empty, only adds a single element, which is the
     *        numeric limit of Time, denoting that it is to be invoked at every
     *        time step.
     */
    std::pair<bool, std::set<Time>> extract_times(const Config& cfg) const {
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
            // NOTE 0 may not be included because 
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

    /// Create a rule function that uses a random number distribution
    /** \detail This constructs a mutable ``EnvFunc`` lambda, moving the
      *         ``dist`` into the capture.
      */
    template<class DistType>
    EnvFunc build_rng_env_func(DistType&& dist,
                               const std::string& param_name,
                               const ValMode& mode) const
    {
        // NOTE It is VITAL to move-construct the perfectly-forwarded dist into
        //      the lambda; otherwise it has to be stored outside, which is a
        //      real pita. Also, the lambda has to be declared mutable such
        //      that the captured object are allowed to be changed; again, this
        //      is only relevant for the distribution's internal state ...
        return
            [this, param_name, mode, dist{std::move(dist)}]
            (const auto& env_cell) mutable {
                auto& env_state = env_cell->state;

                double current_value = 0.;
                if (mode == ValMode::Add) {
                    current_value = env_state.get_env(param_name);
                }
                const double rn = dist(*this->_rng);

                env_state.set_env(param_name, current_value + rn);
                return env_state;
            };
    }


    // -- Environment modification functions ----------------------------------
    // .. Keep these in alphabetical order and prefix with _ef_ ! .............
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
    EnvFuncBundle _ef_noise(const std::string& param_name,
                            const Config& cfg) const
    {
        this->_log->debug("Constructing 'noise' environment function for "
                          "parameter '{}' ...", param_name);

        // Extract parameters
        const auto efb_name = "noise." + param_name;
        auto times_pair = extract_times(cfg);
        const auto mode = extract_val_mode(cfg, "noise");
        const auto distribution = get_as<std::string>("distribution", cfg);

        // Depending on chosen distribution, construct it and build a rule
        // function using a reference to the newly created one...
        if (distribution == "normal") {
            const auto mean = get_as<double>("mean", cfg);
            const auto stddev = get_as<double>("stddev", cfg);
            std::normal_distribution<> dist(mean, stddev);

            auto ef = build_rng_env_func(std::move(dist), param_name, mode);
            return {efb_name, ef, Update::sync, times_pair};
        }
        else if (distribution == "poisson") {
            const auto mean = get_as<double>("mean", cfg);
            std::poisson_distribution<> dist(mean);
            
            auto ef = build_rng_env_func(std::move(dist), param_name, mode);
            return {efb_name, ef, Update::sync, times_pair};
        }
        else if (distribution == "exponential") {
            const auto lambda = get_as<double>("lambda", cfg);
            std::exponential_distribution<> dist(lambda);
            
            auto ef = build_rng_env_func(std::move(dist), param_name, mode);
            return {efb_name, ef, Update::sync, times_pair};
        }
        else if (distribution == "uniform_int") {
            auto interval = get_as<std::array<int, 2>>("interval", cfg);
            std::uniform_int_distribution<> dist(interval[0], interval[1]);
            
            auto ef = build_rng_env_func(std::move(dist), param_name, mode);
            return {efb_name, ef, Update::sync, times_pair};
        }
        else if (distribution == "uniform_real" or distribution == "uniform") {
            auto interval = get_as<std::array<double, 2>>("interval", cfg);
            std::uniform_real_distribution<> dist(interval[0], interval[1]);

            auto ef = build_rng_env_func(std::move(dist), param_name, mode);
            return {efb_name, ef, Update::sync, times_pair};
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
     */ 
    EnvFuncBundle _ef_slope(const std::string& param_name,
                            const Config& cfg) const
    {
        this->_log->debug("Constructing 'slope' environment function for "
                          "parameter '{}' ...", param_name);

        const auto efb_name = "slope." + param_name;
        auto times_pair = extract_times(cfg);
        const auto mode = extract_val_mode(cfg, "slope");

        const auto values_north_south =
            get_as<std::array<double, 2>>("values_north_south", cfg);

        EnvFunc ef =
            [this, param_name, mode, values_north_south]
            (const auto& env_cell) mutable
        {
            auto& env_state = env_cell->state;

            // Use the relative position along y-dimension
            const double pos = (  env_state.position[1]
                                / this->_cm.space()->extent[1]);
            const double slope = values_north_south[0] - values_north_south[1];
            const double value = values_north_south[1] + pos * slope;

            double current_value = 0.;
            if (mode == ValMode::Add) {
                current_value = env_state.get_env(param_name);
            }
            env_state.set_env(param_name, current_value + value);
            return env_state;
        };
        return EnvFuncBundle(efb_name, ef, Update::sync, times_pair);
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
    EnvFuncBundle _ef_steps(const std::string& param_name,
                            const Config& cfg) const
    {
        this->_log->debug("Constructing 'steps' environment function for "
                          "parameter '{}' ...", param_name);

        const auto efb_name = "steps." + param_name;
        auto times_pair = extract_times(cfg);
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

        EnvFunc ef = 
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

        this->_log->debug("Constructed 'steps' environment function for "
                          "parameter '{}'.", param_name);
        return EnvFuncBundle(efb_name, ef, Update::sync, times_pair);
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
    EnvFuncBundle _ef_uniform(const std::string& param_name,
                              const Config& cfg) const
    {
        this->_log->debug("Constructing 'uniform' environment function "
                          "for parameter '{}' ...", param_name);

        const auto efb_name = "uniform." + param_name;
        auto times_pair = extract_times(cfg);
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

        EnvFunc ef =
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
        return EnvFuncBundle(efb_name, ef, Update::sync, times_pair);
    };
};

} // namespace Environment
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_ENVIRONMENT_HH
