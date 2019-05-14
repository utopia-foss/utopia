#ifndef UTOPIA_DATAIO_DATA_MANAGER_HH
#define UTOPIA_DATAIO_DATA_MANAGER_HH

// stl includes for having shared_ptr, hashmap, vector, swap
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <vector>
#include <sstream>

#include <boost/hana/ext/std/array.hpp>
#include <boost/hana/ext/std/tuple.hpp>
#include <boost/hana/for_each.hpp>

// utopia includes
#include "../../core/logging.hh"
#include "../../core/utils.hh"
#include "../cfg_utils.hh"
#include "utils.hh"
#include "defaults.hh"


namespace Utopia {
namespace DataIO {

using namespace Utopia::Utils;

// Doxygen group for DataManager ++++++++++++++++++++++++++++++++++++++++++++++
/**
 *  \addtogroup DataManager
 *  \{
 * 
 * The data management high-level interface has as its central piece the
 * ::DataManager class. That class orchestrates one or many WriteTask objects
 * and determines depending on so-called deciders and triggers, whether they
 * are to perform their write operation and whether a new write destination
 * should be acquired, respectively.
 *
 * For more information, see the class implementations.
 */

/**
 * @brief    Type traits for the DataManager
 * @detail   This allows to specify custom types for the DataManager. Usually,
 *           you only need to touch this if you do not want to use the
 *           defaults or if you want to need to explicitly specify a common
 *           base class because it cannot be deduced automatically.
 *
 * @note     As typical for trait structs, this is not intended to be ever
 *           instanciated; it is only used to encapsulate types used by the
 *           DataManager.
 *
 * @tparam TaskType      Type of the tasks. If you intend to use instances of a
 *                       derived class mixed with instances of a base class,
 *                       give the base class type here.
 * @tparam DeciderType   Type of the deciders
 * @tparam TriggerType   Type of the triggers
 * @tparam ExecutionProcessType  Type of the execution process functor
 */
template<class TaskType,
         class DeciderType,
         class TriggerType,
         class ExecutionProcessType>
struct DataManagerTraits {
    using Task = TaskType;
    using Decider = DeciderType;
    using Trigger = TriggerType;
    using ExecutionProcess = ExecutionProcessType;
};


/**
 * @brief Manage different tasks of writing out data from a source in a uniform
 *        yet flexible way.

 * @detail This is a class which, when being supplied with appropriate
 *         callables, manages their execution.
 *
 *         ## Idea
 *         The entire process of writing data from a source to a location
 *         consists of deciding when to write, deciding when to switch location
 *         (making sure the respective location exists) and then actually
 *         writing data.
 *         The ::DataManager handles multiple independent instances of this
 *         process. In that sence, it works like an STL stream.
 *
 *         ## Implementation
 *         - The decision of when to write is handled by a callable which can
 *           be evaluated in an ``if`` clause context. This is called a
 *           "decider".
 *           How this works is user defined. All deciders have to have the same
 *           call signature. All deciders need to have a common base type.
 *         - The decision of when to switch/built a new location to write to is
 *           handled by a callable which can be evaluated in an ``if`` clause
 *           context. This is called a "trigger". How this works is user
 *           defined. All trigges need to have a common base type.
 *         - The process of writing out data to the appropriate location is
 *           handled by a class/struct which supplies the means to do this.
 *           This is called a "write task" or short: "task".
 *           A task hence has to supply the means to produce a new location
 *           to write to, and the means to receive data and write them to this
 *           location.
 *           How this works is user defined. Usually, the ::WriteTask class
 *           is enough for our context of writing data to HDF5 files with
 *           attributes. All tasks need to have a common base type.
 *         - Each decider is associated with one or more tasks.
 *         - Each trigger is associated with one or more tasks.
 *         - The actual execution of all data writing processes is handled by
 *           one and only one callable, the so-called "execution process".
 *           This has to at least be able to receive a reference
 *           to a ::DataManager instance, a reference to an object being a
 *           source of data (the model), and optionally an arbitrary number of
 *           additional arguments.
 *
 *           The details of all these callables/classes are implementation
 *           defined and up to the user.
 *
 * @tparam   Traits  A ::DataManagerTraits-like type. Supplies all
 *           customaizable types for the DataManager, i.e. the decider,
 *           trigger, task, and execution process.
 */
template<class Traits>
class DataManager
{
  public:
    /// Alias for own type
    using Self = DataManager<Traits>;

