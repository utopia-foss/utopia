#ifndef UTOPIA_DATAIO_DATAMANAGER_HH
#define UTOPIA_DATAIO_DATAMANAGER_HH

// stl includes for having shared_ptr
// hashmap, vector, swap
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <vector>

// utopia includes
#include "cfg_utils.hh"
#include "../core/compiletime_algos.hh"
#include "../core/logging.hh"
#include "../core/utils.hh"

namespace Utopia
{
namespace DataIO
{

using namespace Utopia::Utils;
/**
 * @brief Store arbitrarily many write procedures and execute them upon demand.
 *        A write procedure consists of: 
 *        - A callable 'Decider' which returns a boolean telling if a write operation should happen or not. 
 *        - A callable 'Trigger' which returns a boolean telling if the currently used dataset shall be changed/
 *          a new one generated.
 *        - A struct/class Task which has public callables 'write' and 'build_dataset', which are invoked when 
 *          the associated decider or trigger returns true, respectively. 
 *        Each Decider and Trigger is associated with arbitrarily many tasks, such that one decider/trigger 
 *        can manage the behavior of many tasks. 
 *
 * @tparam Model   Class representing the source of data to write and conditions used to
 *                 determine when to write (decider) and when to build a new dataset
 *                 (trigger). It also has to be supplied to the call operator of this class.
 *
 * @tparam Task A class type which takes care of dataset creation/opening/closing and writing
 * @tparam Decider Callable returning a boolean which tells when a task should write data
 * @tparam Trigger Callable returning a boolean which tells when a task should open a new dataset
 */
template <class Model, class Task, class Decider, class Trigger>
class DataManager
{
public:
    using TaskContainer = std::unordered_map<std::string, std::shared_ptr<Task>>;
    using DeciderContainer = std::unordered_map<std::string, std::shared_ptr<Decider>>;
    using TriggerContainer = std::unordered_map<std::string, std::shared_ptr<Trigger>>;
    using NamingMap = std::unordered_map<std::string, std::vector<std::string>>;

protected:
    /**
     * @brief Used to inform about progress of DataManager operations
     *
     */
    std::shared_ptr<spdlog::logger> _log;

    /**
     * @brief Stores (name, task) pairs in an unordered map.
     *
     */
    TaskContainer _tasks;

    /**
     * @brief Stores (name, deciderfunction) pairs in an unordered map.
     *
     */
    DeciderContainer _deciders;

    /**
     * @brief Stores (name, triggerfunction) pairs in an unordered map.
     *
     */
    TriggerContainer _triggers;

    /**
     * @brief Mapping from deciders names to containers of names of tasks that
     *        use those deciders
     */
    NamingMap _decider_task_map;

    /**
     * @brief Mapping from trigger names to containers of names of tasks that
     *        use those triggers
     */
    NamingMap _trigger_task_map;

public:


    // WIP prototype for call operator
    template <typename... Args>
    void operator()(Model&, Args&&...);


    /**
     * @brief Register a new task and its name with the datamanager.
     *
     * @tparam Tasktype Type of the task to register
     * @param name name of the task to register
     * @param new_task task object to register
     */
    template<typename Tasktype>
    void register_task(std::string name, Tasktype&& new_task)
    {
        _tasks[name] = std::make_shared<Tasktype>(std::forward<Tasktype>(new_task));
    }

    /**
     * @brief Register a new trigger and its name with the datamanager.
     *
     * @tparam Triggertype Type of the trigger to register
     * @param name name of the new trigger
     * @param new_trigger  new trigger object to register
     */
    template <typename Triggertype>
    void register_trigger(std::string name, Triggertype&& new_trigger)
    {
        _triggers[name] = std::make_shared<Triggertype>(std::forward<Triggertype>(new_trigger));
    }

    /**
     * @brief Register a new decider and its name with the datamanager.
     * 
     * @tparam Decidertype Type of the decider to register
     * @param name name of the new decider
     * @param new_decider new decider object to register
     */
    template <typename Decidertype>
    void register_decider(std::string name, Decidertype&& new_decider)
    {
        _deciders[name] =
            std::make_shared<Decidertype>(std::forward<Decidertype>(new_decider));
    }

