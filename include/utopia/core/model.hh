#ifndef UTOPIA_CORE_MODEL_HH
#define UTOPIA_CORE_MODEL_HH

#include <algorithm>

#include "exceptions.hh"
#include "signal.hh"
#include "logging.hh"
#include "space.hh"
#include "parallel.hh"

#include "../data_io/hdffile.hh"
#include "../data_io/hdfgroup.hh"
#include "../data_io/cfg_utils.hh"
#include "../data_io/monitor.hh"
#include "../data_io/data_manager/data_manager.hh"
#include "../data_io/data_manager/factory.hh"


namespace Utopia {

/**
 *  \addtogroup Model
 *  \{
 */


/// How to write data in the models
enum class WriteMode {
    /// Basic writing features: write_start, write_every
    /** This leads to `write_data` being invoked the first time at time
      * `write_start` and henceforth every `write_every` steps. If
      * `write_start` is 0, this means that `write_data` is called after the
      * model constructor finished and before `perform_step` is called the
      * first time.
      */
    basic,

    /// Use the \ref DataManager to handle output
    /** The DataManager is invoked before the initial iteration and then once
      * after each iteration; it decides itself whether data is to be written
      * or not.
      * Note that the `write_start` and `write_every` arguments are ignored;
      * such functionality has to be implemented via the DataManager's
      * interface.
      */
    managed,

    /// Fully manual: write_data method is always called
    /** More accurately: `write_data` will be called after the model
      * constructor finished (and before `perform_step` is called for the
      * first time) and henceforth every time after `perform_step` finished
      * and the time was incremented. There are no further checks and the
      * `write_start` and `write_every` arguments are not needed.
      */
    manual,

    /// The write_data method is never called.
    off
};

/// Alias for the default write mode
constexpr WriteMode DefaultWriteMode = WriteMode::basic;


/// Wrapper struct for defining model class data types
/** Using the template parameters, derived classes can specify the types they
 *  desire to use.
 *
 *  \tparam RNGType               The RNG class to use
 *  \tparam data_write_mode       Mode in which data is written in model class
 *  \tparam Spacetype             The SpaceType to use, defaults to a 2D space
 *  \tparam ConfigType            The class to use for reading the config
 *  \tparam DataGroupType         The data group class to store datasets in
 *  \tparam DataSetType           The data group class to store data in
 *  \tparam TimeType              The type to use for the time members
 *  \tparam MonitorType           The type to use for the Monitor member
 *  \tparam MonitorManagerType    The type to use for the Monitor manager
 */
template<
    typename RNGType=DefaultRNG,
    WriteMode data_write_mode=DefaultWriteMode,
    typename SpaceType=DefaultSpace,
    typename ConfigType=Utopia::DataIO::Config,
    typename DataGroupType=Utopia::DataIO::HDFGroup,
    typename DataSetType=Utopia::DataIO::HDFDataset,
    typename TimeType=std::size_t,
    typename MonitorType=Utopia::DataIO::Monitor,
    typename MonitorManagerType=Utopia::DataIO::MonitorManager
    >
struct ModelTypes {
    using RNG = RNGType;
    static constexpr WriteMode write_mode = data_write_mode;
    using Space = SpaceType;
    using Config = ConfigType;
    using DataGroup = DataGroupType;
    using DataSet = DataSetType;
    using Time = TimeType;
    using Monitor = MonitorType;
    using MonitorManager = MonitorManagerType;
    using Level = std::size_t;
};


/// Base class interface for Models using the CRT Pattern
/** \tparam Derived    Type of the derived model class
 *  \tparam ModelTypes Traits of this model, can be used for specializing
 */
template<class Derived, typename ModelTypes>
class Model
{
public:
    // -- Data types uses throughout the model class --------------------------
    /// Data type that holds the configuration
    using Config = typename ModelTypes::Config;

    /// The data manager to use, specialized with the derived model
    using DataManager = DataIO::Default::DefaultDataManager<Derived>;

    /// Data type that is used for storing datasets
    using DataGroup = typename ModelTypes::DataGroup;

    /// Data type that is used for storing data
    using DataSet = typename ModelTypes::DataSet;

    /// Data type of the shared RNG
    using RNG = typename ModelTypes::RNG;

