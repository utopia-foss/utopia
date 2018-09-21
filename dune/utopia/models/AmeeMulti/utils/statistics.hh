#ifndef STATISTICS_HH
#define STATISTICS_HH
/** @file statistics.hh
 *  @brief Implements functions for computing statistics.
 *
 *  @author Harald Mack, harald.mack@iup.uni-heidelberg.de
 *  @bug Quantiles return only exact quantiles, that is only
 *       values which are contained in the container.
 *  @todo Optimize, perhaps use template metaprogramming where appropriate
 *  @todo Use pratition sums to make numerical errors smaller, test this!
 *  @todo Implement interpolation for quantiles, since they are defined in the
 * respective way!
 *
 */

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <random>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
namespace Utils
{
/**
 * @brief Functor for computing factorial during compile time
 *
 * @tparam n
 */
template <std::size_t n>
struct factorial
{
    constexpr static std::size_t value = n * factorial<n - 1>::value;
};

template <>
struct factorial<0>
{
    constexpr static std::size_t value = 1;
};

/**
 * @brief Functor for computing binomial coefficient during compile time
 *
 * @tparam n
 * @tparam k
 */
template <std::size_t n, std::size_t k>
struct binomial
{
    constexpr static std::size_t value =
        factorial<n>::value / (factorial<k>::value * factorial<n - k>::value);
};

struct Sum
{
    /**
     * @brief      Pairwise summation of the range [start, end).
     *             Ignores NaN and Inf values
     *
     * @param[in]  start          The start iterator
     * @param[in]  end            The end iterator
     * @param[in]  f              function to apply to the values to accumulate.
     *
     *
     * @return     sum over (f(it)) for 'it' in [start, end)
     */

    template <typename InputIterator, typename F, typename... Args>
    inline double operator()(InputIterator start, InputIterator end, F f, Args... args)
    {
        if (start == end)
        {
            return 0.;
        }
        auto size = std::distance(start, end);
        if (size < 1000)
        {
            double s = 0.;
            for (; start != end; ++start)
            {
                double val = f(start, args...);
                if (std::isinf(val) || std::isnan(val))
                {
                    continue;
                }
                s += f(start, args...);
            }
            return s;
        }
        else
        {
            auto m = std::floor(size / 2);
            double s = sum(start, start + m, f, args...) + sum(start + m, end, f, args...);
            return s;
        }
    }
};

struct SumKahan
{
    /**
     * @brief      Summation of the range [start, end) using Kahan's algorithm.
     *
     * @param[in]  start          The start iterator
     * @param[in]  end            The end iterator
     * @param[in]  f              function to apply to the values to accumulate.
     * @param[in]  args           additional arguments to f
     *
     *
     * @return     sum(f(it)) for it in [star, end)
     */

    template <typename InputIterator, typename F, typename... Args>
    inline double operator()(InputIterator start, InputIterator end, F f, Args... args)
    {
        if (start == end)
        {
            return 0.;
        }
        auto size = std::distance(start, end);
        double comp = 0.;
        double sm = 0.;
        for (; start != end; ++start)
        {
            double y = f(start, args...) - comp;
            double t = sm + y;
            comp = (t - sm) - y;
            sm = t;
        }
        return sm;
    }
};

struct Moment
{
    /**
     * @brief      Computes the 'order'-th moment of the range [start, end)
     *
     * @param[in]  order          The order
     * @param[in]  start          The start iterator
     * @param[in]  end            The end iterator
     *
     *
     *
     * @return    order-th moment wrt zero.
     */
    template <typename InputIterator, typename Getter>
    inline double operator()(int order, InputIterator start, InputIterator end, Getter getter)
    {
        if (start == end)
        {
            return 0;
        }
        double mmnt = 0;
        for (auto it = start; it != end; ++it)
        {
            mmnt += std::pow(getter(*it), order);
        }
        return mmnt / static_cast<double>(std::distance(start, end));
    }

