/** @file metaprogramming.hh
 *  @brief Functions emulating stl-algortihms, but for (heterogenous)
 * collections the types and size of which are known at compile time.
 */
#ifndef UTOPIA_CORE_COMPILETIME_ALGOS_HH
#define UTOPIA_CORE_COMPILETIME_ALGOS_HH
#include "type_traits.hh"

namespace Utopia {
// namespace for implementing tuple for each
namespace _Metaprogramming_helpers {
/**
 * @brief Prototype for apply_impl
 *
 * @tparam Metafunc
 * @tparam Tuplelike
 * @tparam X
 */
template<template<typename...> class Metafunc, typename Tuplelike, typename X>
struct apply_impl;

/**
 * @brief Metafunction which applies an arbitrary metafunction to a tuplelike
 * object - backend implementation
 *
 * @tparam Metafunc Metafunc to apply to Tuplelike type. Has to provide a member
 * alias 'type'.
 * @tparam Tuplelike Tuplelike type treatable with std::tuple_element
 * @tparam idxs indices used to get the elements of tuplelike
 */
template<template<typename...> class Metafunc,
         typename Tuplelike,
         std::size_t... idxs>
struct apply_impl<Metafunc, Tuplelike, std::index_sequence<idxs...>>
{
    // application of metafunction
    using type = typename Metafunc<
      typename std::tuple_element_t<idxs, Tuplelike>...>::type;
};

} // namespace _Metaprogramming_helpers

namespace Utils {


/**
 * @brief Apply the metafunction 'Metafunc' to a tuplelike type 'Tuplelike'.
 *
 * @tparam Metafunc A metafunction accepting as many template args as
 * 'Tuplelike' is big.
 * @tparam Tuplelike A tuplelike object which can be exploded into a parameter
 * pack with std::tuple_element
 */
template<template<typename...> class Metafunc, typename Tuplelike>
struct apply
{

    using type = typename _Metaprogramming_helpers::apply_impl<
      Metafunc,
      std::decay_t<Tuplelike>,
      std::make_index_sequence<std::tuple_size_v<std::decay_t<Tuplelike>>>>::type;
};

/**
 * @brief Alias for apply for applying a metafunction to a tuple
 *
 * @tparam Metafunc A metafunction accepting as many template args as
 * 'Tuplelike' is big.
 * @tparam Tuplelike A tuplelike object which can be exploded into a parameter
 * pack with std::tuple_element
 */
template<template<typename...> class Metafunc, typename Tuplelike>
using apply_t = typename apply<Metafunc, Tuplelike>::type;


} // namespace Utils
} // namespace Utopia
#endif