    /// Data type of the space this model resides in
    using Space = typename ModelTypes::Space;

    /// Data type for the model time
    using Time = typename ModelTypes::Time;

    /// Data type for the monitor
    using Monitor = typename ModelTypes::Monitor;

    /// Data type for the monitor manager
    using MonitorManager = typename ModelTypes::MonitorManager;

    /// Data type for the hierarchical level
    using Level = typename ModelTypes::Level;


protected:
    // -- Member declarations -------------------------------------------------
    /// Name of the model instance
    const std::string _name;

    /// Config node belonging to this model instance
    const Config _cfg;

    /// The hierarchical level
    const Level _level;

    /// The space this model resides in
    std::shared_ptr<Space> _space;

    /// Model-internal current time stamp
    Time _time;

    /// Model-internal maximum time stamp
    const Time _time_max;

    /// The HDF group this model instance should write its data to
    const std::shared_ptr<DataGroup> _hdfgrp;

    /// Which data-writing mode the base model should use
    static constexpr WriteMode _write_mode = ModelTypes::write_mode;

    /// First time at which write_data is called
    const Time _write_start;

    /// How often to call write_data from iterate
    const Time _write_every;

    /// The RNG shared between models
    const std::shared_ptr<RNG> _rng;

    /// The (model) logger
    const std::shared_ptr<spdlog::logger> _log;

    /// The monitor
    Monitor _monitor;

    /// Manager object for handling data output; see \ref DataManager
    /** \note The data manager is always constructed, but only used if the
      *       ``_write_mode`` was set to WriteMode::managed.
      */
    DataManager _datamanager;

private:
    // TODO Consider doing this in constructor directly
    auto setup_space() const {
        if (_cfg["space"]) {
            // Build a space with the given parameters
            return std::make_shared<Space>(_cfg["space"]);
        }
        else {
            // Use the default space
            return std::make_shared<Space>();
        }
    }


public:
    // -- Constructor ---------------------------------------------------------

    /// Constructs a Model instance
  /** Uses information from a parent model to create an instance of this
   *  model.
   *
   *  \tparam ParentModel The parent model's type
   *
   *  \param name         The name of this model instance, ideally used only
   *                      once on the current hierarchical level
   *  \param parent_model The parent model object from which the
   *                      corresponding config node, the group, the RNG,
   *                      and the parent log level are extracted.
   *  \param custom_cfg   If given, will use this configuration node instead
   *                      of trying to extract one from the parent model's
   *                      configuration.
   *  \param w_args       Passed on to DataManager constructor. If not given,
   *                      the DataManager will still be constructed. Take
   *                      care to also set the WriteMode accordingly.
   * \param w_deciders    Map which associates names with factory functions for
   *                      deciders of signature factory()::shared_ptr<Decider<Derived>>
   * \param w_triggers    Map which associates names with factory functions for
   *                      deciders of signature factory()::shared_ptr<Trigger<Derived>>
   */
  template < class ParentModel, class... WriterArgs >
  Model(const std::string&                           name,
        const ParentModel&                           parent_model,
        const Config&                                custom_cfg = {},
        std::tuple< WriterArgs... > w_args     = {},
        const DataIO::Default::DefaultDecidermap< Derived >&
            w_deciders = DataIO::Default::default_deciders< Derived >,
        const DataIO::Default::DefaultTriggermap< Derived >&
            w_triggers = DataIO::Default::default_triggers< Derived >
        ):
        // First thing: setup name, configuration, and level in hierarchy
        _name(name),
        _cfg(custom_cfg.size() ? custom_cfg
             : get_as<Config>(_name, parent_model.get_cfg())),
        _level(parent_model.get_level() + 1),

        // Determine space and time
        _space(this->setup_space()),
        _time(0),
        _time_max(parent_model.get_time_max()),

        // Extract the other information from the parent model object
        _hdfgrp(parent_model.get_hdfgrp()->open_group(_name)),
        _write_start(get_as<Time>("write_start", _cfg,
                                  parent_model.get_write_start())),
        _write_every(get_as<Time>("write_every", _cfg,
                                  parent_model.get_write_every())),
        _rng(parent_model.get_rng()),
        _log(spdlog::stdout_color_mt(parent_model.get_logger()->name() + "."
                                     + _name)),
        _monitor(Monitor(name, parent_model.get_monitor_manager())),

