// custom assert macros go here
#include "utils.hh"
#include <cmath>

namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
namespace Utils
{
#define ASSERT_EQ(lhs, rhs)                                           \
    if (!is_equal(lhs, rhs))                                          \
    {                                                                 \
        std::cerr << "Asserted equality wrong at line: " << __LINE__; \
        std::cerr << " lhs: " << lhs;                                 \
        std::cerr << ", rhs: " << rhs << std::endl;                   \
        std::exit(-1);                                                \
    }

#define EXPECT_EQ(lhs, rhs)                                           \
    if (!is_equal(lhs, rhs))                                          \
    {                                                                 \
        std::cerr << "Exepcted equality wrong at line: " << __LINE__; \
        std::cerr << " lhs: " << lhs;                                 \
        std::cerr << ", rhs: " << rhs << std::endl;                   \
    }

#define ASSERT_NEQ(lhs, rhs)                                            \
    if (is_equal(lhs, rhs))                                             \
    {                                                                   \
        std::cerr << "Asserted inequality wrong at line: " << __LINE__; \
        std::cerr << " lhs: " << lhs;                                   \
        std::cerr << ", rhs: " << rhs << std::endl;                     \
        std::exit(-1);                                                  \
    }

#define EXPECT_NEQ(lhs, rhs)                                            \
    if (is_equal(lhs, rhs))                                             \
    {                                                                   \
        std::cerr << "Exepcted inequality wrong at line: " << __LINE__; \
        std::cerr << " lhs: " << lhs;                                   \
        std::cerr << ", rhs: " << rhs << std::endl;                     \
    }

#define ASSERT_GREATER(lhs, rhs)                                                  \
    if (!(is_greater(lhs, rhs)))                                                  \
    {                                                                             \
        std::cerr << "Asserted relation 'lhs > rhs' wrong at line: " << __LINE__; \
        std::cerr << " lhs: " << lhs;                                             \
        std::cerr << ", rhs: " << rhs << std::endl;                               \
        std::exit(-1);                                                            \
    }

#define EXPECT_GREATER(lhs, rhs)                                                  \
    if (!(is_greater(lhs, rhs)))                                                  \
    {                                                                             \
        std::cerr << "Exepcted relation 'lhs > rhs' wrong at line: " << __LINE__; \
        std::cerr << " lhs: " << lhs;                                             \
        std::cerr << ", rhs: " << rhs << std::endl;                               \
    }

#define ASSERT_GEQ(lhs, rhs)                                                          \
    if (!(is_greater(lhs, rhs)) and !Utopia::Models::Amee::Utils(::is_equallhs, rhs)) \
    {                                                                                 \
        std::cerr << "Asserted relation 'lhs <= rhs' wrong at line: " << __LINE__;    \
        std::cerr << " lhs: " << lhs;                                                 \
        std::cerr << ", rhs: " << rhs << std::endl;                                   \
        std::exit(-1);                                                                \
    }

#define EXPECT_GEQ(lhs, rhs)                                                       \
    if (!(is_greater(lhs, rhs)) and !is_equal(lhs, rhs))                           \
    {                                                                              \
        std::cerr << "Exepcted relation 'lhs <= rhs' wrong at line: " << __LINE__; \
        std::cerr << " lhs: " << lhs;                                              \
        std::cerr << ", rhs: " << rhs << std::endl;                                \
    }

#define ASSERT_LESS(lhs, rhs)                                                     \
    if (!(is_less(lhs, rhs)))                                                     \
    {                                                                             \
        std::cerr << "Asserted relation 'lhs < rhs' wrong at line: " << __LINE__; \
        std::cerr << " lhs: " << lhs;                                             \
        std::cerr << ", rhs: " << rhs << std::endl;                               \
        std::exit(-1);                                                            \
    }

#define EXPECT_LESS(lhs, rhs)                                                     \
    if (!(is_less(lhs, rhs)))                                                     \
    {                                                                             \
        std::cerr << "Exepcted relation 'lhs < rhs' wrong at line: " << __LINE__; \
        std::cerr << " lhs: " << lhs;                                             \
        std::cerr << ", rhs: " << rhs << std::endl;                               \
    }

#define ASSERT_LEQ(lhs, rhs)                                                       \
    if (!(is_less(lhs, rhs)) and !is_equal(lhs, rhs))                              \
    {                                                                              \
        std::cerr << "Asserted relation 'lhs <= rhs' wrong at line: " << __LINE__; \
        std::cerr << " lhs: " << lhs;                                              \
        std::cerr << ", rhs: " << rhs << std::endl;                                \
        std::exit(-1);                                                             \
    }

#define EXPECT_LEQ(lhs, rhs)                                                       \
    if (!(is_less(lhs, rhs)) and !is_equal(lhs, rhs))                              \
    {                                                                              \
        std::cerr << "Exepcted relation 'lhs <= rhs' wrong at line: " << __LINE__; \
        std::cerr << " lhs: " << lhs;                                              \
        std::cerr << ", rhs: " << rhs << std::endl;                                \
    }

} // namespace Utils
} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia