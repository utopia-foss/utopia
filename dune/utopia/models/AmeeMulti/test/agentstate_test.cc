#include "../agentstate.hh"
#include "../utils/agentstate_test_utils.hh"
#include "../utils/test_utils.hh"
#include <random>
#include <vector>

using Trait = std::vector<double>;
using RNG = std::mt19937;
using AS = Agentstate<Cell, Trait, RNG>;

using namespace Utopia::Models::AmeeMulti;
using namespace Utopia::Models::AmeeMulti::Utils;

int main()
{
    AS state(Trait{1., 2., 2., 3., 5.}, std::make_shared<Cell>(3, 4),
             std::make_shared<RNG>(359874), 2, 6, 0.532);

    AS childstate(state, std::vector<double>{0.9. 0.9, 0.25});
    return 0;
}