    /// Task type, as defined in the traits
    using Task = typename Traits::Task;

    /// Decider type, as defined in the traits
    using Decider = typename Traits::Decider;

    /// Trigger type, as defined in the traits
    using Trigger = typename Traits::Trigger;

    /// Execution process type, as defined in the traits
    using ExecutionProcess = typename Traits::ExecutionProcess;

    /// Map of task names to shared pointers of Tasks; supporting polymorphism
    using TaskMap = std::unordered_map<std::string, std::shared_ptr<Task>>;

    /// Map of decider names to decider functions
    using DeciderMap =
        std::unordered_map<std::string, std::shared_ptr<Decider>>;

    /// Map of trigger names to trigger functions
    using TriggerMap =
        std::unordered_map<std::string, std::shared_ptr<Trigger>>;

    /// Map of decider/task names to a collection of task names
    using AssocsMap =
        std::unordered_map<std::string, std::vector<std::string>>;

    /// Map for holding decider types by default constructed objects
    using DeciderTypemap = std::unordered_map<std::string, Decider>;

    /// Map for holding trigger types by default constructed objects
    using TriggerTypemap = std::unordered_map<std::string, Trigger>;

    /// Map for holding Task types by default constructed objects
    using TaskTypemap = std::unordered_map<std::string, Task>;


  protected:
    /**
     * @brief Used to inform about progress of DataManager operations
     */
    std::shared_ptr<spdlog::logger> _log;

    /**
     * @brief Stores (name, task) pairs in an unordered map.
     */
    TaskMap _tasks;

    /**
     * @brief Stores (name, deciderfunction) pairs in an unordered map.
     */
    DeciderMap _deciders;

    /**
     * @brief Stores (name, triggerfunction) pairs in an unordered map.
     */
    TriggerMap _triggers;

    /**
     * @brief Mapping from deciders names to containers of names of tasks that
     *        use those deciders
     */
    AssocsMap _decider_task_map;

    /**
     * @brief Mapping from trigger names to containers of names of tasks that
     *        use those triggers
     */
    AssocsMap _trigger_task_map;

    /**
     * @brief Callable which tells how to utilize triggers, deciders, tasks to
     *        write data
     */
    ExecutionProcess _execution_process;

    // .. Construction Helpers ................................................

