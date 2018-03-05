#ifndef HDFUTILITIES_HH
#define HDFUTILITIES_HH
#include <array>
#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <string>
#include <vector>

// function for determining if a type is a stl-container are provided here.
// this is used if we wish to make hdf5 types for storing such data in an
// hdf5 dataset.
namespace Utopia {
namespace DataIO {

// prototype for determining if something is a container or not

template <typename T> struct is_container : public std::false_type {};

// providing specializations for stl containers
// currently non provided for associative containers
// FIXME: why does this even work with variadics??
template <typename T, std::size_t N>
struct is_container<std::array<T, N>> : public std::true_type {};

template <typename... Args>
struct is_container<std::vector<Args...>> : public std::true_type {};

template <typename... Args>
struct is_container<std::list<Args...>> : public std::true_type {};

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

// FIXME: tuple_for_each needed here!
} // namespace DataIO
} // namespace Utopia
#endif