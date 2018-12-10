#ifndef UTOPIA_MODEL_HH
#define UTOPIA_MODEL_HH

#include <dune/utopia/data_io/hdffile.hh>
#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/cfg_utils.hh>
#include <dune/utopia/data_io/monitor.hh>
#include <dune/utopia/core/logging.hh>


namespace Utopia {

/// Wrapper struct for defining base class data types
/** \tparam DataType              Type of the data the model operates on
 *  \tparam BoundaryConditionType Data type of the boundary condition. If not
 *                                set, an empty struct is used as default.
 *  \tparam RNGType               The RNG class to use
 *  \tparam ConfigType            The class to use for reading the config
 *  \tparam DataGroupType         The data group class to store datasets in
 *  \tparam DataSetType           The data group class to store data in
 *  \tparam TimeType              The type to use for the time members
 *  \tparam MonitorType           The type to use for the Monitor member
 */
template<typename RNGType=DefaultRNG,
         typename ConfigType=Utopia::DataIO::Config,
         typename DataGroupType=Utopia::DataIO::HDFGroup,
         typename DataSetType=Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>,
         typename TimeType=std::size_t,
         typename MonitorType=Utopia::DataIO::Monitor,
         typename MonitorManagerType=Utopia::DataIO::MonitorManager
         >
struct ModelTypes
{
    using RNG = RNGType;
    using Config = ConfigType;
    using DataGroup = DataGroupType;
    using DataSet = DataSetType;
    using Time = TimeType;
    using Monitor = MonitorType;
    using MonitorManager = MonitorManagerType;
    using Level = std::size_t;
};


/// Base class interface for Models using the CRT Pattern
/** \tparam Derived Type of the derived model class
 *  \tparam ModelTypes Convenience wrapper for extracting model data types
 */
template<class Derived, typename ModelTypes>
class Model
{
public:
    // -- Data types uses throughout the model class -- //
    // NOTE: these are also available to derived classes
    
    /// Data type that holds the configuration
    using Config = typename ModelTypes::Config;
    
    /// Data type that is used for storing datasets
    using DataGroup = typename ModelTypes::DataGroup;
    
    /// Data type that is used for storing data
    using DataSet = typename ModelTypes::DataSet;

    /// Data type of the shared RNG
    using RNG = typename ModelTypes::RNG;

    /// Data type for the model time
    using Time = typename ModelTypes::Time;

    /// Data type for the monitor
    using Monitor = typename ModelTypes::Monitor;

    /// Data type for the monitor manager
    using MonitorManager = typename ModelTypes::MonitorManager;

    /// Data type for the hierarchical level
    using Level = typename ModelTypes::Level;

protected:
    // -- Member declarations -- //
    /// Model-internal current time stamp
    Time _time;

    /// Model-internal maximum time stamp
    const Time _time_max;
    
    /// Name of the model instance
    const std::string _name;

    /// Config node belonging to this model instance
    const Config _cfg;

    /// The HDF group this model instance should write its data to
    const std::shared_ptr<DataGroup> _hdfgrp;

    /// How often to call write_data from iterate
    const Time _write_every;

    /// The RNG shared between models
    const std::shared_ptr<RNG> _rng;

    /// The (model) logger
    const std::shared_ptr<spdlog::logger> _log;

    /// The monitor
    Monitor _monitor;

    /// The hierarchical level
    const Level _level;

public:
    // -- Constructor -- //

    /// Constructor
    /** \detail Uses information from a parent model to create an instance of
     *          this model.
     *
     *  \tparam ParentModel The parent model's type
     *
     *  \param name         The name of this model instance, ideally used only
     *                      once on the current hierarchical level
     *  \param parent_model The parent model object from which the
     *                      corresponding config node, the group, the RNG,
     *                      and the parent log level are extracted.
     */
    template<class ParentModel>
    Model (const std::string name,
           const ParentModel &parent_model)
    :
        _time(0),
        _time_max(parent_model.get_time_max()),
        _name(name),
        //extract the other information from the parent model object
        _cfg(parent_model.get_cfg()[_name]),
        _hdfgrp(parent_model.get_hdfgrp()->open_group(_name)),
        _write_every(this->determine_write_every(parent_model)),
        _rng(parent_model.get_rng()),
        _log(spdlog::stdout_color_mt(parent_model.get_logger()->name() + "."
                                     + _name)),
        _monitor(Monitor(name, parent_model.get_monitor_manager())),
        _level(parent_model.get_level() + 1)
    {
        // Set this model instance's log level
        if (_cfg["log_level"]) {
            // Via value given in configuration
            const auto lvl = as_str(_cfg["log_level"]);
            _log->debug("Setting log level to '{}' ...", lvl);
            _log->set_level(spdlog::level::from_str(lvl));
        }
        else {
            // No config value given; use the level of the parent's logger
            _log->set_level(parent_model.get_logger()->level());
        }

        // Store write_every value in the hdfgrp
        _hdfgrp->add_attribute("write_every", _write_every);

        // Provide some informative log messages
        _log->info("Model base constructor finished.");
        _log->debug("time_max:     {} step(s)", _time_max);
        _log->debug("write_every:  {} step(s)", _write_every);
    }


