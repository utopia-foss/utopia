#ifndef UTOPIA_DATAIO_DATA_MANAGER_HH
#define UTOPIA_DATAIO_DATA_MANAGER_HH

// stl includes for having shared_ptr, hashmap, vector, swap
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <vector>

// utopia includes
#include "../../core/logging.hh"
#include "../../core/utils.hh"
#include "../cfg_utils.hh"
#include "defaults.hh"
#include "utils.hh"

namespace Utopia
{
namespace DataIO
{

using namespace Utopia::Utils;

// Doxygen group for DataManager ++++++++++++++++++++++++++++++++++++++++++++++
/**
 *  \addtogroup DataManager
 *  \{
 */

/**
 * @page Datamanager Datamanager module
 * \section what Overview
 *
 * The data management high-level interface has as its central piece the
 * ::DataManager class, which acts as a manager for abstract 'writetasks'
 * which contain the relevant functionality.
 * \section impl Implementation
 * The datamanager is implemented as a class which orchestrates so called write
 * tasks, which consist of functionality for creating resources to write to,
 * extracting data for writing and actual data writing, and for handling meta
 * data. It is flanked by functions which decide when to write and when to
 * change the associated resource. A datamanager is not bound to a specific
 * data format or library, hence does not know anything about the actual
 * process of writing out data. It merely organizes the writing process.
 *
 */

/**
 * @brief    Type traits for the DataManager
 *           This allows to specify custom types for the DataManager. Usually,
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
template < class TaskType,
           class DeciderType,
           class TriggerType,
           class ExecutionProcessType >
struct DataManagerTraits
{
    using Task             = TaskType;
    using Decider          = DeciderType;
    using Trigger          = TriggerType;
    using ExecutionProcess = ExecutionProcessType;
};

/**
 * @brief Manage different tasks of writing out data from a source in a uniform
 *        yet flexible way.
 *        This is a class which, when being supplied with appropriate
 *        callables, manages their execution.
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
template < class Traits >
class DataManager
{
  public:
    /// Alias for own type
    using Self = DataManager< Traits >;

    /// Task type, as defined in the traits
    using Task = typename Traits::Task;

    /// Decider type, as defined in the traits
    using Decider = typename Traits::Decider;

    /// Trigger type, as defined in the traits
    using Trigger = typename Traits::Trigger;

    /// Execution process type, as defined in the traits
    using ExecutionProcess = typename Traits::ExecutionProcess;

    /// Map of task names to shared pointers of Tasks; supporting polymorphism
    using TaskMap = std::unordered_map< std::string, std::shared_ptr< Task > >;

    /// Same as TaskMap, but using std::map such that ordering is preserved
    using OrderedTaskMap = std::map< std::string, std::shared_ptr< Task > >;

    /// Map of decider names to decider functions
    using DeciderMap =
        std::unordered_map< std::string, std::shared_ptr< Decider > >;

    /// Same as DeciderMap, but using std::map such that ordering is preserved
    using OrderedDeciderMap =
        std::map< std::string, std::shared_ptr< Decider > >;

    /// Map of trigger names to trigger functions
    using TriggerMap =
        std::unordered_map< std::string, std::shared_ptr< Trigger > >;

    /// Same as TriggerMap, but using std::map such that ordering is preserved
    using OrderedTriggerMap =
        std::map< std::string, std::shared_ptr< Trigger > >;

    /// Map of decider/task names to a collection of task names
    using AssocsMap =
        std::unordered_map< std::string, std::vector< std::string > >;

  protected:
    /**
     * @brief Used to inform about progress of DataManager operations
     */
    std::shared_ptr< spdlog::logger > _log;

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

