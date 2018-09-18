#ifndef UTOPIA_MODELS_AMEEMULTI_UTILS_HH
#define UTOPIA_MODELS_AMEEMULTI_UTILS_HH

#include "../../../data_io/hdfutilities.hh"
#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <type_traits>

namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
namespace Utils
{
/**
 * @brief General function for comparing things for lhs == rhs relation.
 *        Containers are compared elementwise, but only if they are equally long
 *        Unequal-length containers compare as fals
 *
 * @tparam T Automatically determined
 * @param lhs object to compare
 * @param rhs object to compare
 * @return true if (elementwise) lhs == rhs
 * @return false else
 */
template <typename T>
constexpr bool is_equal(const T& lhs, const T& rhs, [[maybe_unused]] double tol = 2e-15)
{
    if constexpr (std::is_floating_point_v<T>)
    {
        if (std::abs(lhs) < tol && std::abs(rhs) < tol)
        {
            return true;
        }
        else
        {
            return std::abs(lhs - rhs) / std::max(std::abs(lhs), std::abs(rhs)) < tol;
        }
    }
    else if constexpr (std::is_integral_v<T>)
    {
        return lhs == rhs;
    }
    else if constexpr (DataIO::is_container_v<T>)
    {
        bool equal = lhs.size() == rhs.size();
        if (equal)
        {
            for (std::size_t i = 0; i < lhs.size(); ++i)
            {
                equal = is_equal(lhs[i], rhs[i], tol);
                if (!equal)
                {
                    break;
                }
            }
        }
        return equal;
    }
    else
    {
        throw std::runtime_error("Unknown objects to compare in is_equal");
        return false;
    }
}

/**
 * @brief General function for comparing things for lhs > rhs relation.
 *        Containers are compared elementwise, but only if they are equally long
 *        Unequal-length containers compare as fals
 *
 * @tparam T Automatically determined
 * @param lhs object to compare
 * @param rhs object to compare
 * @return true if (elementwise) lhs > rhs
 * @return false else
 */
template <typename T>
constexpr bool is_greater(const T& lhs, const T& rhs)
{
    bool equal = false;
    if constexpr (DataIO::is_container_v<T>)
    {
        equal = lhs.size() == rhs.size();
        if (equal)
        {
            for (std::size_t i = 0; i < lhs.size(); ++i)
            {
                equal = (lhs[i] > rhs[i]);
                if (!equal)
                {
                    break;
                }
            }
        }
    }
    else
    {
        equal = (lhs > rhs);
    }
    return equal;
}

/**
 * @brief General function for comparing things for lhs < rhs relation.
 *        Containers are compared elementwise, but only if they are equally long
 *        Unequal-length containers compare as fals
 *
 * @tparam T Automatically determined
 * @param lhs object to compare
 * @param rhs object to compare
 * @return true if (elementwise) lhs < rhs
 * @return false else
 */
template <typename T>
constexpr bool is_less(const T& lhs, const T& rhs)
{
    bool equal = false;
    if constexpr (DataIO::is_container_v<T>)
    {
        equal = lhs.size() == rhs.size();
        if (equal)
        {
            for (std::size_t i = 0; i < lhs.size(); ++i)
            {
                equal = (lhs[i] < rhs[i]);
                if (!equal)
                {
                    break;
                }
            }
        }
    }
    else
    {
        equal = (lhs < rhs);
    }
    return equal;
}

template <typename T, std::enable_if_t<DataIO::is_container_v<T>, int> = 0>
std::ostream& operator<<(std::ostream& out, const T& container)
{
    std::setprecision(16);
    out << "[";
    for (std::size_t i = 0; i < (container.size() - 1); ++i)
    {
        out << container[i] << ",";
    }
    out << container.back() << "]";

    return out;
}

} // namespace Utils
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia

#endif