        // Default-construct the data maanger; only used if needed, see below.
        _datamanager()
    {
        // Set this model instance's log level
        if (_cfg["log_level"]) {
            // Via value given in configuration
            const auto lvl = get_as<std::string>("log_level", _cfg);
            _log->debug("Setting log level to '{}' ...", lvl);
            _log->set_level(spdlog::level::from_str(lvl));
        }
        else {
            // No config value given; use the level of the parent's logger
            _log->set_level(parent_model.get_logger()->level());
        }

        // Provide some information, also depending on write mode
        _log->info("Model base constructor finished.");
        _log->info("  time_max:    {:7d}", _time_max);

        if constexpr (_write_mode == WriteMode::basic) {
            _log->info("  write_mode:  {:>7s}", "basic");
            _log->info("  write_start: {:7d}", _write_start);
            _log->info("  write_every: {:7d}", _write_every);
            _log->info("  #writes:     {:7d}", get_remaining_num_writes());

            // Store relevant info in base group attributes
            _hdfgrp->add_attribute("write_mode", "basic");
            _hdfgrp->add_attribute("write_start", _write_start);
            _hdfgrp->add_attribute("write_every", _write_every);
            _hdfgrp->add_attribute("time_max",    _time_max);
        }
        else if constexpr (_write_mode == WriteMode::manual) {
            _log->info("  write_mode:  {:>7s}", "manual");
            _hdfgrp->add_attribute("write_mode", "manual");
        }
        else if constexpr (_write_mode == WriteMode::off) {
            _log->info("  write_mode:  {:>7s}", "off");
            _hdfgrp->add_attribute("write_mode", "off");
        }
        else if constexpr (_write_mode == WriteMode::managed) {
            static_assert(sizeof...(WriterArgs) > 0,
                          "No arguments to construct write_tasks given!");

            _log->info("  write_mode:  {:>7s}", "managed");

            // Store relevant info in base group attributes
            _hdfgrp->add_attribute("write_mode", "managed");

            // Some convenience key checks for nicer error messages.
            if (not _cfg["data_manager"]) {
                throw KeyError("data_manager", _cfg,
                               "Cannot set up DataManager!");
            }
            const auto dm_cfg = _cfg["data_manager"];

            if (not dm_cfg["tasks"]) {
                throw KeyError("tasks", dm_cfg,
                               "Cannot set up DataManager tasks!");
            }
            if (not dm_cfg["deciders"]) {
                throw KeyError("deciders", dm_cfg,
                               "Cannot set up DataManager deciders!");
            }
            if (not dm_cfg["triggers"]) {
                throw KeyError("triggers", dm_cfg,
                               "Cannot set up DataManager triggers!");
            }

            _log->info("Invoking DataManager task factory ...");

            _datamanager = DataIO::DataManagerFactory<Derived>()(
                dm_cfg, w_args, w_deciders, w_triggers
            );

            _log->info("DataManager set up with {} task(s), {} decider(s), "
                       "and {} trigger(s).", _datamanager.get_tasks().size(),
                       _datamanager.get_deciders().size(),
                       _datamanager.get_triggers().size());
        }
    }



    // -- Getters -------------------------------------------------------------

    /// Return the space this model resides in
    std::shared_ptr<Space> get_space() const {
        return _space;
    }

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

    /// Return the name of this model instance
    std::string get_name() const {
        return _name;
    }

    /// Return a pointer to the HDF group this model stores data in
    std::shared_ptr<DataGroup> get_hdfgrp() const {
        return _hdfgrp;
    }

    /// Return the parameter that controls when write_data is called first
    Time get_write_start() const {
        return _write_start;
    }

    /// Return the parameter that controls how often write_data is called
    Time get_write_every() const {
        return _write_every;
    }

    /// return the datamanager
    DataManager get_datamanager() const {
        return _datamanager;
    }

    /// Return the number of remaining `write_data` calls this model will make
    /** The 'remaining' refers to the current time being included into the
      * calculation, e.g.: when writing every time, currently at time == 42
      * and time_max == 43, it will return the value 2, i.e. for the write
      * operations at times 42 and 43
      */
    hsize_t get_remaining_num_writes() const {

        static_assert(_write_mode == WriteMode::basic
                     or
                     _write_mode == WriteMode::manual
                     or
                     _write_mode == WriteMode::off
                     or
                     _write_mode == WriteMode::managed);

        if constexpr (_write_mode == WriteMode::basic) {
            return (  (_time_max - std::max(_time, _write_start))
                    / _write_every) + 1;
        }
        else if constexpr (_write_mode == WriteMode::manual) {
            return _time_max - _time + 1;
        }
        else{
            return 0;
        }

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


    // -- Simulation control --------------------------------------------------
    /// A function that is to be called before starting the iteration of a model
    /** See __prolog() for default tasks
     */
    virtual void prolog () {
        __prolog();
    }

    /// A function that is to be called after the last iteration of a model
    /** See __epilog() for default tasks
     */
    virtual void epilog () {
        __epilog();
    }

    /// Iterate one (time) step of this model
    /** Increment time, perform step, emit monitor data, and write data.
     *  Monitoring is performed differently depending on the model level.
     *  The write_data method is called depending on the configured value for
     *  the `write_mode` (template parameter) and (if in mode `basic`): the
     *  configuration parameters `write_start` and `write_every`.
     */
    void iterate () {
        // -- Perform the simulation step
        __perform_step();
        increment_time();

        // -- Monitoring
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

        // -- Data output
        if constexpr (_write_mode == WriteMode::basic) {
            if (    (_time >= _write_start)
                and (_time - _write_start) % _write_every == 0) {
                __write_data();
            }
        }
        else if constexpr (_write_mode == WriteMode::manual) {
            __write_data();
        }
        else if constexpr (_write_mode == WriteMode::managed) {
            _datamanager(static_cast<Derived&>(*this));
        }

        _log->debug("Finished iteration: {:7d} / {:d}", _time, _time_max);
    }

    /// Run the model from the current time to the maximum time
    /** This repeatedly calls the iterate method until the maximum time is
      * reached. Additionally, it calls the ``__write_data`` method to allow
      * it to write the initial state. In write mode ``basic``, this is only
      * done if ``_write_start == _time``.
      */
    void run () {
        // First, attach the signal handler, such that the while loop below can
        // be left upon receiving of a signal.
        __attach_sig_handlers();

        // call the prolog of the model
        prolog();

        // Now, let's go repeatedly iterate the model ...
        _log->info("Running from current time  {}  to  {}  ...",
                   _time, _time_max);

        while (_time < _time_max) {
            iterate();

            if (stop_now.load()) {
                const auto signum = received_signum.load();

                if (signum == SIGUSR1) {
                    _log->warn("The stop condition for this run is fulfilled. "
                               "Not iterating further ...");
                }
                else {
                    _log->warn("Was told to stop. Not iterating further ...");
                }

                _log->info("Invoking epilog ...");
                epilog();

                throw GotSignal(received_signum.load());
            }
        }

        _log->info("Run finished. Current time:  {}", _time);

        // call the epilog of the model
        epilog();
    }


protected:
    // -- Functions requiring/allowing user-defined implementations -----------

    /// Perform the computation of a step
    void __perform_step () {
        impl().perform_step();
    }

    /// Monitor information in the terminal
    /* The child implementation of this function will only be called if the
     * monitor manager has determined that an emission will occur, because it
     * only makes sense to collect data if it will be emitted in this step.
     */
    void __monitor () {
        if (_monitor.get_monitor_manager()->emit_enabled()) {
            // Perform actions that should only happen once by the monitor at
            // the highest level of the model hierarchy.
            if (_level == 1){
                // Supply the global time. When reaching this point, all sub-
                // models will also have reached this time.
                _monitor.get_monitor_manager()->set_time_entries(_time,
                                                                 _time_max);
                // This method also sets the other top level entries
            }

            // Call the child's implementation of the monitor functions.
            impl().monitor();
        }
    }

    /// Write data; calls the implementation's write_data method
    void __write_data () {
        _log->trace("Calling write_data ...");
        impl().write_data();
    }

    /// Write the initial state
    void __write_initial_state () {
        // Select the required WriteMode
        // Decide on whether the initial state needs to be written
        if constexpr (_write_mode == WriteMode::basic) {
            if (_write_start == _time) {
                _log->info("Writing initial state ...");
                __write_data();
            }
        }
        else if constexpr (_write_mode == WriteMode::manual) {
            __write_data();
        }
        else if constexpr (_write_mode == WriteMode::managed) {
            _datamanager(static_cast<Derived&>(*this));
        }

    }

    /// Increment time
    /** \param dt Time increment, defaults to 1
     */
    void increment_time (const Time dt=1) {
        _time += dt;
    }

    /// The default prolog of a model
    /** Default tasks:
     *      1. call __write_initial_state
     */
    void __prolog () {
        __write_initial_state();

        this->_log->debug("Prolog finished.");
    }

    /// The default epilog of a model
    /** Default tasks:
     *      None
     */
    void __epilog () {
        this->_log->debug("Epilog finished.");
    }


    // -- CRTP ----------------------------------------------------------------
    /// cast to the derived class
    Derived& impl () {
        return static_cast<Derived&>(*this);
    }

    /// const cast to the derived interface
    const Derived& impl () const {
        return static_cast<const Derived&>(*this);
    }


public:
    // -- Convenience functions -----------------------------------------------

    /** @brief Create a new dataset within the given group
     *
     * The capacity - the shape of the dataset - is calculated automatically
     * from the num_steps and write_every parameter. Additionally, dataset
     * attributes are set that carry information on dimension labels and
     * coordinates.
     *
     * @note  The attributes are only written for the `time` dimension, as that
     *        is the only one that is known in this method. Furthermore, this
     *        assumes that it writes at `write_every` and - importantly - has
     *        the first write operation at time zero. Coordinates will be wrong
     *        if that is not the case! For such cases, it is advised to
     *        suppress writing of these attributes by setting the
     *        configuration entry _cfg['write_dim_labels_and_coords'] to false.
     *
     * @param name The name of the dataset
     * @param hdfgrp The parent HDFGroup
     * @param add_write_shape Additional write shape which, together with the
     *                        number of time steps, is used to calculate
     *                        the capacity of the dataset:
     *                        (capacity = (num_time_steps, add_write_shape)).
     * @param compression_level The compression level
     * @param chunksize The chunk size

     * @return std::shared_ptr<DataSet> The hdf dataset
     */
    std::shared_ptr<DataSet>
        create_dset(const std::string name,
                    const std::shared_ptr<DataGroup>& hdfgrp,
                    std::vector<hsize_t> add_write_shape,
                    const std::size_t compression_level=1,
                    const std::vector<hsize_t> chunksize={})
    {
        _log->debug("Creating dataset '{}' in group '{}' ...",
                    name, hdfgrp->get_path());

        // Calculate the number of time steps to be written
        const hsize_t num_write_ops = get_remaining_num_writes();

        // Calculate the shape of the dataset
        add_write_shape.insert(add_write_shape.begin(), num_write_ops);
        auto capacity = add_write_shape;

        // Create the dataset and return it.
        const auto dset = hdfgrp->open_dataset(name, capacity,
                                               chunksize, compression_level);
        _log->debug("Successfully created dataset '{}'.", name);

        // Write further attributes, if not specifically suppressed
        if (get_as<bool>("write_dim_labels_and_coords", _cfg, true)) {
            // We know that dimension 0 is the time dimension. Add the
            // attributes that specify dimension names and coordinates:
            dset->add_attribute("dim_name__0", "time");
            dset->add_attribute("coords_mode__time", "start_and_step");
            dset->add_attribute("coords__time",
                                std::vector<std::size_t>{_write_start,
                                                         _write_every});
            _log->debug("Added time dimension labels and coordinates to "
                        "dataset '{}'.", name);
        }

        return dset;
    }

    /** @brief Create a new dataset within the model's base data group
     *
     * The capacity - the shape of the dataset - is calculated automatically
     * from the num_steps and write_every parameter. Additionally, dataset
     * attributes are set that carry information on dimension labels and
     * coordinates.
     *
     * @note  The attributes are only written for the `time` dimension, as that
     *        is the only one that is known in this method. Furthermore, this
     *        assumes that it writes at `write_every` and - importantly - has
     *        the first write operation at time zero. Coordinates will be wrong
     *        if that is not the case! For such cases, it is advised to
     *        suppress writing of these attributes by setting the
     *        configuration entry _cfg['write_dim_labels_and_coords'] to false.
     *
     * @param name The name of the dataset
     * @param add_write_shape Additional write shape which, together with the
     *                        number of time steps, is used to calculate
     *                        the capacity of the dataset:
     *                        (capacity = (num_time_steps, add_write_shape)).
     * @param compression_level The compression level
     * @param chunksize The chunk size

     * @return std::shared_ptr<DataSet> The hdf dataset
     */
    std::shared_ptr<DataSet>
        create_dset(const std::string name,
                    const std::vector<hsize_t> add_write_shape,
                    const std::size_t compression_level=1,
                    const std::vector<hsize_t> chunksize={})
    {
        // Forward to the main create_dset function
        return create_dset(name,
                           _hdfgrp, // The base group for this model
                           add_write_shape, compression_level, chunksize);
    }

    /** @brief Create a dataset storing data from a CellManager
     *
     * The required capacity - the shape of the dataset - is calculated using
     * both data from the model and the CellManager. Additionally, dimension
     * and coordinate labels are added.
     *
     * @note   For the time dimension, the coordinates assume that data is
     *         written the first time at time 0 and then every _write_every.
     *         Time coordinates will be wrong if the model does not write the
     *         data this way. For such cases, it is advised to suppress writing
     *         of attributes by setting the _cfg['write_dim_labels_and_coords']
     *         entry to false.
     *
     * @param name The name of the dataset
     * @param cm   The CellManager whose cells' states are to be stored in the
     *             dataset
     * @param compression_level  The compression level
     * @param chunksize          The chunk size

     * @return std::shared_ptr<DataSet> The newly created HDFDataset
     */
    template<class CellManager>
    std::shared_ptr<DataSet>
        create_cm_dset(const std::string name,
                       const CellManager& cm,
                       const std::size_t compression_level=1,
                       const std::vector<hsize_t> chunksize={})
    {
        // Forward to the main create_dset function
        const auto dset = create_dset(name, _hdfgrp,
                                      {cm.cells().size()}, // -> 2D dataset
                                      compression_level, chunksize);

        // Set attributes to mark this dataset as containing grid data
        dset->add_attribute("content", "grid");
        dset->add_attribute("grid_shape", cm.grid()->shape());
        dset->add_attribute("space_extent", cm.grid()->space()->extent);
        dset->add_attribute("index_order", "F");
        // NOTE CellManager uses "column-style" index ordering, also called "Fortran-style".

        _log->debug("Added attributes to dataset '{}' to mark it as storing "
                    "grid data.", name);

        // Write additional attributes, if not specifically suppressed.
        if (get_as<bool>("write_dim_labels_and_coords", _cfg, true)) {
            // We know that the dimensions here refer to (time, cell ids). The
            // time  information is already aded in create_dset; add the ID
            // information here
            dset->add_attribute("dim_name__1", "ids");

            // For ids, the dimensions are trivial
            dset->add_attribute("coords_mode__ids", "range");
            dset->add_attribute("coords__ids",
                                std::vector<std::size_t>{cm.cells().size()});

            _log->debug("Added cell ID dimension labels and coordinates to "
                        "dataset '{}'.", name);
        }

        return dset;
    }


private:
    // -- Private Helper Methods ----------------------------------------------

    /// Attaches signal handlers: SIGINT, SIGTERM, SIGUSR1
    /** These signals are caught and handled such that the run method is able
      * to finish in an ordered manner, preventing data corruption. This is
      * done by invoking attach_signal_handler and attaching the
      * default_signal_handler to them.
      *
      * The SIGUSR1 signal is sent by the frontend if the stop conditions for
      * this simulation are all fulfilled. It will likewise lead to the
      * invocation of the default_signal_handler.
      */
    void __attach_sig_handlers() {
        _log->debug("Attaching signal handlers for SIGINT and SIGTERM ...");

        attach_signal_handler(SIGINT);
        attach_signal_handler(SIGTERM);

        _log->debug("Attaching signal handler for stop conditions, triggered "
                    "by SIGUSR1 ({:d}) ...", SIGUSR1);
        attach_signal_handler(SIGUSR1);

        _log->debug("Signal handlers attached.");
    }
};



/// A class to use at the top level of the model hierarchy as a mock parent
/** It is especially useful when initializing a top-level model as the
 *  the model constructor that expects a Model-class-like object can be used.
 *  This class also takes care to load and hold a configuration file, to
 *  create a HDFFile for output, and to initialize a shared RNG. A template
 *  parameter exists that allows customization of the RNG class.
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
    /** From the config file, all necessary information is extracted, i.e.:
     *  the path to the output file ('output_path') and the seed of the shared
     *  RNG ('seed'). These keys have to be located at the top level of the
     *  configuration file.
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
    _hdffile(std::make_shared<HDFFile>(get_as<std::string>("output_path",
                                                           _cfg), "w")),
    // Initialize the RNG from a seed
    _rng(std::make_shared<RNG>(get_as<int>("seed", _cfg))),
    // And initialize the root logger at warning level
    _log(Utopia::init_logger("root", spdlog::level::warn, false)),
    // Create a monitor manager
    _monitor_mgr(get_as<double>("monitor_emit_interval", _cfg))
    {
        setup_loggers(); // global loggers
        set_log_level(); // this log level
        ParallelExecution::init(_cfg);

        _log->info("Initialized PseudoParent from config file");
        _log->debug("cfg_path:       {}", cfg_path);
        _log->debug("output_path:    {}", get_as<std::string>("output_path",
                                                              _cfg));
        _log->debug("RNG seed:       {}", get_as<int>("seed", _cfg));
        _log->debug("emit_interval:  {}s",
                    get_as<double>("monitor_emit_interval", _cfg));
    }


    /// Constructor that allows granular control over config parameters
    /**
     *  \param cfg_path The path to the YAML-formatted configuration file
     *  \param output_path Where the HDF5 file is to be located
     *  \param seed The seed the RNG is initialized with (default: 42)
     *  \param output_file_mode The access mode of the HDF5 file (default: w)
     *  \param emit_interval The monitor emit interval (in seconds)
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
        ParallelExecution::init(_cfg);

        _log->info("Initialized PseudoParent from parameters");
        _log->debug("cfg_path:      {}", cfg_path);
        _log->debug("output_path:   {}  (mode: {})",
                    output_path, output_file_mode);
        _log->debug("RNG seed:      {}", seed);
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

    /// Return the parameter that controls when write_data is called first
    Time get_write_start() const {
        return get_as<Time>("write_start", _cfg, 0);
    }

    /// Return the parameter that controls how often write_data is called
    Time get_write_every() const {
        return get_as<Time>("write_every", _cfg, 1);
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
    /** Currently, this reads the `num_steps` key, but this might be changed
      * in the future to allow continuous time steps.
      */
    Time get_time_max() const {
        return get_as<Time>("num_steps", _cfg);
    }

