#ifndef HDFUTILITIES_HH
#define HDFUTILITIES_HH

#include <array>
#include <deque>
#include <forward_list>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <vector>

// Functions for determining if a type is an STL-container are provided here.
// This is used if we wish to make hdf5 types for storing such data in an
// hdf5 dataset.
namespace Utopia {
namespace DataIO {

// Prototype for determining if something is a container or not
template <typename T> struct is_container : public std::false_type {};

// Providing specializations for STL containers
// Currently, none provided for associative containers.
// FIXME: why does this even work with variadics??
template <typename T, std::size_t N>
struct is_container<std::array<T, N>> : public std::true_type {};

template <typename... Args>
struct is_container<std::vector<Args...>> : public std::true_type {};

template <typename... Args>
struct is_container<std::list<Args...>> : public std::true_type {};

template <typename... Args>
struct is_container<std::forward_list<Args...>> : public std::true_type {};

template <typename... Args>
struct is_container<std::deque<Args...>> : public std::true_type {};

template <typename... Args>
struct is_container<std::queue<Args...>> : public std::true_type {};

template <typename... Args>
struct is_container<std::priority_queue<Args...>> : public std::true_type {};

template <typename... Args>
struct is_container<std::stack<Args...>> : public std::true_type {};

template <typename... Args>
struct is_container<std::set<Args...>> : public std::true_type {};

template <typename... Args>
struct is_container<std::basic_string<Args...>> : public std::true_type {};

/**
 * @brief      wrapper for more simple usage of 'is_container'
 *
 * @tparam     T     { description }
 * @tparam     U     { description }
 */
template <typename T, typename U = T>
struct is_container_type : public is_container<U> {};

// specialization for reference types
template <typename T> struct is_container_type<T &> : public is_container<T> {};
// specialization for pointer types
template <typename T> struct is_container_type<T *> : public is_container<T> {};

// specialization for const types
template <typename T>
struct is_container_type<const T &> : public is_container<T> {};
// specialization for pointer types
template <typename T>
struct is_container_type<const T *> : public is_container<T> {};

// specialization for rvalue refs
template <typename T>
struct is_container_type<T &&> : public is_container<T> {};

// FIXME: tuple_for_each needed here!

} // namespace DataIO
} // namespace Utopia

#endif