    /**
     * @brief Register a trigger->write->task procedure with the datamanager
     *        The task will be associated with the given trigger and decider.
     *       
     * @param task_name name of the newly added task
     * @param ts Task to add
     * @param decider_name  name of the newly added decider
     * @param dc Decider to add
     * @param trigger_name name of the newly added trigger
     * @param tg Trigger to add
     */
    template <typename Tsk, typename Dcd, typename Trgr>
    void register_procedure(std::string task_name,
                            Tsk&& ts,
                            std::string decider_name,
                            Dcd&& dc,
                            std::string trigger_name,
                            Trgr&& tg)
    {
        // TODO Make this use the register_* methods! Include checks there.

        if (_tasks.find(task_name) != _tasks.end())
        {
            this->_log->warn("Task name '{}' already exists! Not registering.",
                             task_name);
        }
        else
        {
            _tasks[task_name] = std::make_shared<Tsk>(std::forward<Tsk>(ts));
        }

        if (_triggers.find(trigger_name) != _triggers.end())
        {
            this->_log->warn("Trigger name '{}' already exists! Not "
                             "registering.", trigger_name);
        }
        else
        {
            _triggers[trigger_name] = std::make_shared<Trgr>(std::forward<Trgr>(tg));
        }

        if (_deciders.find(decider_name) != _deciders.end())
        {
            this->_log->warn("Decider name '{}' already exists! Not "
                             "registering.", decider_name);
        }
        else
        {
            _deciders[decider_name] = std::make_shared<Dcd>(std::forward<Dcd>(dc));
        }

        _decider_task_map[decider_name].push_back(task_name);
        _trigger_task_map[trigger_name].push_back(task_name);
    }

    /**
     * @brief Reassign a task to a different decider. Nonexistant values for 
     *        the at least one argument is undefined behavior.
     *
     * @param decidername Name of the decider to associate the task to
     * @param taskname Name of the task to reassociate
     */
    void link_task_to_decider(std::string taskname,
                              std::string decidername,
                              std::string old_decidername = "")
    {
        // old decidername not given -> no previous association
        if (old_decidername.size() == 0)
        {
            _decider_task_map[decidername].push_back(taskname);
        }
        else
        {
            // delete from the old decider's task name vector
            _decider_task_map[old_decidername].erase(
                std::remove_if(_decider_task_map[old_decidername].begin(),
                               _decider_task_map[old_decidername].end(),
                               [&](std::string& s) { return s == taskname; }),
                _decider_task_map[old_decidername].end());


            // and add to the new one
            _decider_task_map[decidername].push_back(taskname);
        }
    }

    /**
     * @brief Reassign a task to a different trigger. Nonexistant values for
     *        the at least one argument is undefined behavior.
     *
     * @param triggername Name of the trigger to associate the task to
     * @param taskname Name of the task to reassociate
     */
    void link_task_to_trigger(std::string taskname,
                              std::string triggername,
                              std::string old_triggername = "")
    {
        // old triggername not given -> no previous association
        if (old_triggername.size() == 0)
        {
            _trigger_task_map[triggername].push_back(taskname);
        }
        else
        {
            _trigger_task_map[old_triggername].erase(
                std::remove_if(_trigger_task_map[old_triggername].begin(),
                               _trigger_task_map[old_triggername].end(),
                               [&](std::string& s) { return s == taskname; }),
                _trigger_task_map[old_triggername].end());

            // and add to the new one
            _trigger_task_map[triggername].push_back(taskname);
        }
    }


    // .. Getters .............................................................

    /**
     * @brief Get the container of decider objects
     *
     * @return const DeciderContainer&
     */
    const DeciderContainer& get_deciders() const
    {
        return _deciders;
    }

    /**
     * @brief Get the container of task objects
     *
     * @return const TaskContainer&
     */
    const TaskContainer& get_tasks() const
    {
        return _tasks;
    }

    /**
     * @brief Get the container of trigger objects
     *
     * @return const TriggerContainer&
     */
    const TriggerContainer& get_triggers() const
    {
        return _triggers;
    }

    /**
     * @brief Get the decider task map object
     *
     * @return const NamingMap&
     */
    const NamingMap& get_decider_task_map() const
    {
        return _decider_task_map;
    }

    /**
     * @brief Get the trigger task map object
     *
     * @return const NamingMap&
     */
    const NamingMap& get_trigger_task_map() const
    {
        return _trigger_task_map;
    }

    /**
     * @brief Get the logger used in this DataManager
     *
     * @return const std::shared_ptr<spdlog::logger>&
     */
    const std::shared_ptr<spdlog::logger>& get_logger() const
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
     * @brief Construct a new DataManager object
     *
     * @param model     The model this DataManager is to be associated with
     * @param cfg       The data manager configuration
     * @param tasks     Container of (name, Task) pairs
     * @param add_deciders  Container of (name, decider function) pairs that
     *                  are made available _additionally_
     * @param add_triggers  Container of (name, trigger function) pairs that
     *                  are made available _additionally_
     */
    DataManager(Model& model,
                const Config& cfg,
                std::vector<std::pair<std::string, Task>> tasks,
                std::vector<std::pair<std::string, Decider>> add_deciders = {},
                std::vector<std::pair<std::string, Trigger>> add_triggers = {}
                )
        :
        // Get whatever is needed from the model
        _log(model.get_logger()),