    /**
     * @brief Given a configuration node, sets up a (name -> object) map
     *
     * @tparam ObjMap    The map type to populate
     * @tparam ObjTuple  The tuple of known objects
     *
     * @param cfg        The mapping that is to be iterated over. Keys on the
     *                   top-level of this node will be the keys of the map
     *                   that is returned by this method. For each key, another
     *                   mapping is expected to provide arguments, e.g. whether
     *                   the object is to be considered ``active``. If the name
     *                   matches a name from ``known_objects``, that object is
     *                   carried over. If a ``type`` key is available in the
     *                   config of an object, it is used to construct a new
     *                   object, with the option to pass the ``args`` mapping
     *                   to its constructor.
     *
     * @param known_objects  A tuple of (name, object) pairs that is used as
     *                   basis to populate the object mapping.
     *
     * @return ObjMap    The constructor name -> object map.
     */
    template<typename ObjMap, typename ObjTuple>
    ObjMap setup_from_config(const Config& cfg, ObjTuple&& known_objects) {
        // Check whether the given configuration is valid
        if (not cfg) {
            throw std::invalid_argument("Received a zombie node for the setup "
                                        "of DataManager objects!");
        }
        else if (not cfg.IsMap()) {
            std::stringstream cfg_stream;
            cfg_stream << cfg;
            throw std::invalid_argument("Expected a mapping for DataManager "
                "object setup, got:\n" + cfg_stream.str());
        }

        // The name -> object map that is to be populated
        ObjMap map;
        _log->debug("Configuring DataManager objects ... (container size: {})",
                    std::tuple_size_v<std::decay_t<ObjTuple>>);

        // Iterate over the given configuration and populate the object map
        for (const auto& node_pair : cfg) {
            // Unpack the (key node, value node) pair, i.e. the name of the
            // object that is to be configured and the corresponding
            // configuration node
            const auto name = node_pair.first.as<std::string>();
            const auto& obj_cfg = node_pair.second;

            _log->trace("Configuration key:  {}", name);

            // Go over the known objects and decide whether to retain them or
            // whether new objects need to be constructed from the known ones
            boost::hana::for_each(known_objects, [&](const auto& named_object){
                auto [obj_name, object] = named_object;
                _log->trace("  Object name:      {}", obj_name);

                using ObjType = std::decay_t<decltype(object)>;

                // Depending on the name of the object, the name given in the
                // configuration, and the configuration itself, decide on
                // whether a new object needs to be constructed or can be
                // retained from the known objects. Start with the most
                // specific case...

                // If the configuration specifies a type, use that information
                // to either construct a new object from given arguments or
                // copy-construct one from an existing object
                if (    obj_cfg["type"]
                    and obj_name == get_as<std::string>("type", obj_cfg))
                {
                    _log->trace("  Creating new object from known type '{}'"
                                " ...", obj_name);
                    
                    // Determine whether to build with arguments or using the
                    // copy constructor
                    if (obj_cfg["args"]) {
                        _log->trace("  ... using given arguments ...");

                        // Need be config-constructible; otherwise throw
                        if constexpr (std::is_constructible_v<ObjType,
                                                              const Config&>)
                        {
                            // Construct from arguments, then associate
                            map[name] =
                                std::make_shared<ObjType>(obj_cfg["args"]);
                        }
                        else {
                            throw std::invalid_argument("Attempted to create "
                                "an object with name '" + name + "' from the "
                                "type of the known object of name '" + obj_name
                                + "' and configuration arguments, but that "
                                "type is not config-constructible! Either do "
                                "not pass `args`, in which case the copy-"
                                "constructor will be used, or implement a "
                                "constructor that accepts a DataIO::Config&.");
                        }
                    }
                    else {
                        _log->trace("  ... using copy constructor ...");
                        map[name] = std::make_shared<ObjType>(object);
                    }
                }

                // If this object's name matches that given in the
                // configuration, we want to retain that object (unless it is
                // not marked active).
                else if (obj_name == name) {
                    // Find out if it is marked active (default)
                    if (get_as<bool>("active", obj_cfg, true)) {
                        _log->trace("  Adding object ...");
                        map[name] = std::make_shared<ObjType>(object);
                    }
                    else {
                        _log->trace("  Not marked active. Skipping ...");
                    }
                }

                // else: No match for this config key. Do not add an entry.
            });
        }

        return map;
    }

    /**
     * @brief Given a configuration, builds an association map
     *
     * @tparam NCMap Type of the map that is represented by ``lookup_key``
     *
     * @param cfg               The mapping to iterate over
     * @param map_to_associate
     * @param lookup_key
     *
     * @return AssocsMap
     */
    template<typename NCMap>
    AssocsMap associate_from_config(const Config& cfg,
                                    NCMap&& map_to_associate,
                                    std::string lookup_key)
    {
        if (not cfg) {
            throw std::invalid_argument("Received a zombie node for task "
                "association with lookup key '" + lookup_key + "'!");
        }
        else if (not cfg.IsMap()) {
            std::stringstream cfg_stream;
            cfg_stream << cfg;
            throw std::invalid_argument("Expected a mapping for task -> "
                + lookup_key + " association, got:\n" + cfg_stream.str());
        }

        _log->info("Building task to {} associations from given config ...",
                   lookup_key);

        // The association pairs: task name -> decider/trigger name
        // This is an intermediary container that is later translated into the
        // actual association map.
        std::vector<std::pair<std::string, std::string>> assoc_pairs;

        // Iterate over the given configuration node
        for (const auto& node_pair : cfg) {
            // Unpack the (key node, value node) pair
            const auto task_name = node_pair.first.as<std::string>();
            const auto& task_cfg = node_pair.second;

            // Find out if active; true by default
            const auto active = get_as<bool>("active", task_cfg, true);

            // Associate only if active
            if (not active) {
                _log->debug("Task '{}' was marked as not active; skipping.",
                            task_name);
                continue;
            }
            // Get the name of the trigger or decider to associate to
            const auto associate_to = get_as<std::string>(lookup_key,
                                                          task_cfg);

            // Construct the association pairs
            assoc_pairs.push_back(std::make_pair(task_name, associate_to));
            _log->debug("Associating task '{}' to {} '{}'.",
                        task_name, lookup_key, associate_to);
        }

        // Use the helper function to build the actual association map
        return 
            _DMUtils::build_task_association_map<AssocsMap>(
                _tasks, map_to_associate, assoc_pairs
            );
    }


