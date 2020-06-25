#ifndef UTOPIA_DATAIO_DATA_MANAGER_UTILS_HH
#define UTOPIA_DATAIO_DATA_MANAGER_UTILS_HH

#include "../../core/type_traits.hh"
#include "../../core/zip.hh"
#include "../cfg_utils.hh"

namespace Utopia
{
namespace DataIO
{
namespace _DMUtils
{

/**
 *  \addtogroup DataManagerUtils Utilities
 *  \{
 *  \ingroup DataManager
 *  @details Here some utility functions for the datamanager are implemented.
 */

/**
 * @page
 * \section what Overview
 * This module provides auxilliary functions which are used in the datamanager
 * class and its associated factory classes
 *
 */

/**
 * @brief Metafunction for use with boost hana, check if all T are callable
 *        types
 *
 * @tparam T Parameter pack to check
 */
template < typename... T >
struct all_callable
    : std::conjunction< Utopia::Utils::is_callable< T >... >
{
};

/**
 * @brief Helper function to unpack (key, value) container into a map of
 *        shared pointers of a type.
 *
 * @tparam ValType Used in make_shared<ValType> call
 * @tparam KVPairs Container of (key, value) pairs. Can also be a tuple.
 * @tparam ObjMap  The name -> object map type
 *
 * @param kv_pairs The container of (key, value) pairs to unpack into a
 *                 new map of type ObjMap
 */
template < class ValType, class KVPairs, class ObjMap >
void
unpack_shared(KVPairs&& kv_pairs, ObjMap&& map)
{
    using namespace Utopia::Utils;

    // Distinguish between tuple-like key value pairs and others
    if constexpr (has_static_size_v< remove_qualifier_t< KVPairs > >)
    {
        using std::get; // enable ADL

        // Build map from given key value pairs
        boost::hana::for_each(kv_pairs, [&](auto&& kv) {
            // Check if the deduced kv type is a base class of the
            // given target ValType
            if constexpr (std::is_base_of_v<
                              ValType,
                              std::tuple_element_t<
                                  1,
                                  remove_qualifier_t< decltype(kv) > > >)
            {
                map[get< 0 >(kv)] = std::make_shared< std::tuple_element_t<
                    1,
                    remove_qualifier_t< decltype(kv) > > >(get< 1 >(kv));
            }
            else
            {
                map[get< 0 >(kv)] = std::make_shared< ValType >(get< 1 >(kv));
            }
        });
    }
    else
    {
        for (const auto& [k, v] : kv_pairs)
        {
            map[k] = std::make_shared< ValType >(v);
        }
    }
}

/**
 * @brief Helper function to unpack (key, value) container into an empty
 *        map of shared pointers of a type.
 *
 * @tparam ObjMap  The name -> object map to create
 * @tparam ValType Used in make_shared<ValType> call
 * @tparam KVPairs Container of (key, value) pairs. Can also be a tuple.
 *
 * @param kv_pairs The container of (key, value) pairs to unpack into a
 *                 new map of type ObjMap
 *
 * @return ObjMap The newly created and populated map
 */
template < class ObjMap, class ValType, class KVPairs >
ObjMap
unpack_shared(KVPairs&& kv_pairs)
{
    ObjMap map;
    unpack_shared< ValType >(kv_pairs, map);
    return map;
}

/**
 * @brief Build an association map, i.e., a map that associates a
 *        decider/trigger name with a collection of tasknames.
 * @details The Association Map is built from a map that
 *          associates names to tasks, a map that associates names and
 *          deciders/triggers, and a map or vector of pairs that associates
 *          *each taskname* with the name of a trigger/decider functor. If this
 *          last argument is not given, then a bijective association is
 *          attempted in which each task is associated with a trigger/decider
 *          that correponds to its position in the "named_dts" argument. This
 *          means that "tasks" and "named_dts" needs to be of equal length. If
 *          this is violated the function throws. If the "assocs" argument is
 *          given, the requirement of equal length is not necessary, because it
 *          specifies this bijective mapping. Note that this means that the
 *          "assocs" argument maps one decider/triggername to a taskname, and
 *          tasknames may repeat, while decider/triggernames do not.
 *
 * @tparam AssocsMap Final map that maps names of deciders/triggers to a
 *                   collection of task names
 * @tparam NamedTasks automatically determined
 * @tparam NamedDTMap automatically determined
 * @tparam Assocs  automatically determined
 * @param tasks map or vector of pairs containing (name, task).
 * @param named_dts  map or vector of pairs containing (name, decider/trigger)
 * @param assocs map or vector of pairs containing (taskname,
 *               decider-/triggername).
 * @return AssocsMap Map that maps a name of a decider/trigger to a vector of
 *                   tasknames. This argument is optional: If it is not given,
 *                   then the function will try to associate tasks and
 *                   deciders/triggers one by one in a bijective way in the
 *                   order given. If that fails, an error is thrown.
 */
template < class AssocsMap, class NamedTasks, class NamedDTMap, class Assocs >
AssocsMap
build_task_association_map(const NamedTasks& tasks,
                           const NamedDTMap& named_dts,
                           Assocs            assocs = Assocs{})
{

    AssocsMap map;

    // Check if helpers and tasks can be associated one by one bijectivly, if
    // not, demand a map explicitly giving associations to be given, which can
    // be used for association

    if (tasks.size() != named_dts.size())
    {
        if (assocs.size() == 0)
        {
            throw std::invalid_argument(
                "Error, explicit associations have to be given when mapping "
                "unequal numbers of decider or trigger functions and tasks.");
        }
        else
        {
            for (auto&& [taskname, dt_name] : assocs)
            {
                map[dt_name].push_back(taskname);
            }
        }
    }
    else
    {

        for (auto&& [namedtask, namedhelper] : Itertools::zip(tasks, named_dts))
        {
            map[namedhelper.first].push_back(namedtask.first);
        }
    }

    return map;
}

/*! @} */

} // namespace _DMUtils
} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_DATAIO_DATA_MANAGER_UTILS_HH