    /// Return the monitor manager of this model
    std::shared_ptr<MonitorManager> get_monitor_manager() const {
        return std::make_shared<MonitorManager>(_monitor_mgr);
    }


private:

    /// Set up the global loggers with levels specified in the config file
    /** Extract the following keys from the ``_cfg`` member:
      *
      *     - ``log_levels.core``:      the Core module log level; required
      *     - ``log_levels.data_io``:   the DataIO module log level; required
      *     - ``log_levels.data_mngr``: the DataIO::DataManager log level;
      *                                 optional, defaults to ``warn``
      *     - ``log_pattern``:          the global log pattern
      */
    void setup_loggers () const {
        Utopia::setup_loggers(
            spdlog::level::from_str(
                get_as<std::string>("core", _cfg["log_levels"])
            ),
            spdlog::level::from_str(
                get_as<std::string>("data_io", _cfg["log_levels"])
            ),
            spdlog::level::from_str(
                get_as<std::string>("data_mngr", _cfg["log_levels"], "warn")
            ),
            get_as<std::string>("log_pattern", _cfg, "")
        );
    }

    /// Set the log level for the pseudo parent from the base_cfg
    void set_log_level () const {
        _log->set_level(
            spdlog::level::from_str(get_as<std::string>("model",
                                                        _cfg["log_levels"]))
        );
    }
};


// end group Model
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_MODEL_HH