  public:
    // -- Public interface ----------------------------------------------------

    /**
     * @brief Invoke the execution process
     *
     * @tparam Args Parameter pack types
     *
     * @param model The model reference to pass on to the execution process
     * @param args  The parameter pack to pass on to the execution process
     */
    template<class Model, typename... Args>
    void operator()(Model&& model, Args&&... args)
    {
        _execution_process(
          *this, std::forward<Model>(model), std::forward<Args>(args)...);
    }

    /**
     * @brief  Register a new task and its name with the DataManager
     *
     * @detail No task named ``name`` may already be registered
     *
     * @tparam Tsk       Type of the task to register
     *
     * @param  name      Name of the task to register
     * @param  new_task  Task object to register
     *
     * @note   Might not remain in the public interface
     */
    template<typename Tsk>
    void register_task(const std::string& name, Tsk&& new_task) {
        if (_tasks.find(name) != _tasks.end()) {
            throw std::invalid_argument("A task named '" + name + "' is "
                                        "already registered!");
        }
        _tasks[name] = std::make_shared<Task>(std::forward<Tsk>(new_task));
    }

    /**
     * @brief  Register a new decider and its name with the DataManager
     *
     * @detail No decider named ``name`` may already be registered
     * 
     * @tparam Dcd         Type of the decider to register
     *
     * @param  name        Name of the new decider
     * @param  new_decider New decider object to register
     *
     * @note   Might not remain in the public interface
     */
    template<typename Dcd>
    void register_decider(const std::string& name, Dcd&& new_decider) {
        if (_deciders.find(name) != _deciders.end()) {
            throw std::invalid_argument("A decider named '" + name + "' is "
                                        "already registered!");
        }
        _deciders[name] =
            std::make_shared<Decider>(std::forward<Dcd>(new_decider));
    }

    /**
     * @brief  Register a new trigger and its name with the DataManager
     *
     * @detail No trigger named ``name`` may already be registered
     *
     * @tparam Trgr         Type of the trigger to register
     *
     * @param  name         Name of the new trigger
     * @param  new_trigger  New trigger object to register
     *
     * @note   Might not remain in the public interface
     */
    template<typename Trgr>
    void register_trigger(const std::string& name, Trgr&& new_trigger) {
        if (_triggers.find(name) != _triggers.end()) {
            throw std::invalid_argument("A trigger named '" + name + "' is "
                                        "already registered!");
        }
        _triggers[name] =
            std::make_shared<Trigger>(std::forward<Trgr>(new_trigger));
    }

    /**
     * @brief Associate a task with an existing decider
     *
     * @warning This does NOT take care of disassociating the task from a
     *          potentially already existing association!
     *
     * @note  Might not remain in the public interface or might change its
     *        interface in order to automatically remove a previous association
     *
     * @param task_name     Name of the task to reassociate
     * @param decider_name  Name of the decider to associate the task to
     * @param old_decider_name  Old decider name that should be disassociated
     */
    void link_task_to_decider(std::string task_name,
                              std::string decider_name,
                              std::string old_decider_name = "")
    {
        if (old_decider_name.size()) {
            // Remove existing association
            _decider_task_map.at(old_decider_name).erase(
                std::remove_if(
                    _decider_task_map[old_decider_name].begin(),
                    _decider_task_map[old_decider_name].end(),
                    [&](const std::string& s) {
                        return s == task_name; 
                    }),
                _decider_task_map[old_decider_name].end()
            );
        }
        // else: _Assume_ no previous association. // FIXME Ensure.

        // Add new association
        _decider_task_map[decider_name].push_back(task_name);
    }