    // -- Getters -- //

    /// Return the current time of this model
    Time get_time() const {
        return _time;
    }

    /// Return the maximum time possible for this model
    Time get_time_max() const {
        return _time_max;
    }

    /// Return the config node of this model
    Config get_cfg() const {
        return _cfg;
    }
    
    /// Return a pointer to the HDF group this model stores data in
    std::shared_ptr<DataGroup> get_hdfgrp() const {
        return _hdfgrp;
    }
    
    /// Return the parameter that controls how often write_data is called
    Time get_write_every() const {
        return _write_every;
    }
    
    /// Return a pointer to the shared RNG
    std::shared_ptr<RNG> get_rng() const {
        return _rng;
    }

    /// Return a pointer to the logger of this model
    std::shared_ptr<spdlog::logger> get_logger() const {
        return _log;
    }

    /// Return the monitor of this model
    Monitor get_monitor() const {
        return _monitor;
    }

    /// Get the monitor manager of the root model
    std::shared_ptr<MonitorManager> get_monitor_manager() const {
        return _monitor.get_monitor_manager();
    }

    /// Return the hierarchical level within the model hierarchy
    Level get_level() const {
        return _level;
    }

    // -- Convenient functions -- //

    /// Create and setup a new HDFDataset object
    /** @brief Create a HDFDataset object within a HDFGroup. The capacity
     *         - the shape of the dataset - is calculated automatically from
     *         the num_steps and write_every parameter.
     * 
     * @param name The name of the dataset
     * @param hdfgrp The parent HDFGroup
     * @param add_shape_dims The shape dimensions which together with the number
     *                       of write steps equals the capacity of the dataset
     *                       (capacity = (num_writesteps, add_shape_dims)).
     * @param with_initial_write Account for the initial write step in the capacity.
     * @param chunksize The chunk size
     * @param compression_level The compression level
     * @return std::shared_ptr<DataSet> The hdf dataset
     */
    std::shared_ptr<DataSet> create_dset(const std::string name,
                                         const std::shared_ptr<DataGroup>& hdfgrp,
                                         std::vector<hsize_t> add_shape_dims,
                                         const bool with_initial_write = true,
                                         const std::vector<hsize_t> chunksize={},
                                         const std::size_t compression_level=1){
        // Calculate the number of time steps to be written
        hsize_t num_steps = this->get_time_max() / this->get_write_every();
        if (with_initial_write){
            ++num_steps;
        }

        // Calculate the shape of the dataset
        add_shape_dims.insert(add_shape_dims.begin(), num_steps);
        auto capacity = add_shape_dims;

        // Create the dataset and return it.
        return hdfgrp->open_dataset(name, capacity, chunksize, compression_level);
    }

    /// Create and setup a new HDFDataset object
    /** @brief Create a HDFDataset object within the _hdfgrp class member. 
     *         The capacity - the shape of the dataset - is calculated 
     *         automatically from the num_steps and write_every parameter.
     * 
     * @param name The name of the dataset
     * @param add_shape_dims The shape dimensions which together with the number
     *                       of write steps equals the capacity of the dataset
     *                       (capacity = (num_writesteps, add_shape_dims)).
     * @param with_initial_write Account for the initial write step in the capacity.
     * @param chunksize The chunk size
     * @param compression_level The compression level
     * @return std::shared_ptr<DataSet> The hdf dataset
     */    std::shared_ptr<DataSet> create_dset(const std::string name,
                                         const std::vector<hsize_t> add_shape_dims,
                                         const bool with_initial_write = true,
                                         const std::vector<hsize_t> chunksize={},
                                         const std::size_t compression_level=1){
        // Forward to the create_dset function that requires a parent hdf group
        return create_dset(name, 
                           _hdfgrp, 
                           add_shape_dims,
                           with_initial_write, 
                           chunksize, 
                           compression_level);
    }

    // -- Default implementations -- //