    /**
     * @brief      Computes the 'order'-th moment of the range [start, end)
     *
     * @param[in]  order          The order
     * @param[in]  start          The start iterator
     * @param[in]  end            The end iterator
     * @param[in]  getter         The getter, a function taking an iterator and
     * returning something else, for instance an element contained in the object
     * the iterator points to.
     *
     *
     *
     * @return    order-th moment wrt zero.
     */
    template <typename InputIterator>
    inline double operator()(int order, InputIterator start, InputIterator end)
    {
        return this->operator()(order, start, end,
                                [](auto& value) { return value; });
    }
};

struct ArithmeticMean
{
    /**
    * @brief      Function computing the mean of the range [start, end).
    *
    * @param[in]  start      The iterator pointing to the first element in the
    *                         range considered.
    * @param[in]  end        The iterator pointing to the element after the last
    *                        element in the range.
    * @param[in]  getter     The Function for converting an iterator into an
    element.
    *

    *
    * @return     The mean of the range [start, end)
    */
    template <typename InputIterator, typename Elementgetter>
    inline double operator()(InputIterator start, InputIterator end, Elementgetter getter)
    {
        return Moment()(1, start, end, getter);
    }

    /**
     * @brief      Function computing the mean of the range [start, end).
     *
     * @param[in]  start      The iterator pointing to the first element in the
     *                         range considered.
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range.
     *
     * @return     The mean of the range [start, end)
     */
    template <typename InputIterator>
    inline double operator()(InputIterator start, InputIterator end)
    {
        return Moment()(1, start, end);
    }
};

struct HarmonicMean
{
    /**
     * @brief      Function computes the harmonic mean of the range [start, end).
     *
     * @param[in]  start          The start iterator
     * @param[in]  end            The end iterator
     * @param[in]  getter         The getter function extracting an element from
     *                            the object the iterator points to
     *
     *
     * @return     The harmonic mean of the range start,end
     */

    template <typename InputIterator, typename Elementgetter>
    inline double operator()(InputIterator start, InputIterator end, Elementgetter getter)
    {
        if (start == end)
        {
            return 0;
        }

        double hm =
            sum(start, end, [&](InputIterator it) { return 1. / getter(*it); });
        return static_cast<double>(std::distance(start, end)) / hm;
    }

    /**
     * @brief      Computes the harmonic mean of the values in the range [start,
     * end)
     *
     * @param[in]  start          The start iterator of the range
     * @param[in]  end            The end iterator of the range
     *
     * @tparam     InputIterator  Automatically determined template parameter
     *
     * @return     harmonic mean of [start, end)
     */
    template <typename InputIterator>
    inline double operator()(InputIterator start, InputIterator end)
    {
        return this->operator()(start, end, [](auto& value) { return value; });
    }
};

struct CentralMoment
{
    /**
     * @brief      Function computing the 'order'-th central moment
     *              (moment with respect to the mean) of the range
     *              [start, end).
     *
     * @param[in]  order      The order.
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     *
     * @return     The order-th central moment of the range [start, end)
     */
    template <typename InputIterator, typename Elementgetter>
    double operator()(int order, InputIterator start, InputIterator end, Elementgetter getter)
    {
        // double c = 0;
        double mean = arithmetic_mean(start, end, getter);
        // for(auto it = start; it != end; ++it){
        //     c+= std::pow(getter(*it)-mean, order);
        // }
        double c = sum(
            start, end,
            [&](InputIterator it) { return std::pow(getter(*it) - mean, order); }, order);
        return c / static_cast<double>(std::distance(start, end));
    }

    /**
     * @brief      Function computing the 'order'-th central moment
     *              (moment with respect to the mean) of the range
     *              [start, end).
     *
     * @param[in]  order      The order.
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     *
     * @return     The order-th central moment of the range [start, end)
     */
    // FIXME: these need one-pass algorithm!
    template <typename InputIterator>
    double operator()(int order, InputIterator start, InputIterator end)
    {
        return this->operator()(order, start, end,
                                [](auto& value) { return value; });
    }
};

struct Variance
{
    /**
     * @brief      Computes the sample-variance of the range [start, end).
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     *
     * @return    The sample-variance of the range [start, end)
     */
    // FIXME: use partition algorithm for the summation. Overly complicated??
    template <typename InputIterator, typename Elementgetter>
    double operator()(InputIterator start, InputIterator end, Elementgetter getter)
    {
        double n = 0;
        double mean = 0;
        double M2 = 0;
        double d = 0;
        double element = 0;
        for (auto it = start; it != end; ++it)
        {
            element = getter(*it);
            n += 1;
            d = element - mean;
            mean += d / n;
            M2 += d * (element - mean);
        }
        return M2 / (n - 1.);
    }

    /**
     * @brief      Computes the sample-variance of the range [start, end).
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     *
     * @return    The sample-variance of the range [start, end)
     */
    // FIXME: use partition algorithm for the summation.
    template <typename InputIterator>
    double operator()(InputIterator start, InputIterator end)
    {
        return this->operator()(start, end, [](auto& value) { return value; });
    }
};

struct Stddev
{
    /**
     * @brief      Computes the standard-deviation of the range [start, end)
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     *
     *
     * @return     The standard-deviation of the range [start, end).
     */
    template <typename InputIterator, typename Elementgetter>
    double operator()(InputIterator start, InputIterator end, Elementgetter getter)
    {
        return std::pow(Variance()(start, end, getter), 0.5);
    }

    /**
     * @brief      Computes the standard-deviation of the range [start, end)
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     *
     *
     * @return     The standard-deviation of the range [start, end).
     */
    template <typename InputIterator>
    double operator()(InputIterator start, InputIterator end)
    {
        return std::pow(Variance()(start, end), 0.5);
    }
};

struct Skewness
{
    /**
     * @brief      Computes the sample-skewness of the range [start, end).
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     *
     *
     * @return     The sample-skewness of [start, end)
     */
    template <typename InputIterator, typename Elementgetter>
    double operator()(InputIterator start, InputIterator end, Elementgetter getter)
    {
        double n = 0;
        double mean = 0;
        double M2 = 0;
        double M3 = 0;
        double n1 = 0;
        double delta = 0;
        double delta_n = 0;
        // double delta_n2 = 0;
        double term1 = 0;
        for (auto it = start; it != end; ++it)
        {
            n1 = n;
            n = n + 1;
            delta = getter(*it) - mean;
            delta_n = delta / n;
            // delta_n2 = delta_n * delta_n;
            term1 = delta * delta_n * n1;
            mean = mean + delta_n;
            // M4 = M4 + term1 * delta_n2 * (n*n - 3*n + 3) + 6 * delta_n2 * M2 - 4
            // * delta_n * M3;
            M3 = M3 + term1 * delta_n * (n - 2) - 3 * delta_n * M2;
            M2 = M2 + term1;
        }

        return std::pow(n, 0.5) * M3 / std::pow(M2, 1.5);
    }

    /**
     * @brief      Computes the sample-skewness of the range [start, end).
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     *
     *
     * @return     The sample-skewness of [start, end)
     */
    template <typename InputIterator>
    double operator()(InputIterator start, InputIterator end)
    {
        return this->operator()(start, end, [](auto& value) { return value; });
    }
};

struct Kurtosis
{
    /**
     * @brief      Computes the sample-kurtosis of the range [start, end).
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     *
     *
     * @return     The sample-kurtosis [start, end)
     */

    template <typename InputIterator, typename Elementgetter>
    double operator()(InputIterator start, InputIterator end, Elementgetter getter)
    {
        double n = 0;
        double mean = 0;
        double M2 = 0;
        double M3 = 0;
        double M4 = 0;
        double n1 = 0;
        double delta = 0;
        double delta_n = 0;
        double delta_n2 = 0;
        double term1 = 0;
        for (auto it = start; it != end; ++it)
        {
            n1 = n;
            n = n + 1;
            delta = getter(*it) - mean;
            delta_n = delta / n;
            delta_n2 = delta_n * delta_n;
            term1 = delta * delta_n * n1;
            mean = mean + delta_n;
            M4 = M4 + term1 * delta_n2 * (n * n - 3 * n + 3) +
                 6 * delta_n2 * M2 - 4 * delta_n * M3;
            M3 = M3 + term1 * delta_n * (n - 2) - 3 * delta_n * M2;
            M2 = M2 + term1;
        }
        return n * M4 / (std::pow(M2, 2));
    }

    /**
     * @brief      Computes the sample-kurtosis of the range [start, end).
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     *
     *
     * @return     The sample-kurtosis [start, end)
     */
    template <typename InputIterator>
    double operator()(InputIterator start, InputIterator end)
    {
        return operator()(start, end, [](auto& value) { return value; });
    }
};

struct ExcessKurtosis
{
    /**
     * @brief      Computes the excess-kurtosis of [start, end)
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     *
     *
     * @return     Excess-kurtosis of [start, end), i.e., sample-kurtosis-3.0.
     */
    template <typename InputIterator, typename Elementgetter>
    double operator()(InputIterator start, InputIterator end, Elementgetter getter)
    {
        return Kurtosis()(start, end, getter) - 3.;
    }

    /**
     * @brief      Computes the excess-kurtosis of [start, end)
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     *
     *
     * @return     Excess-kurtosis of [start, end), i.e., sample-kurtosis-3.0.
     */
    template <typename InputIterator>
    double operator()(InputIterator start, InputIterator end)
    {
        return Kurtosis()(start, end) - 3.;
    }
};

struct Standardize
{
    /**
     * @brief Standardizes the values in the range [begin,end)
     *
     * @tparam InputIterator
     * @tparam Getter
     * @param begin
     * @param end
     * @param getter
     * @return auto
     */
    template <typename InputIterator, typename Getter>
    auto operator()(InputIterator begin, InputIterator end, Getter&& getter)
    {
        long double avg = mean(begin, end, getter);
        long double std = stddev(begin, end, getter);
        std::vector<double> zscore(std::distance(begin, end));
        auto vbegin = zscore.begin();
        for (; begin != end; ++begin, ++vbegin)
        {
            *vbegin = (static_cast<long double>(getter(*begin)) - avg) / std;
        }

        return zscore;
    }
};

struct Covariance
{
    /**
     * @brief      Computes the covariance between the two ranges
     *             [start_first, end_first) and [start_second, end_second).
     *             If one range is shorter than another the covariance is
     *             computed only for the overlab of the two ranges.
     * @param[in]  start_first   The iterator pointing to the start of the first
     *                           range.
     * @param[in]  end_first     The iterator pointing to the element after the
     *                            end  of the first range
     * @param[in]  start_second  The iterator pointing to the start of the second
     *                           range.
     * @param[in]  end_second    The iterator pointing to the element after the
     *                           end  of the secodn range
     * @param[in]  getter        Function for converting an iterator into a value.
     *
     *
     * @return     The covariance of the two ranges:
     *              cov([start_first, end_first), [start_second, end_second))
     */
    template <typename InputIterator1, typename InputIterator2, typename Getter1, typename Getter2>
    double operator()(InputIterator1 start_first,
                      InputIterator1 end_first,
                      InputIterator2 start_second,
                      InputIterator2 end_second,
                      Getter1 getter1,
                      Getter2 getter2)
    {
        // runs over min(std::distance(start_first, end_first), start_second,
        // end_second))
        double n = 0;
        double mean1 = 0;
        double mean2 = 0;

        double M12 = 0;
        double d1 = 0;
        double d2 = 0;
        auto it1 = start_first;
        auto it2 = start_second;

        while (it1 != end_first && it2 != end_second)
        {
            n += 1;
            d1 = (getter1(*it1) - mean1) / n;
            mean1 += d1;
            d2 = (getter2(*it2) - mean2) / n;
            mean2 += d2;
            M12 += (n - 1) * d1 * d2 - M12 / n;
            ++it1;
            ++it2;
        }
        return M12 * (n / (n - 1));
    }

    /**
     * @brief      Computes the covariance between the two ranges
     *             [start_first, end_first) and [start_second, end_second).
     *             If one range is shorter than another the covariance is
     *             computed only for the overlab of the two ranges.
     * @param[in]  start_first   The iterator pointing to the start of the first
     *                           range.
     * @param[in]  end_first     The iterator pointing to the element after the
     *                            end  of the first range
     * @param[in]  start_second  The iterator pointing to the start of the second
     *                           range.
     * @param[in]  end_second    The iterator pointing to the element after the
     *                           end  of the secodn range
     * @param[in]  getter        Function for converting an iterator into a value.
     *
     *
     * @return     The covariance of the two ranges:
     *              cov([start_first, end_first), [start_second, end_second))
     */
    template <typename InputIterator1, typename InputIterator2>
    double operator()(InputIterator1 start_first,
                      InputIterator1 end_first,
                      InputIterator2 start_second,
                      InputIterator2 end_second)
    {
        // runs over min(std::distance(start_first, end_first), start_second,
        // end_second))
        return operator()(start_first, end_first, start_second, end_second,
                          [](auto& value) { return value; },
                          [](auto& value) { return value; });
    }
};

struct Quantile
{
    /**
     * @brief      Computes the 'percentage'-th quantile of the distribution.
     *
     * @param[in]  percent    The percentage to compute the quantile for.
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  comp       The comparison-function to use, defaults to
     *                        operator<(const T&, const T&).
     *
     *
     * @return    The 'percentage'-th quantile of the range [start, end).
     */
    template <typename InputIterator, typename Getter, typename Comp>
    double operator()(double percent, InputIterator start, InputIterator end, Comp&& comp, Getter getter)
    {
        using T = typename std::iterator_traits<InputIterator>::value_type;
        std::vector<T> temp;
        double size = std::distance(start, end);

        temp.reserve(size);
        for (auto it = start; it != end; ++it)
        {
            temp.emplace_back(getter(*it));
        }
        double idx = size * (percent / 100.);

        std::nth_element(temp.begin(), temp.begin() + idx, temp.end(), comp);
        return temp[std::floor(idx)];
    }