    template < typename ObjMap, typename KnownObjectsMap >
    ObjMap
    _setup_from_config(const Config& cfg, KnownObjectsMap&& known_objects)
    {
        _log->debug("Setting up name -> object map from config node ...");

        // Check whether the given configuration is valid
        if (not cfg)
        {
            throw std::invalid_argument("Received a zombie node for the setup "
                                        "of DataManager objects!");
        }
        else if (not cfg.IsMap())
        {
            throw std::invalid_argument("Expected a mapping for DataManager "
                                        "object setup, got:\n" +
                                        to_string(cfg));
        }

        // The name -> object map that is to be populated
        ObjMap map;
        _log->debug("Configuring DataManager objects ... (container size: {})",
                    known_objects.size());

        // Go over the known objects and decide whether to retain them
        // as they are or whether new objects need to be constructed from the
        // known ones
        // Depending on the name of the object, the name given in the
        // configuration, and the configuration itself, decide on
        // whether a new object needs to be constructed or can be
        // retained from the known objects.
        // If the configuration specifies a type, use that information
        // to either construct a new object from given arguments or
        // copy-construct one from an existing object
        for (const auto& node_pair : cfg)
        {
            // Unpack the (key node, value node) pair, i.e. the name of the
            // object that is to be configured and the corresponding
            // configuration node
            const auto  cfg_name = node_pair.first.as< std::string >();
            const auto& obj_cfg  = node_pair.second;

            const auto type_name = get_as< std::string >("type", obj_cfg);

            _log->debug("Attempting to build {} of type {} from config",
                        cfg_name,
                        type_name);

            if (known_objects.find(type_name) == known_objects.end())
            {
                throw std::invalid_argument("Error for node " + cfg_name +
                                            ": No 'type' node given");
            }
            else
            {
                if (obj_cfg["args"])
                {
                    _log->debug("  ... using given arguments from config ...");
                    map[cfg_name] =
                        known_objects[type_name](); // default construct
                    map[cfg_name]->set_from_cfg(obj_cfg["args"]);
                }
                else
                {

                    // if no args is given, do a default build because this
                    // not all of the deciders/triggers need an args node
                    _log->debug("... constructing {} of type {} without "
                                "config args because no node 'args' is "
                                "given for it in the config.",
                                cfg_name,
                                type_name);

                    map[cfg_name] = known_objects[type_name]();
                }
            }
        }

        return map;
    }

    /**
     * @brief Check which tasks supplied to the datamanager are active and shall
     *        be retained, using the config node provided
     *
     * @param task_cfg config node defining tasks
     * @param tasks unordered map naming pointers to tasks
     * @return TaskMap 'tasks' map filtered by activity
     */
    TaskMap
    _filter_tasks_from_config(const Config& task_cfg, TaskMap& tasks)
    {
        // map to use in the end
        TaskMap map;

        if (not task_cfg)
        {
            throw std::invalid_argument(
                "Error: data_manager config node needs to contain a node "
                "'tasks' which it apparently is missing ");
        }
        if (not task_cfg.IsMap())
        {
            throw std::invalid_argument("Expected a mapping for DataManager "
                                        "task filtering, got:\n" +
                                        to_string(task_cfg));
        }

        for (const auto& node_pair : task_cfg)
        {
            // Unpack the (key node, value node) pair, i.e. the name of the
            // object that is to be configured and the corresponding
            // configuration node
            const auto  cfg_name = node_pair.first.as< std::string >();
            const auto& obj_cfg  = node_pair.second;

            _log->debug("Investigating task {} and checking if it is active ",
                        cfg_name);

            if (get_as< bool >("active", obj_cfg))
            {

                _log->debug("Task '{}' was marked as active; will be kept.",
                            cfg_name);

                if (tasks.find(cfg_name) == tasks.end())
                {
                    throw std::invalid_argument(
                        "Error, no task supplied to the datamanager is named " +
                        cfg_name);
                }
                else
                {
                    map[cfg_name] = tasks[cfg_name];
                }
            }
            else
            {
                // skip inactive tasks
                _log->debug("Task '{}' was marked as not active; skipping.",
                            cfg_name);
                continue;
            }
        }

        return map;
    }