    /// Iterate one (time) step of this model
    /** Increment time, perform step, emit monitor data, and write data.
     *  Monitoring is performed differently depending on the model level. Also,
     *  the write_data function may be called only every `write_every` steps.
     */
    void iterate () {
        // -- Perform the simulation step -- //
        __perform_step();     
        increment_time();

        // -- Monitoring -- //
        /* If the model is at the first hierarchical level, check whether the
         * monitor entries should be collected and emitted. This leads to a
         * flag being set in the monitor manager, such that the submodels do
         * not have to do the check against the timer as well and that all
         * collected data stems from the same time step.
         */ 
        if (_level == 1) {
            _monitor.get_monitor_manager()->check_timer();
            __monitor();
            
            // If enabled for this step, perform the emission of monitor data
            // NOTE At this point, we can be sure that all submodels have
            //      already run, because their iterate functions were called
            //      in the perform_step of the level 1 model.
            _monitor.get_monitor_manager()->emit_if_enabled();
        }
        else {
            __monitor();
        }

        // -- Data output -- //
        if (_time % _write_every == 0) {
            _log->debug("Calling write_data ...");
            __write_data();
        }

        _log->debug("Finished iteration: {:9d} / {:d}", _time, _time_max);
    }

    /// Run the model from the current time to the maximum time
    /** @detail This repeatedly calls the iterate method until the maximum time
      *         is reached.
      */
    void run () {
        _log->info("Running from current time  {}  to  {}  ...",
                   _time, _time_max);

        while (_time < _time_max) {
            iterate();
        }
        _log->info("Run finished. Current time:  {}", _time);
    }


    // -- Functions requiring user-defined implementations -- //

    /// Perform the computation of a step
    void __perform_step () {
        impl().perform_step();
    }

    /// Monitor information in the terminal
    /* @detail The child implementation of this function will only be called if
     *         the monitor manager has determined that an emission will occur,
     *         because it only makes sense to collect data if it will be
     *         emitted in this step.
     */
    void __monitor () {
        if (_monitor.get_monitor_manager()->emit_enabled()) {
            // Perform actions that should only happen once by the monitor at
            // the highest level of the model hierarchy.
            if (_level == 1){
                // Supply the global time. When reaching this point, all sub-
                // models will also have reached this time.
                _monitor.get_monitor_manager()->set_time(_time);
            }

            // Call the child's implementation of the monitor functions.
            impl().monitor();
        }
    }
    
    /// Write data
    void __write_data () {
        impl().write_data();
    }

protected:
    /// Increment time
    /** \param dt Time increment, defaults to 1
     */
    void increment_time (const Time dt=1) {
        _time += dt;
    }
    // TODO perhaps make this private?

    /// cast to the derived class
    Derived& impl () { return static_cast<Derived&>(*this); }
    
    /// const cast to the derived interface
    const Derived& impl () const { return static_cast<const Derived&>(*this); }

private:
    /// Helper function to initialize the write_every member
    template<typename ParentModel>
    Time determine_write_every(ParentModel &parent_model) const {
        if (_cfg["write_every"]) {
            // Use the value given in the configuration
            return as_<Time>(_cfg["write_every"]);
        }
        // Use the value from the parent
        return parent_model.get_write_every();
    }

};


/// A class to use at the top level of the model hierarchy as a mock parent
/** \detail It is especially useful when initializing a top-level model as then
 *          the model constructor that expects a Model-class-like object can
 *          be used.
 *          This class also takes care to load and hold a configuration file,
 *          to create a HDFFile for output, and to initialize a shared RNG. A
 *          template parameter exists that allows customization of the RNG
 *          class.
 *
 *  \tparam RNG The RNG type to use
 */
template<typename RNG=DefaultRNG>
class PseudoParent
{
protected:
    // Convenience type definitions
    using Config = Utopia::DataIO::Config;
    using HDFFile = Utopia::DataIO::HDFFile;
    using HDFGroup = Utopia::DataIO::HDFGroup;
    using Time = std::size_t;
    using MonitorManager = Utopia::DataIO::MonitorManager;
    using Level = std::size_t;

    /// The hierarchical level
    const Level _level;

    /// The config node
    const Config _cfg;

    /// Pointer to the HDF5 file where data is written to
    const std::shared_ptr<HDFFile> _hdffile;

    /// Pointer to a RNG that can be shared between models
    const std::shared_ptr<RNG> _rng;

    /// Pointer to the logger of this (pseudo) model
    /** Required for passing on the logging level if unspecified for the
     *  respective model
     */
    const std::shared_ptr<spdlog::logger> _log;