        // Unpack tasks, deciders, and triggers into the respective containers
        _tasks(unpack_shared<TaskContainer, Task>(tasks)),
        _deciders(setup_deciders(cfg, add_deciders)),
        _triggers(setup_triggers(cfg, add_triggers)),

        // Set up task association mappings empty; set in body
        _decider_task_map{},
        _trigger_task_map{}
        {
            update_decider_and_trigger_maps(cfg);
        }


    // TODO Implement tuple-argument constructor accepting config node

    /**
     * @brief Construct a new DataManager object
     *
     * @param model     The model this DataManager is to be associated with
     * @param deciders  Container of (name, decider function) pairs
     * @param triggers  Container of (name, trigger function) pairs
     * @param tasks     Container of (name, Task) pairs
     * @param task_decider_assocs  Container of task -> decider association
     *                  pairs, i.e. (task name, decider name) pairs
     * @param task_trigger_assocs  Container of task -> trigger association
     *                  pairs, i.e. (task name, trigger name) pairs
     */
    DataManager(Model& model,
                std::vector<std::pair<std::string, Task>> tasks,
                std::vector<std::pair<std::string, Decider>> deciders,
                std::vector<std::pair<std::string, Trigger>> triggers,
                std::vector<std::pair<std::string, std::string>> task_decider_assocs = {},
                std::vector<std::pair<std::string, std::string>> task_trigger_assocs = {})
        :
        // Get whatever is needed from the model
        _log(model.get_logger()),

        // Unpack tasks, deciders, and triggers into the respective containers
        _tasks(unpack_shared<TaskContainer, Task>(tasks)),
        _deciders(unpack_shared<DeciderContainer, Decider>(deciders)),
        _triggers(unpack_shared<TriggerContainer, Trigger>(triggers)),

