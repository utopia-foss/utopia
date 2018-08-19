#ifndef UTOPIA_MODEL_HH
#define UTOPIA_MODEL_HH

#include <dune/utopia/data_io/hdffile.hh>
#include <dune/utopia/data_io/hdfgroup.hh>
#include <dune/utopia/data_io/cfg_utils.hh>
#include <dune/utopia/core/logging.hh>


namespace Utopia {


/// Dummy type for boundary conditions; this is used as a default value
struct BCDummy {};


/// Wrapper struct for defining base class data types
/** \tparam DataType              Type of the data the model operates on
 *  \tparam BoundaryConditionType Data type of the boundary condition. If not
 *                                set, an empty struct is used as default.
 *  \tparam RNGType               The RNG class to use
 *  \tparam ConfigType            The class to use for reading the config
 *  \tparam DataGroupType         The data group class to store datasets in
 *  \tparam DataSetType           The data group class to store data in
 *  \tparam TimeType              The type to use for the time members
 */
template<typename DataType,
         typename BoundaryConditionType=BCDummy,
         typename RNGType=DefaultRNG,
         typename ConfigType=Utopia::DataIO::Config,
         typename DataGroupType=Utopia::DataIO::HDFGroup,
         typename DataSetType=Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>,
         typename TimeType=std::size_t
         >
struct ModelTypes
{
    using Data = DataType;
    using BCType = BoundaryConditionType;
    using RNG = RNGType;
    using Config = ConfigType;
    using DataGroup = DataGroupType;
    using DataSet = DataSetType;
    using Time = TimeType;
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

    /// Data type of the state
    using Data = typename ModelTypes::Data;
    
    /// Data type of the boundary condition
    using BCType = typename ModelTypes::BCType;
    
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

    /// The RNG shared between models
    const std::shared_ptr<RNG> _rng;

    /// The (model) logger
    const std::shared_ptr<spdlog::logger> _log;

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
        _rng(parent_model.get_rng()),
        _log(spdlog::stdout_color_mt(parent_model.get_logger()->name() + "."
                                     + _name))
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

        // TODO add other informative log messages here?
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
    
    /// Return a pointer to the shared RNG
    std::shared_ptr<RNG> get_rng() const {
        return _rng;
    }

    /// Return a pointer to the logger of this model
    std::shared_ptr<spdlog::logger> get_logger() const {
        return _log;
    }


    // -- Default implementations -- //

    /// Iterate one (time) step of this model
    /** @detail Increment time, perform step, then write data
     */
    void iterate () {
        perform_step();
        increment_time();
        write_data();
        _log->debug("Finished iteration: {:9d} / {:d}", _time, _time_max);
    }

    /// Run the model from the current time to the maximum time
    /** @detail This repeatedly calls the iterate method, which increments time
      *
      */
    void run () {
        _log->info("Running model from current time  {}  to time  {}  ...",
                   _time, _time_max);

        while (_time < _time_max) {
            iterate();
        }
        _log->info("Run finished. Current time:  {}", _time);
    }


    // -- User-defined implementations -- //

    /// Perform the computation of a step
    void perform_step () {
        impl().perform_step();
    }
    
    /// Write data
    void write_data () {
        impl().write_data();
    }

    /// Return const reference to stored data
    const Data& data () const {
        return impl().data();
    }
    
    /// Set model boundary condition
    void set_boundary_condition (const BCType& bc) {
        impl().set_boundary_condition(bc);
    }

    /// Set model initial condition
    void set_initial_condition (const Data& ic) {
        impl().set_initial_condition(ic);
    }

protected:
    /// Increment time
    /** \param dt Time increment, defaults to 1
     */
    void increment_time (const Time dt=1) {
        _time += dt;
    }

    /// cast to the derived class
    Derived& impl () { return static_cast<Derived&>(*this); }
    
    /// const cast to the derived interface
    const Derived& impl () const { return static_cast<const Derived&>(*this); }
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
    // Initialize the config node from the path to the config file
    _cfg(YAML::LoadFile(cfg_path)),
    // Create a file at the specified output path and store the shared pointer
    _hdffile(std::make_shared<HDFFile>(as_str(_cfg["output_path"]), "w")),
    // Initialize the RNG from a seed
    _rng(std::make_shared<RNG>(as_<int>(_cfg["seed"]))),
    // And initialize the root logger at warning level
    _log(Utopia::init_logger("root", spdlog::level::warn, false))
    {
        setup_loggers(); // global loggers
        set_log_level(); // this log level

        _log->info("Initialized PseudoParent from config file");
        _log->debug("cfg_path:      {}", cfg_path);
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
                  const std::string output_file_mode="w")
    :
    // Initialize the config node from the path to the config file
    _cfg(YAML::LoadFile(cfg_path)),
    // Create a file at the specified output path
    _hdffile(std::make_shared<HDFFile>(output_path, output_file_mode)),
    // Initialize the RNG from a seed
    _rng(std::make_shared<RNG>(seed)),
    // And initialize the root logger at warning level
    _log(Utopia::init_logger("root", spdlog::level::warn, false))
    {
        setup_loggers(); // global loggers
        set_log_level(); // this log level

        _log->info("Initialized PseudoParent from parameters");
        _log->debug("cfg_path:      {}", cfg_path);
        _log->debug("output_path:   {}  (mode: {})",
                    output_path, output_file_mode);
        _log->debug("seed:          {}", seed);
    }



    // -- Getters -- //

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