    /// The monitor manager
    MonitorManager _monitor_mgr;

public:
    /// Constructor that only requires path to a config file
    /** \detail From the config file, all necessary information is extracted,
     *          i.e.: the path to the output file ('output_path') and the seed
     *          of the shared RNG ('seed'). These keys have to be located at
     *          the top level of the configuration file.
     *
     *  \param cfg_path The path to the YAML-formatted configuration file
     */
    PseudoParent (const std::string cfg_path)
    :
    // The hierarchical level is 0
    _level(0),
    // Initialize the config node from the path to the config file
    _cfg(YAML::LoadFile(cfg_path)),
    // Create a file at the specified output path and store the shared pointer
    _hdffile(std::make_shared<HDFFile>(as_str(_cfg["output_path"]), "w")),
    // Initialize the RNG from a seed
    _rng(std::make_shared<RNG>(as_<int>(_cfg["seed"]))),
    // And initialize the root logger at warning level
    _log(Utopia::init_logger("root", spdlog::level::warn, false)),
    // Create a monitor manager
    _monitor_mgr(as_double(_cfg["monitor_emit_interval"]))
    {
        setup_loggers(); // global loggers
        set_log_level(); // this log level

        _log->info("Initialized PseudoParent from config file");
        _log->debug("cfg_path:  {}", cfg_path);
    }
    

    /// Constructor that allows granular control over config parameters
    /**
     *  \param cfg_path The path to the YAML-formatted configuration file
     *  \param output_path Where the HDF5 file is to be located
     *  \param seed The seed the RNG is initialized with (default: 42)
     *  \param output_file_mode The access mode of the HDF5 file (default: w)
     */
    PseudoParent (const std::string cfg_path,
                  const std::string output_path,
                  const int seed=42,
                  const std::string output_file_mode="w",
                  const double emit_interval=5.)
    :
    // The hierarchical level is 0
    _level(0),
    // Initialize the config node from the path to the config file
    _cfg(YAML::LoadFile(cfg_path)),
    // Create a file at the specified output path
    _hdffile(std::make_shared<HDFFile>(output_path, output_file_mode)),
    // Initialize the RNG from a seed
    _rng(std::make_shared<RNG>(seed)),
    // And initialize the root logger at warning level
    _log(Utopia::init_logger("root", spdlog::level::warn, false)),
    // Create a monitor manager
    _monitor_mgr(emit_interval)
    {
        setup_loggers(); // global loggers
        set_log_level(); // this log level

        _log->info("Initialized PseudoParent from parameters");
        _log->debug("cfg_path:      {}", cfg_path);
        _log->debug("output_path:   {}  (mode: {})",
                    output_path, output_file_mode);
        _log->debug("seed:          {}", seed);
        _log->debug("emit_interval: {}", emit_interval);
    }



    // -- Getters -- //

    /// Return the hierarchical level within the model hierarchy
    Level get_level() const {
        return _level;
    }

    /// Return the config node of the Pseudo model, i.e. the root node
    Config get_cfg() const {
        return _cfg;
    }

    /// Return a pointer to the HDF data file
    std::shared_ptr<HDFFile> get_hdffile() const {
        return _hdffile;
    }
    
    /// Return a pointer to the HDF group, which is the base group of the file
    std::shared_ptr<HDFGroup> get_hdfgrp() const {
        return _hdffile->get_basegroup();
    }
    
    /// Return the parameter that controls how often write_data is called
    Time get_write_every() const {
        return as_<Time>(_cfg["write_every"]);
    }
    
    /// Return a pointer to the RNG
    std::shared_ptr<RNG> get_rng() const {
        return _rng;
    }

    /// Return a pointer to the logger of this model
    std::shared_ptr<spdlog::logger> get_logger() const {
        return _log;
    }

    /// The maximum time value as it can be found in the config
    /** @detail Currently, this reads the `num_steps` key, but this might be
     *          changed in the future to allow continuous time steps.
     */
    Time get_time_max() const {
        return as_<Time>(_cfg["num_steps"]);
    }

    /// Return the monitor manager of this model
    std::shared_ptr<MonitorManager> get_monitor_manager() const {
        return std::make_shared<MonitorManager>(_monitor_mgr);
    }


private:

    /// Set up the global loggers with levels specified in the config file
    void setup_loggers () const {
        Utopia::setup_loggers(
            spdlog::level::from_str(as_str(_cfg["log_levels"]["core"])),
            spdlog::level::from_str(as_str(_cfg["log_levels"]["data_io"]))
        );
    }

    /// Set the log level for the pseudo parent from the base_cfg
    void set_log_level () const {
        _log->set_level(
            spdlog::level::from_str(as_str(_cfg["log_levels"]["model"]))
        );
    }
};


} // namespace Utopia

#endif // UTOPIA_MODEL_HH
