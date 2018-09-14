#include "../agentstates/agentstate_gauss.hh"
#include "../utils/agentstate_test_utils.hh"
#include "../utils/test_utils.hh"
#include <random>
#include <vector>

using namespace Utopia::Models::AmeeMulti;
using namespace Utopia::Models::AmeeMulti::Utils;
using Trait = std::vector<double>;
using RNG = std::mt19937;
using AS = AgentStateGauss<Cell, Trait, RNG>;

int main()
{
    AS adam(Trait{1., 2., 2., 3., 5.}, std::make_shared<Cell>(3, 4), 1.,
            std::make_shared<RNG>(359874));

    adam.start = 2;
    adam.end = 6;
    adam.intensity = 0.532;

    std::vector<double> mutr{0.9, 0.9, 0.25};
    AS childstate(adam, 1.5, mutr);
    return 0;
}