    /**
     * @brief Given a configuration, builds an association map.
     * @tparam DTMap automatically determined
     * @param task_cfg  The mapping to iterate as given by the config.
     *                  It holds the tasks and names the decider and trigger
     *                  and the task is associated to and tells if it is active
     *                  or not.
     * @param dt_map  Map that holds names->decider/trigger mapping. Used to
     *                check if the names in the config indeed match some known
     *                decider/trigger.
     * @param lookup_key  Key which names the mapping used for association, i.e.,
     *                    "decider" or "trigger".
     * @return AssocsMap  Map that associates the given deciders/triggers with a
     *                    vector of names that name tasks.
     */
    template < typename DTMap >
    AssocsMap
    _associate_from_config(const Config& task_cfg,
                           DTMap&&       dt_map,
                           std::string   lookup_key)
    {
        AssocsMap map;

        // error checking regarding the task_cfg node is done in the
        // _filter_tasks function, and hence does not need to be repeated here

        _log->debug("Building task to {} associations from given config ...",
                    lookup_key);

        // Iterate over the given configuration node, pull out the name of the
        // task and the name of the associated decider/trigger, and put the
        // AssocsMap together from this
        for (const auto& node_pair : task_cfg)
        {
            // Unpack the (key node, value node) pair
            const auto  task_name = node_pair.first.as< std::string >();
            const auto& task_cfg  = node_pair.second;

            // Find out if active; true by default
            const auto active = get_as< bool >("active", task_cfg, true);

            // Associate only if active
            if (not active)
            {
                _log->debug("Task '{}' was marked as not active; skipping.",
                            task_name);
                continue;
            }

            // Get the name of the trigger or decider to associate to

            const auto dt_to_associate_to =
                get_as< std::string >(lookup_key, task_cfg);

            // Find erroneous config namings for deciders/triggers
            if (dt_map.find(dt_to_associate_to) == dt_map.end())
            {
                this->_log->info(" Error for decider/trigger: {}",
                                 dt_to_associate_to);
                throw std::invalid_argument(
                    "Error when trying to associate tasks to deciders or "
                    "triggers: "
                    "Name in config does not match the name of a "
                    "decider/trigger known to the datamanager");
            }
            else
            {
                // dt_to_associate_to exists in the dt_map, we're good
                map[dt_to_associate_to].push_back(task_name);

                _log->debug("Associating task '{}' to {} '{}'.",
                            task_name,
                            lookup_key,
                            dt_to_associate_to);
            }
        }

        // Use the helper function to build the actual association map
        return map;
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
    template < class Model, typename... Args >
    void
    operator()(Model&& model, Args&&... args)
    {
        _execution_process(
            *this, std::forward< Model >(model), std::forward< Args >(args)...);
    }

    // .. Getters .............................................................

    /**
     * @brief Get the container of decider objects
     *
     * @return const DeciderMap&
     */
    DeciderMap&
    get_deciders()
    {
        return _deciders;
    }

    /**
     * @brief Get the container of task objects
     *
     * @return const TaskMap&
     */
    TaskMap&
    get_tasks()
    {
        return _tasks;
    }

    /**
     * @brief Get the container of trigger objects
     *
     * @return const TriggerMap&
     */
    TriggerMap&
    get_triggers()
    {
        return _triggers;
    }

    /**
     * @brief Get the decider task map object
     *
     * @return const AssocsMap&
     */
    AssocsMap&
    get_decider_task_map()
    {
        return _decider_task_map;
    }

    /**
     * @brief Get the trigger task map object
     *
     * @return const AssocsMap&
     */
    AssocsMap&
    get_trigger_task_map()
    {
        return _trigger_task_map;
    }

    /**
     * @brief Get the logger used in this DataManager
     *
     * @return const std::shared_ptr<spdlog::logger>&
     */
    const std::shared_ptr< spdlog::logger >&
    get_logger() const
    {
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
    DataManager&
    operator=(const DataManager& other) = default;

    /**
     * @brief Move assignment
     *
     * @param other
     * @return DataManager& reference to object to assign the state from.
     */
    DataManager&
    operator=(DataManager&& other) = default;

    /**
     * @brief Destroy the Data Manager object
     *
     */
    virtual ~DataManager() = default;

    // .. Constructors ........................................................

    /**
     * @brief Construct DataManager using a config node
     * @details Arguments are an unordered_map that assigns names to
     *          shared_ptrs of tasks, and two other unordered_maps that assign
     *          names to factory functions for producing shared_ptrs to Deciders
     *          and triggers. This factory function approach enables us to use
     *          dynamic polymorphism on the deciders and triggers. This ability
     *          is not useful for tasks however, because they are designed to
     *          receive their functionality from the outside via passing them
     *          function objects on construction. Hence it is forgone here.
     *
     * @param cfg  Configuration node that contains datamanager config options
     * @param tasks unordered_map containing name->shared_ptr_to_task mapping
     * @param deciders map associating names to factory functions producing
     *                 shared_ptr_to_deciders
     * @param triggers map associating names to factory functions producing
     *                 shared_ptr_to_triggers
     * @param execproc Function object that determines the execution process of
     *                 the datamanager, i.e., how the deciders, triggers and
     *                 tasks work together to produce the output data
     */
    DataManager(
        const Config& cfg,
        TaskMap       tasks,
        std::unordered_map< std::string,
                            std::function< std::shared_ptr< Decider >() > >
            deciders,
        std::unordered_map< std::string,
                            std::function< std::shared_ptr< Trigger >() > >
                         triggers,
        ExecutionProcess execproc) :
        // Get the global data manager logger
        _log(spdlog::get("data_mngr")),
        _tasks(_filter_tasks_from_config(cfg["tasks"], tasks)),
        _deciders(_setup_from_config< DeciderMap >(cfg["deciders"], deciders)),
        _triggers(_setup_from_config< TriggerMap >(cfg["triggers"], triggers)),
        // Create maps: decider/trigger -> vector of task names
        _decider_task_map(
            _associate_from_config(cfg["tasks"], _deciders, "decider")),
        _trigger_task_map(
            _associate_from_config(cfg["tasks"], _triggers, "trigger")),
        _execution_process(execproc)
    {
        // nothing remains to be done here
    }

    /**
     * @brief Construct DataManager without config node from passed mappings
     *        only. If the last two arguments are not given, it is assumed that
     *        tasks, deciders, triggers  are of equal length and are to be
     *        associated in a one-to-one way in the order given. This order
     *        dependencey is also the reason why we make use of 'OrderedTaskMap'
     *        (an std::map) here.
     * @param tasks map that assigns names to shared_ptrs to tasks
     * @param deciders map that assigns names to shared_ptrs to deciders
     * @param triggers map that assigns names to shared_ptrs to triggers
     * @param execproc Function object that determines the execution process of
     *                 the datamanager, i.e., how the deciders, triggers and
     *                 tasks work together to produce the the output data
     * @param decider_task_assocs Map that assigns each task a decider function
     *                            by name: taskname->decidername
     * @param trigger_task_assocs Map that assigns each task a trigger function
     *                            by name: taskname->triggername
     */
    DataManager(OrderedTaskMap                       tasks,
                OrderedDeciderMap                    deciders,
                OrderedTriggerMap                    triggers,
                ExecutionProcess                     execproc,
                std::map< std::string, std::string > decider_task_assocs = {},
                std::map< std::string, std::string > trigger_task_assocs = {}) :
        // Get the global data manager logger
        _log(spdlog::get("data_mngr")),
        _tasks(TaskMap(tasks.begin(), tasks.end())),
        _deciders(DeciderMap(deciders.begin(), deciders.end())),
        _triggers(TriggerMap(triggers.begin(), triggers.end())),
        // Create maps: decider/trigger -> vector of task names
        _decider_task_map(_DMUtils::build_task_association_map< AssocsMap >(
            tasks, deciders, decider_task_assocs)),
        _trigger_task_map(_DMUtils::build_task_association_map< AssocsMap >(
            tasks, triggers, trigger_task_assocs)),
        _execution_process(execproc)
    {
        _log->info("DataManager setup with {} task(s), {} decider(s), and "
                   "{} trigger(s).",
                   _tasks.size(),
                   _decider_task_map.size(),
                   _trigger_task_map.size());
    }

    // .. Helper Methods ......................................................

    /**
     * @brief Exchange the state of the caller with the argument 'other'.
     *
     * @param other
     */
    void
    swap(DataManager& other)
    {
        if (this == &other)
        {
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

// ++ Default DataManager +++++++++++++++++++++++++++++++++++++++++++++++++++++

/**
 *  \addtogroup DataManagerDefaults Defaults
 *  \{
 *  \ingroup DataManager
 */

namespace Default
{
/**
 * @brief  A default DataManager type
 *         This uses all the default types for deciders, triggers, write task,
 *         and execution process
 *
 * @note   Cannot go anywhere else currently... Once possible, move to a more
 *         suitable location
 */
template < typename Model >
using DefaultDataManager =
    DataManager< DataManagerTraits< Default::DefaultWriteTask< Model >,
                                    Default::DefaultDecider< Model >,
                                    Default::DefaultTrigger< Model >,
                                    Default::DefaultExecutionProcess > >;
} // namespace Default

/**
 *  \}  // endgroup DataManager
 */

} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_DATAIO_DATAMANAGER_HH
