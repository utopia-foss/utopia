
#include "../agent_state_simple.hh"
#include "../utils/generators.hh"
#include "../utilsagentstate_test_utils.hh"
#include <cassert>
#include <iostream>
#include <random>

using namespace Utopia::Models::AmeeMulti;
using namespace Utopia::Models::AmeeMulti::Utils;

using Genotype = std::vector<int>;
using Phenotype = std::vector<double>;
using RNG = Xoroshiro<>;
using State = AgentStateSimple<Cell, Genotype, Phenotype, RNG>;

int main()
{
    auto rng = std::make_shared<RNG>(7564382);
    auto cell = std::make_shared<Cell>(Cell());
    std::vector<double> mtr{0.95, 1e-4, 1e-8}; // last entry unused for State

    State state = State(Genotype{1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4,
                                 1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4},
                        cell, 5., rng);

    assert(state.sumlen == 4);
    assert(std::abs(state.divisor - 5.) < 1e-16);
    assert(state.start == 1);
    assert(state.end == 4);
    assert(std::abs(state.intensity - 3.) < 1e-16);
    assert(state.age == 0);
    assert(state.fitness == 0);
    assert(state.habitat == cell);
    assert(std::abs(state.resources - 5.) < 1e-16);
    Phenotype expected = Phenotype{2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4};
    assert(state.phenotype == expected);

    State copied(state);
    assert(state == copied);

    State copy_assigned = state;
    assert(state == copy_assigned);

    State dummy = state;

    State moveassigned = std::move(dummy);
    assert(state == moveassigned);

    State moveconstructed(
        State(Genotype{1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4,
                       1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4},
              cell, 5., rng));
    assert(state == moveconstructed);

    State child(state, 1., mtr);
    assert(child.sumlen == 4);
    assert(std::abs(child.divisor - 5.) < 1e-16);
    assert(child.start == 1);
    assert(child.end == 4);
    assert(std::abs(child.intensity - 3.) < 1e-16);
    assert(child.age == 0);
    assert(child.fitness == 0);
    assert(child.habitat == cell);
    assert(std::abs(child.resources - 1.) < 1e-16);
    assert(child.genotype != state.genotype);
    assert(child.phenotype != state.phenotype);

    return 0;
}