#ifndef AGENTSTATE_TEST_UTILS_HH
#define AGENTSTATE_TEST_UTILS_HH
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

template <template <typename...> class Agentstate, typename Cell, typename Genotype, typename Phenotype, typename Rng, typename Policy>
bool operator==(const Agentstate<Cell, Genotype, Phenotype, Rng, Policy>& lhs,
                const Agentstate<Cell, Genotype, Phenotype, Rng, Policy>& rhs)
{
    return (lhs.rng == rhs.rng) && (is_equal(lhs.resources, rhs.resources)) &&
           (lhs.fitness == rhs.fitness) && (lhs.sumlen == rhs.sumlen) &&
           (is_equal(lhs.divisor, rhs.divisor)) && (lhs.start == rhs.start) &&
           (lhs.end == rhs.end) && (is_equal(lhs.adaption, rhs.adaption)) &&
           (is_equal(lhs.intensity, rhs.intensity)) && (lhs.age == rhs.age) &&
           (lhs.habitat == rhs.habitat) && (lhs.deathflag == rhs.deathflag) &&
           (lhs.genotype == rhs.genotype) && (lhs.phenotype == rhs.phenotype);
}

} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif