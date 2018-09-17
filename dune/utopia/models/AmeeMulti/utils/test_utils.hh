#ifndef UTOPIA_MODELS_AMEEMULTI_TEST_UTILS_HH
#define UTOPIA_MODELS_AMEEMULTI_TEST_UTILS_HH

// custom assert macros go here
#include "utils.hh"
#include <cmath>
#include <iomanip>
namespace Utopia
{
namespace Models
{
namespace Amee
{
namespace Utils
{
#define ASSERT_EQ(lhs, rhs)                                                  \
    if (!is_equal(lhs, rhs))                                                 \
    {                                                                        \
        std::cerr << std::setprecision(16)                                   \
                  << "Asserted equality wrong at line: " << __LINE__;        \
        std::cerr << std::setprecision(16) << " lhs: " << lhs;               \
        std::cerr << std::setprecision(16) << ", rhs: " << rhs << std::endl; \
        std::exit(-1);                                                       \
    }

#define EXPECT_EQ(lhs, rhs)                                                  \
    if (!is_equal(lhs, rhs))                                                 \
    {                                                                        \
        std::cerr << std::setprecision(16)                                   \
                  << "Exepcted equality wrong at line: " << __LINE__;        \
        std::cerr << std::setprecision(16) << " lhs: " << lhs;               \
        std::cerr << std::setprecision(16) << ", rhs: " << rhs << std::endl; \
    }

#define ASSERT_NEQ(lhs, rhs)                                                 \
    if (is_equal(lhs, rhs))                                                  \
    {                                                                        \
        std::cerr << std::setprecision(16)                                   \
                  << "Asserted inequality wrong at line: " << __LINE__;      \
        std::cerr << std::setprecision(16) << " lhs: " << lhs;               \
        std::cerr << std::setprecision(16) << ", rhs: " << rhs << std::endl; \
        std::exit(-1);                                                       \
    }

#define EXPECT_NEQ(lhs, rhs)                                                 \
    if (is_equal(lhs, rhs))                                                  \
    {                                                                        \
        std::cerr << std::setprecision(16)                                   \
                  << "Exepcted inequality wrong at line: " << __LINE__;      \
        std::cerr << std::setprecision(16) << " lhs: " << lhs;               \
        std::cerr << std::setprecision(16) << ", rhs: " << rhs << std::endl; \
    }

#define ASSERT_GREATER(lhs, rhs)                                                  \
    if (!(is_greater(lhs, rhs)))                                                  \
    {                                                                             \
        std::cerr << std::setprecision(16)                                        \
                  << "Asserted relation 'lhs > rhs' wrong at line: " << __LINE__; \
        std::cerr << std::setprecision(16) << " lhs: " << lhs;                    \
        std::cerr << std::setprecision(16) << ", rhs: " << rhs << std::endl;      \
        std::exit(-1);                                                            \
    }

#define EXPECT_GREATER(lhs, rhs)                                                  \
    if (!(is_greater(lhs, rhs)))                                                  \
    {                                                                             \
        std::cerr << "Exepcted relation 'lhs > rhs' wrong at line: " << __LINE__; \
        std::cerr << std::setprecision(16) << " lhs: " << lhs;                    \
        std::cerr << std::setprecision(16) << ", rhs: " << rhs << std::endl;      \
    }

#define ASSERT_GEQ(lhs, rhs)                                                       \
    if (!(is_greater(lhs, rhs) or is_equal(lhs, rhs)))                             \
    {                                                                              \
        std::cerr << "Asserted relation 'lhs <= rhs' wrong at line: " << __LINE__; \
        std::cerr << std::setprecision(16) << " lhs: " << lhs;                     \
        std::cerr << std::setprecision(16) << ", rhs: " << rhs << std::endl;       \
        std::exit(-1);                                                             \
    }

#define EXPECT_GEQ(lhs, rhs)                                                       \
    if (!(is_greater(lhs, rhs) or is_equal(lhs, rhs)))                             \
    {                                                                              \
        std::cerr << "Exepcted relation 'lhs <= rhs' wrong at line: " << __LINE__; \
        std::cerr << std::setprecision(16) << " lhs: " << lhs;                     \
        std::cerr << std::setprecision(16) << ", rhs: " << rhs << std::endl;       \
    }

#define ASSERT_LESS(lhs, rhs)                                                     \
    if (!(is_less(lhs, rhs)))                                                     \
    {                                                                             \
        std::cerr << std::setprecision(16)                                        \
                  << "Asserted relation 'lhs < rhs' wrong at line: " << __LINE__; \
        std::cerr << std::setprecision(16) << " lhs: " << lhs;                    \
        std::cerr << std::setprecision(16) << ", rhs: " << rhs << std::endl;      \
        std::exit(-1);                                                            \
    }

#define EXPECT_LESS(lhs, rhs)                                                     \
    if (!(is_less(lhs, rhs)))                                                     \
    {                                                                             \
        std::cerr << std::setprecision(16)                                        \
                  << "Exepcted relation 'lhs < rhs' wrong at line: " << __LINE__; \
        std::cerr << std::setprecision(16) << " lhs: " << lhs;                    \
        std::cerr << std::setprecision(16) << ", rhs: " << rhs << std::endl;      \
    }

#define ASSERT_LEQ(lhs, rhs)                                                       \
    if (!(is_less(lhs, rhs) or is_equal(lhs, rhs)))                                \
    {                                                                              \
        std::cerr << std::setprecision(16)                                         \
                  << "Asserted relation 'lhs <= rhs' wrong at line: " << __LINE__; \
        std::cerr << std::setprecision(16) << " lhs: " << lhs;                     \
        std::cerr << std::setprecision(16) << ", rhs: " << rhs << std::endl;       \
        std::exit(-1);                                                             \
    }

#define EXPECT_LEQ(lhs, rhs)                                                       \
    if (!(is_less(lhs, rhs) or is_equal(lhs, rhs)))                                \
    {                                                                              \
        std::cerr << std::setprecision(16)                                         \
                  << "Exepcted relation 'lhs <= rhs' wrong at line: " << __LINE__; \
        std::cerr << std::setprecision(16) << " lhs: " << lhs;                     \
        std::cerr << std::setprecision(16) << ", rhs: " << rhs << std::endl;       \
    }

} // namespace Utils
} // namespace Amee
} // namespace Models
} // namespace Utopia
#endif