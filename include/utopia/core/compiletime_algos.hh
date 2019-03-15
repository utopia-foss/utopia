/** @file compiletime_algos.hh
 *  @brief Functions emulating stl-algortihms, but for (heterogenous)
 * collections the types and size of which are known at compile time.
 */
#ifndef UTOPIA_CORE_COMPILETIME_ALGOS_HH
#define UTOPIA_CORE_COMPILETIME_ALGOS_HH
#include "utils.hh"

namespace Utopia
{
// namespace for implementing tuple for each
namespace _Compiletime_algos_helpers
{
/**
 * Take a bunch of tuples and applies the function f the tuple made up of the
 * idx-th entry of all tuples. This is needed to built the 'visit' and 'reduce'
 * funcitons, and hence not meant to be used individually.
 * @param  f              function to apply the idx-th tuple to.
 * @param  tuplelike      Arbitrary number of tuplelike objects
 * @return                result of f(get<idx>(tuplelike)...)
 */
template <std::size_t idx, typename Function, typename... Tuplelike>
constexpr auto apply_to_index(Function&& f, Tuplelike&&... tuplelike)
{
    using std::get; // enable Argument dependent lookup
    return std::apply(f, std::forward_as_tuple(get<idx>(tuplelike)...));
}

/**
 * Apply the function f to every tuple being made up of the idx-th entries in the
 * 'tuplelike' - objects, for the index range given by 'idxs...'. This is not
 * intended to be used individually, but implements functionality used in
 *  'reduce', 'visit', 'transform' and 'for_each'.
 * @method apply_to_indices
 * @param  f                Function to apply to each argument tuple
 * @param  std::index_sequence  compile time index sequence used to get out idx-th entry.
 * @param  tuplelike        Arbitrary number of tuplelike objects.
 * @return                  Either a tuple of the result of f(get<idx>(tuplelike)...)
 *                          for idx in (idxs...), or void.
 */
template <typename Function, std::size_t... idxs, typename... Tuplelike>
constexpr auto apply_to_indices(Function&& f, std::index_sequence<idxs...>, Tuplelike&&... tuplelike)
{
    using Returntype =
        std::invoke_result_t<Function, std::tuple_element_t<0, std::remove_reference_t<Tuplelike>>...>;

    if constexpr (std::is_same_v<Returntype, void>)
    {
        (apply_to_index<idxs>(std::forward<Function>(f), std::forward<Tuplelike>(tuplelike)...),
         ...);
    }
    else
    {
        // use aggregate initialization
        return std::tuple{apply_to_index<idxs>(
            std::forward<Function>(f), std::forward<Tuplelike>(tuplelike)...)...};
    }
}

// README: I tried to solve this with if constexpr, but was unable to get out
// the correct return type from the pad_to_size function. The enable_if approach
// taken below works, but is somewhat inelegant.

/**
 * Pad a scalar of type T to an array<T> of size N filled with copies of the
 * given value v.
 * @param  v Some value
 * @return  std::array<T, N>, filled with v
 */
template <std::size_t N, typename T, std::enable_if_t<Utils::get_size_v<std::remove_reference_t<T>> == 1, int> = 0>
constexpr auto pad_to_size(T&& v)
{
    std::array<std::remove_reference_t<T>, N> arr;
    arr.fill(v);
    return arr;
}

/**
 * Overload of pad_to_size for values v which have a compile time size > 1 and
 * hence have not to be padded. This is basically only a wrapper for
 * std::forward. The existence of this function enables the known behavior of
 * for_each to be able to modify the elements its iterating over.
 * @param  v value to forward
 * @return   reference to v via std::forward
 */
template <std::size_t N, typename T, std::enable_if_t<Utils::get_size_v<std::remove_reference_t<T>> != 1, int> = 0>
constexpr auto&& pad_to_size(T&& v)
{
    return std::forward<T>(v);
}

} // namespace _Compiletime_algos_helpers
namespace Utils
{
/**
 * Apply the  map-reduce idiom for an arbitrary collection of heterogeneous collections,
 *  the size of which is known at compile time. Takes the ith entry from each of the 'tuplelike'
 * objects and applies the function f to the argumet tuple formed by
 * (tuplelike[0][i], tuplelike[1][i],...). In python like pseudo code, this function
 * works like this:
 * for tgt, a,b,c,... in zip(t, tuplelike...):
 *     tgt = f(a, b, c, ...)
 *
 * The first of the collections in 'tuplelike...'. determines the size
 * of the iteration range, i.e. the number of applications of f.
 * If one of the given collections is a scalar, it is padded with copies of itself
 * to the size of the first element.
 * The first element should not be a scalar if this function should do anything useful,
 * because the iteration range will collapse to 1 in this case.
 * Giving inconsistently sized non-scalar collectionsis undefined behavior.
 * In order to use this with a Tuplelike type, the function 'Utils::get_size' has to be defined for it.
 * The iteration range is determined by the size of the first tuple in the 'Tuplelike'
 * parameter pack. If furthermore given arguments differ in size, this is undefined
 * behavior.
 * @param  f         Function to apply to the tuple of i-th entries of 'tuplelike'
 * @param  t         the target collection, has to be accessible via 'get', e.g. std::get,
 *                   but also user defined get which work similar.
 * @param  tuplelike Arbitrary number of compile time collections, have to be accessible
 *                   via std::get or similar user defined function.
 * @return           copy of the target object t
 */
template <typename NaryFunction, typename... Tuplelike>
constexpr auto reduce(NaryFunction&& f, Tuplelike&&... tuplelike)
{
    constexpr std::size_t N =
        Utils::get_size_v<std::tuple_element_t<0, std::tuple<std::remove_reference_t<Tuplelike>...>>>;

    if constexpr (N == 0)
    {
        return std::tuple<>();
    }
    else
    {
        return _Compiletime_algos_helpers::apply_to_indices(
            std::forward<NaryFunction>(f), std::make_index_sequence<N>(),
            _Compiletime_algos_helpers::pad_to_size<N>(std::forward<Tuplelike>(tuplelike))...);
    }
}

/**
 * Apply a function f to an argument tuple built by extracting the ith element
 * from each entry of the 'tuplelike...' parameter pack. Works similar to the
 * following python-like pseudo code:
 * for a,b,c... in zip(tuplelike...):
 *     f(a,b,c...)
 * In order to use this with a Tuplelike type, the function 'Utils::get_size'
 * has to be defined for it.
 * The iteration range is determined by the size of the first tuple in the 'Tuplelike'
 * parameter pack. If furthermore given arguments differ in size, this is undefined
 * behavior.
 * @param  f         Function accepting sizeof...(Tuplelike) many arguments.
 *                   Is applied to all ith entries, in the tuplelike-argument then.
 * @param  tuplelike Arbitrary number of collections to which can be processed by std::get
 * @return           the Function f
 */
template <typename NaryFunction, typename... Tuplelike>
constexpr NaryFunction visit(NaryFunction&& f, Tuplelike&&... tuplelike)
{
    constexpr std::size_t N =
        Utils::get_size_v<std::tuple_element_t<0, std::tuple<std::remove_reference_t<Tuplelike>...>>>;
    if constexpr (N == 0)
    {
        return f;
    }
    else
    {
        _Compiletime_algos_helpers::apply_to_indices(
            std::forward<NaryFunction>(f), std::make_index_sequence<N>(),
            _Compiletime_algos_helpers::pad_to_size<N>(std::forward<Tuplelike>(tuplelike))...);
        return f;
    }
}

/**
 * Analogon to std::for_each for heterogeneous collections. In order to use this
 * with a Tuplelike type, the function 'Utils::get_size' has to be defined for it.
 *
 * @param  f         function to apply to each element
 * @param  tuplelike Object, the elements of which can be accessed via std::get.
 * @return           the UnaryFunction f
 */
template <typename Tuplelike, typename UnaryFunction>
constexpr UnaryFunction for_each(Tuplelike&& tuplelike, UnaryFunction&& f)
{
    return visit(std::forward<UnaryFunction>(f), std::forward<Tuplelike>(tuplelike));
}

/**
 * Analogon to std::transform for heterogeneous collections or constexpr arrays.
 * In order to use this with a Tuplelike type, the function 'Utils::get_size'
 * has to be defined for it.
 * @param  f         function to apply to each element
 * @param  tuplelike Object, the elements of which can be accessed via std::get.
 * @return           the result of applying f to each element of 'tuplelike'
 */
template <typename Tuplelike, typename UnaryFunction>
constexpr auto transform(Tuplelike&& tuplelike, UnaryFunction&& f)
{
    return reduce(std::forward<UnaryFunction>(f), std::forward<Tuplelike>(tuplelike));
}

} // namespace Utils
} // namespace Utopia
#endif