    /**
     * @brief      Computes the 'percentage'-th quantile of the distribution.
     *
     * @param[in]  percent    The percentage to compute the quantile for.
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     *
     * @return    The 'percentage'-th quantile of the range [start, end).
     */
    template <typename InputIterator, typename Getter>
    double operator()(double percent, InputIterator start, InputIterator end, Getter getter)
    {
        return operator()(percent, start, end,
                          [](const auto& value1, const auto& value2) {
                              return value1 < value2;
                          },
                          getter);
    }

    /**
     * @brief Computes the 'percentage'-th quantile of the distribution.
     *
     * @tparam InputIterator
     * @param percent
     * @param start
     * @param end
     * @return double
     */
    template <typename InputIterator>
    double operator()(double percent, InputIterator start, InputIterator end)
    {
        return operator()(percent, start, end,
                          [](const auto& value1, const auto& value2) {
                              return value1 < value2;
                          },
                          [](auto& value) { return value; });
    }
};

struct Mode
{
    /**
     * @brief      Computes the mode of the range [begin, end), i.e., the most
     *              frequent value in the range.
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     *
     * @return     The mode of the range [begin, end) as pair (element, count)
     */
    template <typename InputIterator, typename Elementgetter>
    std::pair<typename std::iterator_traits<InputIterator>::value_type, int> operator()(
        InputIterator start, InputIterator end, Elementgetter getter)
    {
        using T = typename std::iterator_traits<InputIterator>::value_type;
        auto maxelement = getter(*start);
        int max = 1;
        std::unordered_map<T, int> counter;
        for (auto it = start; it != end; ++it)
        {
            auto element = getter(*it);
            ++counter[*it];
            if (counter[element] > max)
            {
                max = counter[element];
                maxelement = element;
            }
        }
        return std::make_pair(maxelement, counter[maxelement]);
    }

    /**
     * @brief      Computes the mode of the range [begin, end), i.e., the most
     *              frequent value in the range.
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     * @param[in]  comp       The comparison-function to use, defaults to
     *                        operator<(const T&, const T&) for the second of
     * the elements of a pair (T, int).
     *
     *
     * @return     The mode of the range [begin, end)
     */
    template <typename InputIterator, typename T = typename std::iterator_traits<InputIterator>::value_type>
    std::pair<T, int> operator()(InputIterator start, InputIterator end)
    {
        return operator()(start, end, [](auto& value) { return value; });
    }
};

struct Median
{
    /**
    * @brief     Computes the median, the 50th-quantile, of the range
    *            [begin, end).
    *
    * @param[in]  start      The iterator pointing to the first element in
    *                        [start, end)
    * @param[in]  end        The iterator pointing to the element after the last
    element in the range [start, end)
    * @param[in]  getter     Function converting an iterator to a value.
    *
    * @param[in]  comp       The comparison-function to use, defaults to
    *                        operator<(const T&, const T&) for the second of the
    *                        elements of a pair (T, int).
    *
    *
    * @return     The mode of the range [begin, end)
    */
    template <typename InputIterator, typename Elementgetter, typename Compare>
    auto operator()(InputIterator start, InputIterator end, Elementgetter getter, Compare comp)
    {
        return Quantile()(50., start, end, comp, getter);
    }

