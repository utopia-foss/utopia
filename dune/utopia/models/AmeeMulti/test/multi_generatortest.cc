#include "../utils/generators.hh"
#include "../utils/test_utils.hh"
#include "../utils/utils.hh"
#include <cmath>
#include <iostream>
#include <map>
#include <random>
#include <unordered_map>
#include <vector>

using namespace Utopia::Models::AmeeMulti::Utils;
using namespace Utopia::Models::AmeeMulti;

// Chisquaretest for uniformity
class ChiSquareTest
{
private:
    // kahan summation for accuracy
    template <typename InputIterator, typename F>
    static double __sum_kahan__(InputIterator start, InputIterator end, F f)
    {
        if (start == end)
        {
            return 0.;
        }
        // auto size = std::distance(start, end);
        double comp = 0.;
        double sm = 0.;
        for (; start != end; ++start)
        {
            double y = f(*start) - comp;
            double t = sm + y;
            comp = (t - sm) - y;
            sm = t;
        }
        return sm;
    }

    // lookup table for quantiles
    static std::map<std::pair<int, double>, double> _quantile_values;

public:
    // first value in pair is expected_value. second value in pair is test_value
    static double testquantity(std::vector<std::pair<std::size_t, std::size_t>> data)
    {
        return __sum_kahan__(data.begin(), data.end(), [](auto& pair) {
            double expected = pair.first;
            double observed = pair.second;
            return std::pow((observed - expected), 2) / (expected); // T statistics?
        });
    }

    // evaluate test with given confidence interval
    static bool evaluate(std::vector<std::pair<std::size_t, std::size_t>> data,
                         double desired_confidence,
                         int dofs)
    {
        double tq = testquantity(data);
        auto key = std::pair<int, double>{dofs, desired_confidence};
        return tq < _quantile_values[key];
    }
};

// initialize lookup table for quantiles: {#dofs, quantile, value}
// taken from wikipedia
std::map<std::pair<int, double>, double> ChiSquareTest::_quantile_values = {
    {std::pair<int, double>{1, 0.900}, 2.71},
    {std::pair<int, double>{1, 0.950}, 3.84},
    {std::pair<int, double>{1, 0.975}, 5.02},
    {std::pair<int, double>{1, 0.990}, 6.63},
    {std::pair<int, double>{1, 0.995}, 7.88},
    {std::pair<int, double>{1, 0.999}, 10.83},
    {std::pair<int, double>{2, 0.900}, 4.61},
    {std::pair<int, double>{2, 0.950}, 5.99},
    {std::pair<int, double>{2, 0.975}, 7.38},
    {std::pair<int, double>{2, 0.990}, 9.21},
    {std::pair<int, double>{2, 0.995}, 10.60},
    {std::pair<int, double>{2, 0.999}, 13.82},
    {std::pair<int, double>{3, 0.900}, 6.25},
    {std::pair<int, double>{3, 0.950}, 7.81},
    {std::pair<int, double>{3, 0.975}, 9.35},
    {std::pair<int, double>{3, 0.990}, 11.34},
    {std::pair<int, double>{3, 0.995}, 12.84},
    {std::pair<int, double>{3, 0.999}, 16.27},

    {std::pair<int, double>{4, 0.900}, 7.78},
    {std::pair<int, double>{4, 0.950}, 9.49},
    {std::pair<int, double>{4, 0.975}, 11.14},
    {std::pair<int, double>{4, 0.990}, 13.28},
    {std::pair<int, double>{4, 0.995}, 14.86},
    {std::pair<int, double>{4, 0.999}, 18.47},

    {std::pair<int, double>{5, 0.900}, 9.24},
    {std::pair<int, double>{5, 0.950}, 11.07},
    {std::pair<int, double>{5, 0.975}, 12.83},
    {std::pair<int, double>{5, 0.990}, 15.09},
    {std::pair<int, double>{5, 0.995}, 16.75},
    {std::pair<int, double>{5, 0.999}, 20.52},

    {std::pair<int, double>{6, 0.900}, 10.64},
    {std::pair<int, double>{6, 0.950}, 12.59},
    {std::pair<int, double>{6, 0.975}, 14.45},
    {std::pair<int, double>{6, 0.990}, 16.81},
    {std::pair<int, double>{6, 0.995}, 18.55},
    {std::pair<int, double>{6, 0.999}, 22.46},

    {std::pair<int, double>{7, 0.900}, 12.02},
    {std::pair<int, double>{7, 0.950}, 14.07},
    {std::pair<int, double>{7, 0.975}, 16.01},
    {std::pair<int, double>{7, 0.990}, 18.48},
    {std::pair<int, double>{7, 0.995}, 20.28},
    {std::pair<int, double>{7, 0.999}, 24.32},

    {std::pair<int, double>{8, 0.900}, 13.36},
    {std::pair<int, double>{8, 0.950}, 15.51},
    {std::pair<int, double>{8, 0.975}, 17.53},
    {std::pair<int, double>{8, 0.990}, 20.09},
    {std::pair<int, double>{8, 0.995}, 21.95},
    {std::pair<int, double>{8, 0.999}, 26.12},

    {std::pair<int, double>{9, 0.900}, 14.68},
    {std::pair<int, double>{9, 0.950}, 16.92},
    {std::pair<int, double>{9, 0.975}, 19.02},
    {std::pair<int, double>{9, 0.990}, 21.67},
    {std::pair<int, double>{9, 0.995}, 23.59},
    {std::pair<int, double>{9, 0.999}, 27.88},

    {std::pair<int, double>{10, 0.900}, 15.99},
    {std::pair<int, double>{10, 0.950}, 18.31},
    {std::pair<int, double>{10, 0.975}, 20.48},
    {std::pair<int, double>{10, 0.990}, 23.21},
    {std::pair<int, double>{10, 0.995}, 25.19},
    {std::pair<int, double>{10, 0.999}, 29.59},
};

// used for comparsion when employing the map as a histogram for
// floating point values, here it is made such that the buckets
// have the width of the corresponding integer values of rhs and lhs
template <typename T>
struct d_less
{
    constexpr bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs < T(std::floor(rhs));
    }
};

int main()
{
    for (std::size_t i = 0; i < 10; ++i)
    {
        std::size_t samplesize = 900000;
        int min = -4;
        int max = 4;
        // rngs to test
        std::random_device rd;
        XorShift<> xorshift(rd());
        XorShiftPlus<> xorshiftplus(rd());
        XorShiftStar<> xorshiftstar(rd());
        Xoroshiro<> xoroshiro(rd());
        std::mt19937 mersenne(rd());

        ChiSquareTest test;
        // distributions to test
        std::uniform_int_distribution<int> idist(min, max);
        std::uniform_real_distribution<double> rdist(min, max);

        // structures to hold data
        std::map<int, std::size_t> int_buckets;
        std::map<double, std::size_t, d_less<double>> real_buckets;
        std::vector<std::pair<std::size_t, std::size_t>> data;
        ///////////////////////////// XORSHIFT ////////////////////////////////////

        for (std::size_t i = 0; i < samplesize; ++i)
        {
            ++int_buckets[idist(xorshift)];
            ++real_buckets[rdist(xorshift)];
        }

        data.resize(int_buckets.size());
        for (auto [it, vit] = std::make_pair(int_buckets.begin(), data.begin());
             it != int_buckets.end(); ++it, ++vit)
        {
            *vit = std::make_pair(samplesize / int_buckets.size(), it->second);
        }

        // actual tests

        EXPECT_EQ(test.evaluate(data, 0.950, int_buckets.size() - 1), true);

        data.clear();
        data.resize(real_buckets.size());

        for (auto [it, vit] = std::make_pair(real_buckets.begin(), data.begin());
             it != real_buckets.end(); ++it, ++vit)
        {
            *vit = std::make_pair(samplesize / real_buckets.size(), it->second);
        }

        EXPECT_EQ(test.evaluate(data, 0.950, real_buckets.size() - 1), true);

        // clear buckets for reuse

        int_buckets.clear();
        data.clear();
        real_buckets.clear();
        ///////////////////////////// XORSHIFTPLUS ////////////////////////////////////

        for (std::size_t i = 0; i < samplesize; ++i)
        {
            ++int_buckets[idist(xorshiftplus)];
            ++real_buckets[rdist(xorshiftplus)];
        }

        data.resize(int_buckets.size());
        for (auto [it, vit] = std::make_pair(int_buckets.begin(), data.begin());
             it != int_buckets.end(); ++it, ++vit)
        {
            *vit = std::make_pair(samplesize / int_buckets.size(), it->second);
        }

        // actual tests

        EXPECT_EQ(test.evaluate(data, 0.950, int_buckets.size() - 1), true);

        data.clear();
        data.resize(real_buckets.size());

        for (auto [it, vit] = std::make_pair(real_buckets.begin(), data.begin());
             it != real_buckets.end(); ++it, ++vit)
        {
            // std::cout << it->first << "," << it->second << ","
            //           << samplesize / real_buckets.size() << std::endl;
            *vit = std::make_pair(samplesize / real_buckets.size(), it->second);
        }

        EXPECT_EQ(test.evaluate(data, 0.950, real_buckets.size() - 1), true);

        // clear buckets for reuse

        int_buckets.clear();
        real_buckets.clear();
        data.clear();

        ///////////////////////////// XORSHIFTSTAR ////////////////////////////////////

        for (std::size_t i = 0; i < samplesize; ++i)
        {
            ++int_buckets[idist(xorshiftstar)];
            ++real_buckets[rdist(xorshiftstar)];
        }

        data.resize(int_buckets.size());
        for (auto [it, vit] = std::make_pair(int_buckets.begin(), data.begin());
             it != int_buckets.end(); ++it, ++vit)
        {
            *vit = std::make_pair(samplesize / int_buckets.size(), it->second);
        }

        // actual tests

        EXPECT_EQ(test.evaluate(data, 0.950, int_buckets.size() - 1), true);

        data.clear();
        data.resize(real_buckets.size());

        for (auto [it, vit] = std::make_pair(real_buckets.begin(), data.begin());
             it != real_buckets.end(); ++it, ++vit)
        {
            // std::cout << it->first << "," << it->second << ","
            //           << samplesize / real_buckets.size() << std::endl;
            *vit = std::make_pair(samplesize / real_buckets.size(), it->second);
        }

        EXPECT_EQ(test.evaluate(data, 0.950, real_buckets.size() - 1), true);

        // clear buckets for reuse
        int_buckets.clear();
        real_buckets.clear();
        data.clear();

        ///////////////////////////// XOROSHIRO ////////////////////////////////////

        for (std::size_t i = 0; i < samplesize; ++i)
        {
            ++int_buckets[idist(xoroshiro)];
            ++real_buckets[rdist(xoroshiro)];
        }

        data.resize(int_buckets.size());
        for (auto [it, vit] = std::make_pair(int_buckets.begin(), data.begin());
             it != int_buckets.end(); ++it, ++vit)
        {
            *vit = std::make_pair(samplesize / int_buckets.size(), it->second);
        }

        // actual tests

        EXPECT_EQ(test.evaluate(data, 0.950, int_buckets.size() - 1), true);

        data.clear();
        data.resize(real_buckets.size());

        for (auto [it, vit] = std::make_pair(real_buckets.begin(), data.begin());
             it != real_buckets.end(); ++it, ++vit)
        {
            *vit = std::make_pair(samplesize / real_buckets.size(), it->second);
        }

        EXPECT_EQ(test.evaluate(data, 0.950, real_buckets.size() - 1), true);

        // clear buckets for reuse

        int_buckets.clear();
        real_buckets.clear();
        data.clear();

        ///////////////////////////// Mersenne //////////////////////////////////

        for (std::size_t i = 0; i < samplesize; ++i)
        {
            ++int_buckets[idist(mersenne)];
            ++real_buckets[rdist(mersenne)];
        }

        data.resize(int_buckets.size());
        for (auto [it, vit] = std::make_pair(int_buckets.begin(), data.begin());
             it != int_buckets.end(); ++it, ++vit)
        {
            *vit = std::make_pair(samplesize / int_buckets.size(), it->second);
        }

        // actual tests

        EXPECT_EQ(test.evaluate(data, 0.950, int_buckets.size() - 1), true);

        data.clear();
        data.resize(real_buckets.size());

        for (auto [it, vit] = std::make_pair(real_buckets.begin(), data.begin());
             it != real_buckets.end(); ++it, ++vit)
        {
            *vit = std::make_pair(samplesize / real_buckets.size(), it->second);
        }

        EXPECT_EQ(test.evaluate(data, 0.950, real_buckets.size() - 1), true);

        // clear buckets for reuse

        int_buckets.clear();
        real_buckets.clear();
        data.clear();
    }
    return 0;
}