    /**
     * @brief Associate a task with an existing trigger
     *
     * @warning This does NOT take care of disassociating the task from a
     *          potentially already existing association!
     *
     * @note  Might not remain in the public interface or might change its
     *        interface in order to automatically remove a previous association
     *
     * @param task_name     Name of the task to reassociate
     * @param trigger_name  Name of the trigger to associate the task to
     * @param old_trigger_name  Old trigger name that should be disassociated
     */
    void link_task_to_trigger(std::string task_name,
                              std::string trigger_name,
                              std::string old_trigger_name = "")
    {
        if (old_trigger_name.size()) {
            // Remove existing association
            _trigger_task_map.at(old_trigger_name).erase(
                std::remove_if(
                    _trigger_task_map[old_trigger_name].begin(),
                    _trigger_task_map[old_trigger_name].end(),
                    [&](const std::string& s) {
                        return s == task_name; 
                    }),
                _trigger_task_map[old_trigger_name].end()
            );

        }
        // else: _Assume_ no previous association. // FIXME Ensure.

        // Add new association
        _trigger_task_map[trigger_name].push_back(task_name);
    }


    /**
     * @brief Register a decider->trigger->task procedure after construction
     *
     * @detail This will invoke the respective ``register_*`` and
     *         ``link_task_to_*`` methods.
     *
     * @note   It is not possible to use config information here; that has to
     *         happen during construction.
     *
     * @tparam Tsk          The task object type to register
     * @tparam Dcd          The decider object type to register
     * @tparam Trgr         The trigger object type to register
     *
     * @param task_name     Name under which the task is to be registered
     * @param task          The task to add
     * @param decider_name  Name under which the decider is to be registered
     * @param decider       The decider to add
     * @param trigger_name  Name under which the trigger is to be registered
     * @param trigger       The trigger to add
     */
    template<typename Tsk, typename Dcd, typename Trgr>
    void register_procedure(const std::string& task_name,    Tsk&& task,
                            const std::string& decider_name, Dcd&& decider,
                            const std::string& trigger_name, Trgr&& trigger)
    {
        // Register
        register_task(task_name, task);
        register_decider(decider_name, decider);
        register_trigger(trigger_name, trigger);

        // Associate
        link_task_to_decider(task_name, decider_name);
        link_task_to_trigger(task_name, trigger_name);
    }

    // .. Getters .............................................................

    /**
     * @brief Get the container of decider objects
     *
     * @return const DeciderMap&
     */
    DeciderMap& get_deciders() {
        return _deciders;
    }

    /**
     * @brief Get the container of task objects
     *
     * @return const TaskMap&
     */
    TaskMap& get_tasks() {
        return _tasks;
    }

    /**
     * @brief Get the container of trigger objects
     *
     * @return const TriggerMap&
     */
    TriggerMap& get_triggers() {
        return _triggers;
    }

    /**
     * @brief Get the decider task map object
     *
     * @return const AssocsMap&
     */
    AssocsMap& get_decider_task_map() {
        return _decider_task_map;
    }

    /**
     * @brief Get the trigger task map object
     *
     * @return const AssocsMap&
     */
    AssocsMap& get_trigger_task_map() {
        return _trigger_task_map;
    }

    /**
     * @brief Get the logger used in this DataManager
     *
     * @return const std::shared_ptr<spdlog::logger>&
     */
    const std::shared_ptr<spdlog::logger>& get_logger() const {
        return _log;
    }

    // .. Rule of Five ........................................................

    /**
     * @brief Construct a new Data Manager object
     *
     */
    DataManager() = default;

    /**
     * @brief Construct a new Data Manager object
     *
     * @param other Object to copy state from
     */
    DataManager(const DataManager& other) = default;

    /**
     * @brief Construct a new Data Manager object
     *
     * @param other Object to move state from
     */
    DataManager(DataManager&& other) = default;

    /**
     * @brief Copy assignment
     *
     * @param other
     * @return DataManager& reference to object to assign the state from.
     */
    DataManager& operator=(const DataManager& other) = default;

    /**
     * @brief Move assignment
     *
     * @param other
     * @return DataManager& reference to object to assign the state from.
     */
    DataManager& operator=(DataManager&& other) = default;

    /**
     * @brief Destroy the Data Manager object
     *
     */
    virtual ~DataManager() = default;


    // .. Constructors ........................................................