    /**

    * @brief     Computes the median, the 50th-quantile, of the range
    *            [begin, end).
    *
    * @param[in]  start      The iterator pointing to the first element in
    *                        [start, end)
    * @param[in]  end        The iterator pointing to the element after the last
    element in the range [start, end)
    * @param[in]  getter     Function converting an iterator to a value.
    *
    * @param[in]  comp       The comparison-function to use, defaults to
    *                        operator<(const T&, const T&) for the second of the
    *                        elements of a pair (T, int).
    *
    *
    * @return     The mode of the range [begin, end)
    */
    template <typename InputIterator, typename Elementgetter>
    auto operator()(InputIterator start, InputIterator end, Elementgetter getter)
    {
        return Quantile()(
            50., start, end,
            [](auto& value1, auto& value2) { return value1 < value2; }, getter);
    }

    /**
     * @brief     Computes the median, the 50th-quantile, of the range
     *            [begin, end).
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     *                           last element in the range [start, end)
     *
     *
     * @return     The median (= 50th-quantile) of the range [begin, end).
     */
    template <typename InputIterator>
    auto operator()(InputIterator start, InputIterator end)
    {
        return operator()(start, end, [](auto& value) { return value; });
    }
};

struct Minimum
{
    /**
     * @brief      Computes the minimum of the range [begin, end).
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     *
     * @param[in]  The comparison-function to use, defaults to
     *                        operator<(const T&, const T&).
     *
     *
     * @return     The minimum value in the range [begin, end).
     */
    template <typename InputIterator, typename Compare>
    auto operator()(InputIterator start, InputIterator end, Compare comp)
    {
        return *std::min_element(start, end, comp);
    }

    /**
     * @brief      Computes the minimum of the range [begin, end).
     *
     * @tparam InputIterator
     * @param start
     * @param end
     * @return auto
     */
    template <typename InputIterator>
    auto operator()(InputIterator start, InputIterator end)
    {
        return *std::min_element(start, end);
    }
};

struct Maximum
{
    /**
     * @brief      Computes the maximum of the range [begin, end).
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     *
     * @param[in]  The comparison-function to use, defaults to
     *                        operator<(const T&, const T&).
     *
     *
     * @return     The maximum value in the range [begin, end).
     */
    template <typename InputIterator, typename Compare>
    auto operator()(InputIterator start, InputIterator end, Compare&& comp)
    {
        return *std::max_element(start, end, comp);
    }

    /**
     * @brief Computes the maximum of the range [begin, end).
     *
     * @tparam InputIterator
     * @tparam std::function<bool(const T &, const T &)>
     * @param start
     * @param end
     * @param comp
     * @return auto
     */
    template <typename InputIterator>
    auto operator()(InputIterator start, InputIterator end)
    {
        return *std::max_element(start, end);
    }
};

struct Describe
{
    /**
     * @brief      General function returning values characterizing the
     * distribution represented by the values in the range [start, end):
     *             * Mean
     *             * variance
     *             * skewness
     *             * excess-kurtosis
     *             * mode
     *             * min
     *             * 25-th quantile
     *             * 50-th quantile
     *             * 75-th quantile
     *             * max
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     * @param[in]  comp       The comparison operator to use
     *
     *
     * @return     A std::vector<double> holding the quantities given above.
     */
    template <typename InputIterator, typename Elementgetter>
    auto operator()(InputIterator start, InputIterator end, Elementgetter getter)
    {
        if (std::distance(start, end) == 0)
        {
            return std::array<double, 8>({{0, 0, 0, 0, 0, 0, 0, 0}});
        }
        using T = decltype(getter(*start));
        long int n = 0;
        long int n1 = 0;
        double mean = 0;
        double M2 = 0;
        double delta = 0;
        double delta_n = 0;
        double term1 = 0;

        auto min = getter(*start);
        auto max = min;
        std::unordered_map<T, int, std::hash<T>, std::equal_to<T>> buckets;
        std::vector<T> data;
        std::size_t size = std::distance(start, end);
        data.reserve(size);

        std::for_each(start, end,
                      [&](auto& value) { data.emplace_back(getter(value)); });

        long int maxcount = 1.;
        T mode = getter(*start);

        for (auto& element : data)
        {
            n1 = n;
            n += 1;
            delta = element - mean;
            delta_n = delta / n;
            term1 = delta * delta_n * n1;
            mean = mean + delta_n;
            M2 = M2 + term1;
            if (element < min)
            {
                min = element;
            }
            if (element > max)
            {
                max = element;
            }
            ++buckets[element];
            if (maxcount > buckets[element])
            {
                maxcount = buckets[element];
                mode = element;
            }
        }

        // computing quantiles
        std::size_t idx_25 = std::floor(size * 0.25);
        std::size_t idx_50 = std::floor(size * 0.50);
        std::size_t idx_75 = std::floor(size * 0.75);

        std::nth_element(data.begin(), data.begin() + (idx_25), data.end());
        T q25 = *(data.begin() + (idx_25));

        std::nth_element(data.begin(), data.begin() + (idx_50), data.end());
        T q50 = *(data.begin() + (idx_50));

        std::nth_element(data.begin(), data.begin() + (idx_75), data.end());
        T q75 = *(data.begin() + (idx_75));

        return std::array<double, 8>{
            {static_cast<double>(mean), static_cast<double>(M2 / (n - 1)),
             static_cast<double>(min), static_cast<double>(q25), static_cast<double>(q50),
             static_cast<double>(q75), static_cast<double>(max)}};
    }

    /**
     * @brief      General function returning values characterizing the
     * distribution represented by the values in the range [start, end):
     *             * Mean
     *             * variance
     *             * skewness
     *             * excess-kurtosis
     *             * mode
     *             * min
     *             * 25-th quantile
     *             * 50-th quantile
     *             * 75-th quantile
     *             * max *
     * @tparam InputIter
     * @param start
     * @param end
     * @return auto
     */
    template <typename InputIter>
    auto operator()(InputIter start, InputIter end)
    {
        return operator()(start, end, [](auto& value) { return value; });
    }
};

struct FiveNumberSummary
{
    /**
     * @brief      Generates five-number-summary of the data:
     *                - min
     *                - 1st quartile
     *                - 2nd quartile (median)
     *                - 3rd quartile
     *                - max
     *             This function assumes that the respective container-type has
     * a constructor which accepts initializer-lists.
     *
     * @param[in]  start      The iterator pointing to the first element in
     *                        [start, end)
     * @param[in]  end        The iterator pointing to the element after the
     * last element in the range [start, end)
     * @param[in]  getter     Function converting an iterator to a value.
     *
     *
     * @return     A vector<T> containing the quantities given above.
     */
    template <typename InputIterator, typename Elementgetter>
    auto operator()(
        InputIterator start,
        InputIterator end,
        Elementgetter getter,
        std::function<bool(const typename std::iterator_traits<InputIterator>::value_type&,
                           const typename std::iterator_traits<InputIterator>::value_type&)> comp)
    {
        using T = typename std::iterator_traits<InputIterator>::value_type;

        std::size_t size = std::distance(start, end);

        std::vector<T> data;
        data.reserve(size);
        T min = getter(*start);
        T max = min;
        for (auto it = start; it != end; ++it)
        {
            T element = getter(*it);
            data.emplace_back(element);
            if (element < min)
            {
                min = element;
            }
            if (element > max)
            {
                max = element;
            }
        }

        // computing quantiles
        std::size_t idx_25 = std::floor(size * 0.25);
        std::size_t idx_50 = std::floor(size * 0.50);
        std::size_t idx_75 = std::floor(size * 0.75);

        std::nth_element(data.begin(), data.begin() + (idx_25), data.end(), comp);
        T q25 = *(data.begin() + (idx_25));

        std::nth_element(data.begin(), data.begin() + (idx_50), data.end(), comp);
        T q50 = *(data.begin() + (idx_50));

        std::nth_element(data.begin(), data.begin() + (idx_75), data.end(), comp);
        T q75 = *(data.begin() + (idx_75));

        return std::array<T, 5>{{min, q25, q50, q75, max}};
    }

    /**
     * @brief      Computes Tukey's five number summary,
     *             i.e. minimum, quartiles and maximum.
     *
     * @param[in]  start          The start iterator of the range[start, end)
     * @param[in]  end            The end iterator of the range[start, end)
     * @param[in]  comp           The comparison-operator, defaults to '<'
     *
     *
     * @return     A vector<T> containing Tukey's five number summary.
     */
    template <typename InputIterator>
    auto operator()(
        InputIterator begin,
        InputIterator end,
        std::function<bool(const typename std::iterator_traits<InputIterator>::value_type&,
                           const typename std::iterator_traits<InputIterator>::value_type&)> comp)
    {
        return operator()(begin, end, [](auto& value) { return value; }, comp);
    }

    /**
     * @brief returns Tukey's five number summary

     *
     * @tparam InputIterator
     * @tparam Getter
     * @param begin
     * @param end
     * @param getter
     * @return auto
     */
    template <typename InputIterator, typename Getter>
    auto operator()(InputIterator begin, InputIterator end, Getter&& getter)
    {
        return operator()(begin, end, std::forward<Getter&&>(getter),
                          [](const auto& value1, const auto& value2) {
                              return value1 < value2;
                          });
    }

    /**
     * @brief returns Tukey's five number summary
     *
     * @tparam InputIterator
     * @param begin
     * @param end
     * @return auto
     */
    template <typename InputIterator>
    auto operator()(InputIterator begin, InputIterator end)
    {
        return operator()(begin, end, [](auto& value) { return value; },
                          [](const auto& value1, const auto& value2) {
                              return value1 < value2;
                          });
    }
};

struct Sample
{
    /**
     * @brief      Function to generate a random sample of relative
     *              size 'percent' from a range [start, end).
     *
     * @param[in]  percent      The sample size in percent.
     * @param[in]  start      Iterator to the first element to consider
     * @param[in]  end        Iterator pointing to the element after the last
     *                        element to consider.
     *
     *
     * @return     Sample containing 'percent'-percent of the elements in
     * [begin,end) with the elemenst contained being randomly chosen. The output
     * type is the same as the container which provides the input iterators.
     */
    template <typename InputIterator, typename Getter>
    auto operator()(double share, InputIterator start, InputIterator end, Getter getter)
    {
        std::size_t size = std::distance(start, end);
        std::size_t shr = (share / 100.) * size;
        std::vector<typename std::result_of<Getter>::type> smpl(shr);
        std::vector<std::size_t> indices(size);
        std::random_device rd;
        std::minstd_rand0 generator(rd());
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), generator);

        auto smpl_it = smpl.begin();
        for (std::size_t i = 0; i < shr; ++i)
        {
            *smpl_it = getter(std::next(start, indices[i]));
            ++smpl_it;
        }
        return smpl;
    }

    /**
     * @brief      Function to generate a random sample of relative
     *              size 'percent' from a range [start, end).
     *
     * @param[in]  percent      The sample size in percent.
     * @param[in]  start      Iterator to the first element to consider
     * @param[in]  end        Iterator pointing to the element after the last
     *                        element to consider.
     *
     *
     * @return     Sample containing 'percent'-percent of the elements in
     * [begin,end) with the elemenst contained being randomly chosen. The output
     * type is the same as the container which provides the input iterators.
     */
    template <typename InputIterator>
    auto operator()(double percent, InputIterator start, InputIterator end)
    {
        return operator()(percent, start, end, [](auto& value) { return value; });
    }
};

template <typename... Funcs>
struct Statistician
{
    template <typename InputIterator, typename Getter>
    auto operator()(InputIterator begin, InputIterator end, Getter getter)
    {
        return std::array<double, sizeof...(Funcs)>{Funcs()(begin, end, getter)...};
    }

    template <typename InputIterator>
    auto operator()(InputIterator begin, InputIterator end)
    {
        return std::array<double, sizeof...(Funcs)>{Funcs()(begin, end)...};
    }

    template <typename InputIterator1, typename InputIterator2, typename Getter1, typename Getter2>
    auto operator()(InputIterator1 begin1,
                    InputIterator1 end1,
                    InputIterator2 begin2,
                    InputIterator2 end2,
                    Getter1 getter1,
                    Getter2 getter2)
    {
        return std::array<double, sizeof...(Funcs)>{
            Funcs()(begin1, end1, begin2, end2, getter1, getter2)...};
    }

    template <typename InputIterator1, typename InputIterator2>
    auto operator()(InputIterator1 begin1, InputIterator1 end1, InputIterator2 begin2, InputIterator2 end2)
    {
        return std::array<double, sizeof...(Funcs)>{Funcs()(begin1, end1, begin2, end2)...};
    }
}; // namespace Utils

} // namespace Utils
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia

#endif