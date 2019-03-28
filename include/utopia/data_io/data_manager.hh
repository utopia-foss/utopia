#ifndef UTOPIA_DATAIO_DATAMANAGER_HH
#define UTOPIA_DATAIO_DATAMANAGER_HH

// stl includes for having shared_ptr
// hashmap, vector, swap
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <vector>

// boost hana headers used for constructing
// Tasks from argument tuples
#include <boost/hana/for_each.hpp>
#include <boost/hana/range.hpp>
#include <boost/hana/slice.hpp>
#include <boost/hana/tuple.hpp>

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
     * @param deciders  Container of (name, decider function) pairs
     * @param triggers  Container of (name, trigger function) pairs
     * @param tasks     Container of (name, Task) pairs
     * @param decider_task_map  Mapping from decider names to task names that
     *                  are using these deciders
     * @param trigger_task_map  Mapping from trigger names to task names that
     *                  are using these triggers
     */
    DataManager(Model& model,
                std::vector<std::pair<std::string, Task>> tasks,
                std::vector<std::pair<std::string, Decider>> deciders,
                std::vector<std::pair<std::string, Trigger>> triggers,
                std::vector<std::pair<std::string, std::string>> decider_task_map = {},
                std::vector<std::pair<std::string, std::string>> trigger_task_map = {})
        : _log(model.get_logger()),
          _tasks([&]() {
              TaskContainer tc;
              for (auto& [name, t] : tasks)
              {
                  tc[name] = std::make_shared<Task>(t);
              }
              return tc;
          }()),
          _deciders([&]() {
              DeciderContainer dc;
              for (auto& [name, d] : deciders)
              {
                  dc[name] = std::make_shared<Decider>(d);
              }
              return dc;
          }()),
          _triggers([&]() {
              TriggerContainer tc;
              for (auto& [name, t] : triggers)
              {
                  tc[name] = std::make_shared<Trigger>(t);
              }
              return tc;
          }()),
          _decider_task_map([&]() {
              // if no association is given, a 1 to 1 association is assumed
              // if this does not work out, throw exception
              if (decider_task_map.size() == 0 and deciders.size() != tasks.size())
              {
                  throw std::invalid_argument(
                      "deciders size != tasks size! You have to disambiguate "
                      "the association of deciders and write tasks by "
                      "supplying an explicit decider_task_map if you want to "
                      "have an unequal number of tasks and deciders.");
              }
              else if (decider_task_map.size() == 0)
              {
                  // when no explicit name association is given but the
                  // tasks and deciders are equal in number, associate them
                  // one to one
                  NamingMap dm;

                  auto d_it = _deciders.begin();
                  auto t_it = _tasks.begin();

                  // associate the names of deciders to the names of tasks
                  // in a  1 to 1 fashion
                  for (; d_it != _deciders.end() && t_it != _tasks.end(); ++d_it, ++t_it)
                  {
                      dm[d_it->first].push_back(t_it->first);
                  }

                  return dm;
              }
              else
              {
                  // else built the association from the given naming
                  NamingMap dm;
                  for (auto& [dname, tname] : decider_task_map)
                  {
                      dm[dname].push_back(tname);
                  }
                  return dm;
              }
          }()),
          _trigger_task_map([&]() {
              // if no association is given, a 1 to 1 association is assumed
              // if this does not work out, throw exception
              if (trigger_task_map.size() == 0 and triggers.size() != tasks.size())
              {
                  throw std::invalid_argument(
                      "triggers size != tasks size! You have to disambiguate "
                      "the association of triggers and write tasks by "
                      "supplying an explicit trigger_task_map if you want to "
                      "have an unequal number of tasks and triggers.");
              }
              else if (trigger_task_map.size() == 0)
              {
                  // when no explicit name association is given but the
                  // tasks and triggers are equal in number, associate them
                  // one to one
                  NamingMap tm;
                  auto d_it = _triggers.begin();
                  auto t_it = _tasks.begin();

                  // associate the names of deciders to the names of
                  // tasks in a  1 to 1 fashion
                  for (; d_it != _triggers.end() && t_it != _tasks.end(); ++d_it, ++t_it)
                  {
                      tm[d_it->first].push_back(t_it->first);
                  }
                  return tm;
              }
              else
              {
                  // built from given association
                  NamingMap tm;
                  for (auto& [trname, tname] : trigger_task_map)
                  {
                      tm[trname].push_back(tname);
                  }
                  return tm;
              }
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
     * @param decider_task_map  Mapping from decider names to task names that
     *                  are using these deciders
     * @param trigger_task_map  Mapping from trigger names to task names that
     *                  are using these triggers
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
                std::vector<std::pair<std::string, std::string>> decider_task_map = {},
                std::vector<std::pair<std::string, std::string>> trigger_task_map = {})
        : _log(model.get_logger()),
          // quite some copy pasta from above, but no other way seen...
          _tasks([&]() {
              using std::get; // enable ADL
              TaskContainer tc;
              // build map from given type containing pairs

              Utopia::Utils::for_each(tasks, [&](auto&& taskpair) {
                  // check if the deduced Task type is a base class of the given type
                  if constexpr (std::is_base_of_v<Task, std::tuple_element_t<1, Utils::remove_qualifier_t<decltype(taskpair)>>>)
                  {
                      tc[get<0>(taskpair)] =
                          std::make_shared<std::tuple_element_t<1, Utils::remove_qualifier_t<decltype(taskpair)>>>(
                              get<1>(taskpair));
                  }
                  else
                  {
                      tc[get<0>(taskpair)] = std::make_shared<Task>(get<1>(taskpair));
                  }
              });
              return tc;
          }()),
          _deciders([&]() {
              using std::get; // enable adl
              // build map from given type containing pairs

              DeciderContainer dc;
              Utopia::Utils::for_each(deciders, [&](auto&& deciderpair) {
                  // check if the deduced Decider type is a base class of the given type
                  if constexpr (std::is_base_of_v<Decider, std::tuple_element_t<1, Utils::remove_qualifier_t<decltype(deciderpair)>>>)
                  {
                      dc[get<0>(deciderpair)] =
                          std::make_shared<std::tuple_element_t<1, Utils::remove_qualifier_t<decltype(deciderpair)>>>(
                              get<1>(deciderpair));
                  }
                  else
                  {
                      dc[get<0>(deciderpair)] =
                          std::make_shared<Decider>(get<1>(deciderpair));
                  }
              });

              return dc;
          }()),
          _triggers([&]() {
              using std::get; // enable ADL

              // build map from given type containing pairs
              TriggerContainer tc;
              Utopia::Utils::for_each(triggers, [&](auto&& triggerpair) {
                  // check if the deduced Trigger type is a base class of the given type
                  if constexpr (std::is_base_of_v<Trigger, std::tuple_element_t<1, Utils::remove_qualifier_t<decltype(triggerpair)>>>)
                  {
                      tc[get<0>(triggerpair)] =
                          std::make_shared<std::tuple_element_t<1, Utils::remove_qualifier_t<decltype(triggerpair)>>>(
                              get<1>(triggerpair));
                  }
                  else
                  {
                      tc[get<0>(triggerpair)] =
                          std::make_shared<Trigger>(get<1>(triggerpair));
                  }
              });

              return tc;
          }()),
          _decider_task_map([&]() {
              // if no association is given, a 1 to 1 association is assumed
              // if this does not work out, throw exception
              if (decider_task_map.size() == 0 and
                  Utils::get_size_v<Deciders> != Utils::get_size_v<Tasks>)
              {
                  throw std::invalid_argument(
                      "deciders size != tasks size! You have to disambiguate "
                      "the association of deciders and write tasks by "
                      "supplying an explicit decider_task_map if you want to "
                      "have an unequal number of tasks and deciders.");
              }
              else if (decider_task_map.size() == 0)
              {
                  // when no explicit name association is given but the
                  // tasks and deciders are equal in number, associate them
                  // one to one
                  NamingMap dm;

                  auto d_it = _deciders.begin();
                  auto t_it = _tasks.begin();

                  // associate the names of deciders to the names of tasks
                  // in a  1 to 1 fashion
                  for (; d_it != _deciders.end() && t_it != _tasks.end(); ++d_it, ++t_it)
                  {
                      dm[d_it->first].push_back(t_it->first);
                  }

                  return dm;
              }
              else
              {
                  // else built the association from the given naming
                  NamingMap dm;
                  for (auto& [dname, tname] : decider_task_map)
                  {
                      dm[dname].push_back(tname);
                  }
                  return dm;
              }
          }()),
          _trigger_task_map([&]() {
              // if no association is given, a 1 to 1 association is assumed
              // if this does not work out, throw exception
              if (trigger_task_map.size() == 0 and
                  Utils::get_size_v<Triggers> != Utils::get_size_v<Tasks>)
              {
                  throw std::invalid_argument(
                      "triggers size != tasks size! You have to disambiguate "
                      "the association of triggers and write tasks by "
                      "supplying an explicit trigger_task_map if you want to "
                      "have an unequal number of tasks and triggers.");
              }
              else if (trigger_task_map.size() == 0)
              {
                  // when no explicit name association is given but the
                  // tasks and triggers are equal in number, associate them
                  // one to one
                  NamingMap tm;
                  auto d_it = _triggers.begin();
                  auto t_it = _tasks.begin();

                  // associate the names of deciders to the names of
                  // tasks in a  1 to 1 fashion
                  for (; d_it != _triggers.end() && t_it != _tasks.end(); ++d_it, ++t_it)
                  {
                      tm[d_it->first].push_back(t_it->first);
                  }
                  return tm;
              }
              else
              {
                  // built from given association
                  NamingMap tm;
                  for (auto& [trname, tname] : trigger_task_map)
                  {
                      tm[trname].push_back(tname);
                  }
                  return tm;
              }
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