    /**
     * @brief Construct a new DataManager object from tuple of task definitions
     *
     * @param model     The model this DataManager is to be associated with
     * @param cfg       The data manager configuration
     * @param tasks     Tuple of (name, Task) tuples
     * TODO
     */
    template<class Model,
             class Tasks,
             class Deciders,
             class Triggers,
             class ExecProcess,
             std::enable_if_t<is_callable_object_v<ExecProcess>, int> = 0>
    DataManager(Model& model,
                const Config& cfg,
                Tasks&& tasks,
                Deciders&& deciders,
                Triggers&& triggers,
                ExecProcess&& execproc)
      : _log(model.get_logger())

      // setup tasks, deciders and triggers from the given args and the config
      , _tasks(setup_from_config<TaskMap>(cfg["tasks"], tasks))
      , _deciders(setup_from_config<DeciderMap>(cfg["deciders"], deciders))
      , _triggers(setup_from_config<TriggerMap>(cfg["triggers"], triggers))

      // Set up task association mappings from the config
      , _decider_task_map(associate_from_config(cfg["tasks"],
                                                _deciders, "decider"))
      , _trigger_task_map(associate_from_config(cfg["tasks"],
                                                _triggers, "trigger"))
      , _execution_process(execproc)
    {
        _log->info("DataManager for '{}' model set up.", model.get_name());
    }

    /**
     * @brief Construct a new DataManager object
     * @detail This constructor needs to be supplied with arbitrary
     *         tuplelike objects containing pairs of (name, decider),
     *         (name, trigger), and (name, task) respectivly. They all need to
     *         be of the same length.
     *
     * @tparam M Type of the model
     * @tparam Deciders Tuple or array of pairs/tuples. Each pairs/tuple
     *         contains (name, Decider)
     * @tparam Triggers Tuple or array of pairs/tuples. Each pairs/tuple
     *         contains (name, Trigger)
     * @tparam Tasks Tuple or array of pairs/tuples. Each pairs/tuple
     *         contains (name, Task)
     *
     * @param model     The model this DataManager is to be associated with
     * @param tasks     Container of (name, Task) pairs
     * @param deciders  Container of (name, decider function) pairs
     * @param triggers  Container of (name, trigger function) pairs
     * @param task_decider_assocs  Container of task -> decider association
     *                  pairs, i.e. (task name, decider name) pairs
     * @param task_trigger_assocs  Container of task -> trigger association
     *                  pairs, i.e. (task name, trigger name) pairs
     */
    template<class Model,
             class Tasks,
             class Deciders,
             class Triggers,
             class ExecProcess,
             std::enable_if_t<    Utils::is_tuple_like_v<Tasks>
                              and Utils::is_tuple_like_v<Deciders>
                              and Utils::is_tuple_like_v<Triggers>,
                              int> = 0>
    DataManager(
      Model& model,
      Tasks&& tasks,
      Deciders&& deciders,
      Triggers&& triggers,
      ExecProcess&& execproc,
      std::vector<std::pair<std::string, std::string>> task_decider_assocs = {},
      std::vector<std::pair<std::string, std::string>> task_trigger_assocs = {})
      : // Get whatever is needed from the model
      _log(model.get_logger())

      // Unpack tasks, deciders, and triggers into the respective containers
      , _tasks(   _DMUtils::unpack_shared<TaskMap, Task>(tasks))
      , _deciders(_DMUtils::unpack_shared<DeciderMap, Decider>(deciders))
      , _triggers(_DMUtils::unpack_shared<TriggerMap, Trigger>(triggers))

      // Create maps: decider/trigger -> vector of task names
      , _decider_task_map([&]() {
          // Check if there would be issues in 1-to-1 association
          if (    task_decider_assocs.size() == 0
              and Utils::get_size_v<Deciders> != Utils::get_size_v<Tasks>)
          {
              throw std::invalid_argument(
                "deciders size != tasks size! You have to disambiguate "
                "the association of deciders and tasks by "
                "supplying an explicit task_decider_assocs argument if "
                "you want to have an unequal number of tasks and "
                "deciders.");
          }
          // FIXME _tasks and _deciders are not necessarily ordered in the
          //       same way!

          return
            _DMUtils::build_task_association_map<AssocsMap>(
                _tasks, _deciders, task_decider_assocs
            );
      }())
      , _trigger_task_map([&]() {
          // Check if there would be issues in 1-to-1 association
          if (    task_trigger_assocs.size() == 0
              and Utils::get_size_v<Triggers> != Utils::get_size_v<Tasks>)
          {
              throw std::invalid_argument(
                "triggers size != tasks size! You have to disambiguate "
                "the association of triggers and tasks by "
                "supplying an explicit task_trigger_assocs argument if "
                "you want to have an unequal number of tasks and "
                "triggers.");
          }

          return
            _DMUtils::build_task_association_map<AssocsMap>(
                _tasks, _triggers, task_trigger_assocs
            );
      }())
      , _execution_process(execproc)
    {
        _log->info("DataManager for '{}' model set up.", model.get_name());
    }

