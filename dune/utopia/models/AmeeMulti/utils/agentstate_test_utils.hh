#ifndef AGENTSTATE_TEST_UTILS_HH
#define AGENTSTATE_TEST_UTILS_HH
#include <cmath>
namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
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

template <template <typename...> class Agentstate, typename Cell, typename Phenotype, typename Rng>
bool operator==(const Agentstate<Cell, Phenotype, Rng>& lhs,
                const Agentstate<Cell, Phenotype, Rng>& rhs)
{
    return (lhs.rng == rhs.rng) && (std::abs(lhs.resources - rhs.resources) < 1e-16) &&
           (lhs.fitness == rhs.fitness) && (lhs.start == rhs.start) &&
           (lhs.end == rhs.end) && (std::abs(lhs.adaption - rhs.adaption) < 1e-16) &&
           (std::abs(lhs.intensity - rhs.intensity) < 1e-16) &&
           (lhs.age == rhs.age) && (lhs.habitat == rhs.habitat) &&
           (lhs.deathflag == rhs.deathflag) && (lhs.trait == rhs.trait);
}

} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia
#endif