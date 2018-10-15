#ifndef UTOPIA_MODELS_AMEEMULTI_AGENTSTATE_TEST_UTILS_HH
#define UTOPIA_MODELS_AMEEMULTI_AGENTSTATE_TEST_UTILS_HH
#include "utils.hh"
#include <cmath>
namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
using namespace Utils;

struct Cell
{
    int x;
    int y;
    Cell() = default;
    Cell(int a, int b) : x(a), y(b)
    {
    }
    Cell(const Cell& other) = default;
    Cell(Cell&& other) = default;
    Cell& operator=(Cell&& other) = default;
    Cell& operator=(const Cell& other) = default;
    virtual ~Cell() = default;
};

template <template <typename...> class Agentstate, typename Cell, typename Policy>
bool operator==(const Agentstate<Cell, Policy>& lhs, const Agentstate<Cell, Policy>& rhs)
{
    return (lhs.rng == rhs.rng) && (is_equal(lhs.resources, rhs.resources)) &&
           (lhs.fitness == rhs.fitness) && (lhs.sumlen == rhs.sumlen) &&
           (is_equal(lhs.divisor, rhs.divisor)) && (lhs.start == rhs.start) &&
           (lhs.end == rhs.end) && (is_equal(lhs.adaption, rhs.adaption)) &&
           (is_equal(lhs.intensity, rhs.intensity)) && (lhs.age == rhs.age) &&
           (lhs.habitat == rhs.habitat) && (lhs.deathflag == rhs.deathflag) &&
           (lhs.genotype == rhs.genotype) && (lhs.phenotype == rhs.phenotype) &&
           (lhs.start_mod == rhs.start_mod) && (lhs.end_mod == rhs.end_mod);
}

} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif