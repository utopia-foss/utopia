
#include "../agentstates/agentstate.hh"
#include "../agentstates/agentstate_policy_simple.hh"
#include "../utils/agentstate_test_utils.hh"
#include "../utils/generators.hh"
#include "../utils/test_utils.hh"
#include <cassert>
#include <iostream>
#include <random>

using namespace Utopia::Models::AmeeMulti;
using namespace Utopia::Models::AmeeMulti::Utils;

using Genotype = std::vector<double>;
using Phenotype = std::vector<double>;
using RNG = Xoroshiro<>;
using APC = Agentstate_policy_simple<Genotype, Phenotype, RNG>;
using State = AgentState<Cell, Genotype, Phenotype, RNG, APC>;

int main()
{
    auto rng = std::make_shared<RNG>(7564382);
    auto cell = std::make_shared<Cell>(Cell());
    std::vector<double> mtr{0.95, 1e-4, 1e-8}; // last entry unused for State

    State state = State(Genotype{1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4,
                                 1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4},
                        cell, 5., rng);
    Phenotype expected = Phenotype{2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4};

    State copied(state);
    State copy_assigned = state;
    State dummy = state;
    State moveassigned = std::move(dummy);
    State moveconstructed(
        State(Genotype{1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4,
                       1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4},
              cell, 5., rng));
    State child(state, 1., mtr);

    ASSERT_EQ(int(state.sumlen), 4);
    ASSERT_EQ(state.divisor, 5.);
    ASSERT_EQ(state.start, 1);
    ASSERT_EQ(state.end, 4);
    ASSERT_EQ(state.intensity, 3.);
    ASSERT_EQ(int(state.age), 0);
    ASSERT_EQ(int(state.fitness), 0);
    assert(state.habitat == cell);
    ASSERT_EQ(state.resources, 5.);
    ASSERT_EQ(state.phenotype, expected);

    assert(state == copied);
    assert(state == copy_assigned);
    assert(state == moveassigned);
    assert(state == moveconstructed);

    ASSERT_EQ(int(child.sumlen), 4);
    ASSERT_EQ(child.divisor, 5.);
    ASSERT_EQ(child.start, 1);
    ASSERT_EQ(child.end, 4);
    ASSERT_EQ(child.intensity, 3.);
    ASSERT_EQ(int(child.age), 0);
    ASSERT_EQ(int(child.fitness), 0);
    assert(child.habitat == cell);
    ASSERT_EQ(child.resources, 1.);
    ASSERT_NEQ(child.genotype, state.genotype);
    ASSERT_NEQ(child.phenotype, state.phenotype);

    return 0;
}