        // Create maps: decider/trigger -> vector of task names
        _decider_task_map([&]() {
            // Check if there would be issues in 1-to-1 association
            if (    task_decider_assocs.size() == 0
                and deciders.size() != tasks.size())
            {
                throw std::invalid_argument(
                    "deciders size != tasks size! You have to disambiguate "
                    "the association of deciders and write tasks by "
                    "supplying an explicit task_decider_assocs argument if "
                    "you want to have an unequal number of tasks and "
                    "deciders.");
            }
            return task_association_map_from(tasks, deciders,
                                             task_decider_assocs);
        }()),
        _trigger_task_map([&]() {
            // Check if there would be issues in 1-to-1 association
            if (    task_trigger_assocs.size() == 0
                and triggers.size() != tasks.size())
            {
                throw std::invalid_argument(
                    "triggers size != tasks size! You have to disambiguate "
                    "the association of triggers and write tasks by "
                    "supplying an explicit task_trigger_assocs argument if "
                    "you want to have an unequal number of tasks and "
                    "triggers.");
            }
            return task_association_map_from(tasks, triggers,
                                             task_trigger_assocs);
        }())
    {
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
    template <class M,
              class Tasks,
              class Deciders,
              class Triggers,
              std::enable_if_t<Utopia::Utils::is_tuple_like_v<Deciders> && Utopia::Utils::is_tuple_like_v<Triggers> && Utopia::Utils::is_tuple_like_v<Tasks>, int> = 0>
    DataManager(M& model,
                Tasks&& tasks,
                Deciders&& deciders,
                Triggers&& triggers,
                std::vector<std::pair<std::string, std::string>> task_decider_assocs = {},
                std::vector<std::pair<std::string, std::string>> task_trigger_assocs = {})
        :
        // Get whatever is needed from the model
        _log(model.get_logger()),

        // Unpack tasks, deciders, and triggers into the respective containers
        _tasks(unpack_shared<TaskContainer, Task>(tasks)),
        _deciders(unpack_shared<DeciderContainer, Decider>(deciders)),
        _triggers(unpack_shared<TriggerContainer, Trigger>(triggers)),

        // Create maps: decider/trigger -> vector of task names
        _decider_task_map([&]() {
            // Check if there would be issues in 1-to-1 association
            if (    task_decider_assocs.size() == 0
                and Utils::get_size_v<Deciders> != Utils::get_size_v<Tasks>)
            {
                throw std::invalid_argument(
                    "deciders size != tasks size! You have to disambiguate "
                    "the association of deciders and write tasks by "
                    "supplying an explicit task_decider_assocs argument if "
                    "you want to have an unequal number of tasks and "
                    "deciders.");
            }
            return task_association_map_from(_tasks, _deciders,
                                             task_decider_assocs);
            // FIXME _tasks and _deciders are not necessarily ordered in the
            //       same way!
        }()),
        _trigger_task_map([&]() {
            // Check if there would be issues in 1-to-1 association
            if (    task_trigger_assocs.size() == 0
                and Utils::get_size_v<Triggers> != Utils::get_size_v<Tasks>)
            {
                throw std::invalid_argument(
                    "triggers size != tasks size! You have to disambiguate "
                    "the association of triggers and write tasks by "
                    "supplying an explicit task_trigger_assocs argument if "
                    "you want to have an unequal number of tasks and "
                    "triggers.");
            }
            return task_association_map_from(_tasks, _triggers,
                                             task_trigger_assocs);
            // FIXME _tasks and _triggers are not necessarily ordered in the
            //       same way!
        }())
    {
    }


    // .. Helper Methods ......................................................

    /**
     * @brief Exchange the state of the caller with the argument 'other'.
     *
     * @param other
     */
    void swap(DataManager& other)
    {
        if (this == &other)
        {
            return;
        }
        else
        {
            using std::swap;
            swap(_log, other._log);
            swap(_tasks, other._tasks);
            swap(_deciders, other._deciders);
            swap(_triggers, other._triggers);
            swap(_decider_task_map, other._decider_task_map);
            swap(_trigger_task_map, other._trigger_task_map);
        }
    }

private:
    // .. Construction Helpers ................................................
    /**
     * @brief Helper function to unpack (key, value) container into a map of
     *        shared pointers of a type.
     *
     * @tparam ValType Used in make_shared<ValType> call
     * @tparam KVPairs Container of (key, value) pairs. Can also be a tuple.
     * @tparam MapType The map to create
     *
     * @param kv_pairs The container of (key, value) pairs to unpack into a
     *                 new map of type MapType
     */
    template<class ValType, class KVPairs, class MapType>
    void unpack_shared(KVPairs& kv_pairs, MapType& map) {
        // Distinguish between tuple-like key value pairs and others
        if constexpr (Utils::is_tuple_like_v<KVPairs>) {
            using std::get; // enable ADL

            // Build map from given key value pairs
            // NOTE Requires kv_pairs to NOT be const! Otherwise, compilation
            //      will eat all your RAM and will literally never end.
            Utils::for_each(kv_pairs, [&](auto&& kv) {
                // Check if the deduced kv type is a base class of the
                // given target ValType
                if constexpr (std::is_base_of_v<ValType, std::tuple_element_t<1, Utils::remove_qualifier_t<decltype(kv)>>>)
                {
                    map[get<0>(kv)] =
                        std::make_shared<
                            std::tuple_element_t<1,
                                Utils::remove_qualifier_t<decltype(kv)>>
                        >(get<1>(kv));
                }
                else {
                    map[get<0>(kv)] = std::make_shared<ValType>(get<1>(kv));
                }
            });
        }
        else {
            for (const auto& [k, v] : kv_pairs) {
                map[k] = std::make_shared<ValType>(v);
            }
        }
    }

    /**
     * @brief Helper function to unpack (key, value) container into an empty
     *        map of shared pointers of a type.
     *
     * @tparam MapType The map to create
     * @tparam ValType Used in make_shared<ValType> call
     * @tparam KVPairs Container of (key, value) pairs. Can also be a tuple.
     *
     * @param kv_pairs The container of (key, value) pairs to unpack into a
     *                 new map of type MapType
     *
     * @return MapType The newly created and populated map
     */
    template<class MapType, class ValType, class KVPairs>
    MapType unpack_shared(KVPairs& kv_pairs) {
        MapType map;
        unpack_shared<ValType>(kv_pairs, map);
        return map;
    }

    /**
     * @brief Helper function to generate a decider/trigger -> task name map
     *
     * @param tasks  An iterable of (name, task) pairs of which the name is
     * @param nc_pairs An iterable of (name, callable) pairs of which the name
     *               is associated with 
     * @param assocs An iterable (decider/trigger name, task name)
     */
    template<class NameTaskPairs, class NameCTPairs, class Assocs>
    auto task_association_map_from(const NameTaskPairs& tasks,
                                   const NameCTPairs& nc_pairs,
                                   const Assocs& assocs)
    {
        NamingMap map;

        if (assocs.size() == 0) {
            // When no explicit name association is given but the tasks and
            // deciders/triggers are equal in number, associate them in a
            // 1-to-1 fashion.
            auto nc_it = nc_pairs.begin();
            auto t_it = tasks.begin();
            
            for (; nc_it != nc_pairs.end() && t_it != tasks.end();
                 ++nc_it, ++t_it)
            {
                map[nc_it->first].push_back(t_it->first);
            }
        }
        else {
            // Build from association, inverting order such that the map
            // has as keys the callable names.
            for (const auto& [task_name, callable_name] : assocs) {
                map[callable_name].push_back(task_name);
            }
        }

        return map;
    }


    /**
     * @brief Set up default deciders and user-specified additional deciders
     */
    template<class KVPairs>
    DeciderContainer setup_deciders(const Config&, KVPairs& add_deciders) {
        // Generate a map of default deciders
        DeciderContainer deciders;

        // TODO Add deciders here

        // Add the additional deciders to it, overwriting existing ones
        unpack_shared<Decider>(add_deciders, deciders);

        return deciders;
    }

    /**
     * @brief Set up default triggers and user-specified additional triggers
     */
    template<class KVPairs>
    TriggerContainer setup_triggers(const Config&, KVPairs& add_triggers) {
        // Generate a map of default triggers
        DeciderContainer triggers;

        // Add the additional triggers to it, overwriting existing ones
        unpack_shared<Decider>(add_triggers, triggers);

        return triggers;
    }

    /**
     * @brief TODO
     */
    void update_decider_and_trigger_maps(const Config&) {
        // WIP
    }
};

/**
 * @brief Exchange the state of 'lhs' and 'rhs'.
 *
 * @tparam Model   Class representing the source of data to write and data used to
 *                 determine when to write (decider) and when to build a new dataset
 *                 (trigger).
 * @tparam Decider Callable returning a boolean which tells when a task should write data
 * @tparam Trigger Callable returning a boolean which tells when a task should built a new dataset
 * @tparam Task Callable which has a reference to a hdfgroup builds datasets and writes data on demand.
 */
template <class Model, class Task, class Decider, class Trigger>
void swap(DataManager<Model, Task, Decider, Trigger>& lhs,
          DataManager<Model, Task, Decider, Trigger>& rhs)
{
    lhs.swap(rhs);
}

/**
 * @brief Helper metafunction for removing the first element of the
 *
 * @tparam T
 * @tparam Ts
 */
template <typename... Ts>
struct RemoveFirst
{
    using type = std::tuple<std::tuple_element_t<1, Ts>...>;
};

/// Deduction guides for DataManager constructor
template <class Model, class Task, class Decider, class Trigger>
DataManager(Model& model,
            std::vector<std::pair<std::string, Task>> tasks,
            std::vector<std::pair<std::string, Decider>> deciders,
            std::vector<std::pair<std::string, Trigger>> triggers,
            std::vector<std::pair<std::string, std::string>> decider_task_map = {},
            std::vector<std::pair<std::string, std::string>> trigger_task_map = {})
    ->DataManager<Utopia::Utils::remove_qualifier_t<decltype(model)>,
                  typename std::vector<std::pair<std::string, Task>>::value_type::second_type,
                  typename std::vector<std::pair<std::string, Decider>>::value_type::second_type,
                  typename std::vector<std::pair<std::string, Trigger>>::value_type::second_type>;

/// Deduction guides for DataManager constructor
template <class M,
          class Tasks,
          class Deciders,
          class Triggers,
          std::enable_if_t<Utopia::Utils::is_tuple_like_v<Deciders> && Utopia::Utils::is_tuple_like_v<Triggers> && Utopia::Utils::is_tuple_like_v<Tasks>, int> = 0>
DataManager(M& model,
            Tasks&& tasks,
            Deciders&& deciders,
            Triggers&& triggers,
            std::vector<std::pair<std::string, std::string>> decider_task_map = {},
            std::vector<std::pair<std::string, std::string>> trigger_task_map = {})
    ->DataManager<Utopia::Utils::remove_qualifier_t<decltype(model)>,
                  Utopia::Utils::apply_t<std::common_type, Utopia::Utils::apply_t<RemoveFirst, Tasks>>,
                  Utopia::Utils::apply_t<std::common_type, Utopia::Utils::apply_t<RemoveFirst, Deciders>>,
                  Utopia::Utils::apply_t<std::common_type, Utopia::Utils::apply_t<RemoveFirst, Triggers>>>;

} // namespace DataIO
} // namespace Utopia
#endif
