#ifndef UTOPIA_DATAIO_DATA_MANAGER_UTILS_HH
#define UTOPIA_DATAIO_DATA_MANAGER_UTILS_HH

#include <boost/hana/for_each.hpp>

#include "../../core/utils.hh"
#include "../cfg_utils.hh"

namespace Utopia {
namespace DataIO {
namespace _DMUtils {

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
 * class. 
 *
 */

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
template<class ValType, class KVPairs, class ObjMap>
void unpack_shared(KVPairs&& kv_pairs, ObjMap&& map) {
    using namespace Utopia::Utils;

    // Distinguish between tuple-like key value pairs and others
    if constexpr (is_tuple_like_v<remove_qualifier_t<KVPairs>>) {
        using std::get; // enable ADL

        // Build map from given key value pairs
        boost::hana::for_each(kv_pairs, [&](auto&& kv) {
            // Check if the deduced kv type is a base class of the
            // given target ValType
            if constexpr (std::is_base_of_v<
                            ValType,
                            std::tuple_element_t<
                                1, remove_qualifier_t<decltype(kv)>
                                >
                            >)
            {
                map[get<0>(kv)] =
                    std::make_shared<
                        std::tuple_element_t<
                            1, remove_qualifier_t<decltype(kv)>>
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
 * @tparam ObjMap  The name -> object map to create
 * @tparam ValType Used in make_shared<ValType> call
 * @tparam KVPairs Container of (key, value) pairs. Can also be a tuple.
 *
 * @param kv_pairs The container of (key, value) pairs to unpack into a
 *                 new map of type ObjMap
 *
 * @return ObjMap The newly created and populated map
 */
template<class ObjMap, class ValType, class KVPairs>
ObjMap unpack_shared(KVPairs&& kv_pairs) {
    ObjMap map;
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
 *
 * @tparam AssocsMap The association map type to create and populate. This
 *               should map identifiers to containers of other identifiers, for
 *               example: a string that maps to a container of task names.
 */
template<class AssocsMap, class NameTaskPairs, class NameCTPairs, class Assocs>
AssocsMap build_task_association_map(const NameTaskPairs& tasks,
                                     const NameCTPairs& nc_pairs,
                                     const Assocs& assocs)
{
    AssocsMap map;

    if (not assocs.size()) {
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
/*! @} */


} // namespace _DMUtils
} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_DATAIO_DATA_MANAGER_UTILS_HH
