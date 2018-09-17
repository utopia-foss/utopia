
#include "../agentstates/agentstate.hh"
#include "../agentstates/agentstate_policy_highlevel.hh"
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
using APC = Agentstate_policy_highlevel<Genotype, Phenotype, RNG>;
using State = AgentState<Cell, APC>;

int main()
{
    auto rng = std::make_shared<RNG>(7564382);
    auto cell = std::make_shared<Cell>(Cell());
    std::vector<double> mtr{1e-8, 0.95, 1e-2};

    State state = State(Genotype{1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4,
                                 1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4},
                        cell, 5., rng);
    Phenotype expected =
        Phenotype{1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4,
                  1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4};

    State copied(state);
    State copy_assigned = state;
    State dummy = state;
    State moveassigned = std::move(dummy);
    State moveconstructed(
        State(Genotype{1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4,
                       1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4},
              cell, 5., rng));
    State child(state, 1., mtr);

    // adam constructor
    ASSERT_EQ(int(state.sumlen), 0);
    ASSERT_EQ(state.divisor, 0.);
    ASSERT_EQ(state.start, 1);
    ASSERT_EQ(state.end, 2);
    ASSERT_EQ(state.intensity, 3.);
    ASSERT_EQ(int(state.age), 0);
    ASSERT_EQ(int(state.fitness), 0);
    assert(state.habitat == cell);
    ASSERT_EQ(state.resources, 5.);
    ASSERT_EQ(state.phenotype, expected);

    // copy, move, move assignment, copy assignment
    assert(state == copied);
    assert(state == copy_assigned);
    assert(state == moveassigned);
    assert(state == moveconstructed);

    // reproduction constructor
    ASSERT_EQ(int(child.sumlen), 0);
    ASSERT_EQ(child.divisor, 0.);
    ASSERT_EQ(child.start, 1);
    ASSERT_EQ(child.end, 2);
    ASSERT_EQ(child.intensity, 3.);
    ASSERT_EQ(int(child.age), 0);
    ASSERT_EQ(int(child.fitness), 0);
    assert(child.habitat == cell);
    ASSERT_EQ(child.resources, 1.);
    ASSERT_NEQ(child.genotype, state.genotype);
    ASSERT_NEQ(child.phenotype, state.phenotype);

    return 0;
}