    // .. Helper Methods ......................................................

    /**
     * @brief Exchange the state of the caller with the argument 'other'.
     *
     * @param other
     */
    void swap(DataManager& other) {
        if (this == &other) {
            return;
        }

        using std::swap;
        swap(_log, other._log);
        swap(_tasks, other._tasks);
        swap(_deciders, other._deciders);
        swap(_triggers, other._triggers);
        swap(_decider_task_map, other._decider_task_map);
        swap(_trigger_task_map, other._trigger_task_map);
    }
};

/**
 *  \}  // endgroup DataManager
 */


// ++ Helpers and Deduction Guides ++++++++++++++++++++++++++++++++++++++++++++

/**
 * @brief Exchange the state of 'lhs' and 'rhs' DataManager instances
 *
 * @tparam Traits  The (matching) traits of both DataManagers
 */
template<typename Traits>
void swap(DataManager<Traits>& lhs, DataManager<Traits>& rhs) {
    lhs.swap(rhs);
}

/**
 * @brief determine the common type of the elements in named tuplelike object,
 *        i.e., a tuplelike object containing pairs of (name, object).
 *
 * @tparam Ts
 */
template<typename... Ts>
struct determine_common_type_in_named_tpl {
    using type =
        std::common_type_t<std::decay_t<std::tuple_element_t<1, Ts>>...>;
};

/// Deduction guides for DataManager tuple (+cfg) constructor
template<class Model,
         class Tasks,
         class Deciders,
         class Triggers,
         class ExecProcess,
         std::enable_if_t<is_callable_object_v<ExecProcess>, int> = 0>
DataManager(
    Model& model,
    const Config& cfg,
    Tasks&& tasks,
    Deciders&& deciders,
    Triggers&& triggers,
    ExecProcess&& execproc)
    ->DataManager<
        DataManagerTraits<
            Default::DefaultWriteTask<Model>,
            Default::DefaultDecider<Model>,
            Default::DefaultTrigger<Model>,
            ExecProcess
        >>;

/// Deduction guides for DataManager tuple constructor
template<class Model,
         class Tasks,
         class Deciders,
         class Triggers,
         class ExecProcess,
         std::enable_if_t<    Utils::is_tuple_like_v<Tasks>
                          and Utils::is_tuple_like_v<Deciders>
                          and Utils::is_tuple_like_v<Triggers>,
                          int> = 0>
DataManager(
    Model& model,
    Tasks&& tasks,
    Deciders&& deciders,
    Triggers&& triggers,
    ExecProcess&& execproc,
    std::vector<std::pair<std::string, std::string>> task_decider_assocs = {},
    std::vector<std::pair<std::string, std::string>> task_trigger_assocs = {})
    ->DataManager<
        DataManagerTraits<
            Utils::apply_t<determine_common_type_in_named_tpl, Tasks>,
            Utils::apply_t<determine_common_type_in_named_tpl, Deciders>,
            Utils::apply_t<determine_common_type_in_named_tpl, Triggers>,
            ExecProcess
        >>;


// ++ Default DataManager +++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 *  \addtogroup DataManagerDefaults Defaults
 *  \{
 *  \ingroup DataManager
 */

namespace Default {
/**
 * @brief  A default DataManager type
 *
 * @detail This uses all the default types for deciders, triggers, write task,
 *         and execution process
 *
 * @note   Cannot go anywhere else currently... Once possible, move to a more
 *         suitable location
 */
template<typename Model>
using DefaultDataManager =
    DataManager<
        DataManagerTraits<
            Default::DefaultWriteTask<Model>,
            Default::DefaultDecider<Model>,
            Default::DefaultTrigger<Model>,
            Default::DefaultExecutionProcess
        >
    >;
}

/**
 *  \}  // endgroup DataManager::Defaults
 */


} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_DATAIO_DATAMANAGER_HH
