
#include "../agentstates/agentstate.hh"
#include "../agentstates/agentstate_policy_complex.hh"
#include "../utils/agentstate_test_utils.hh"
#include "../utils/generators.hh"
#include "../utils/test_utils.hh"
#include <cassert>
#include <iostream>
#include <random>

using namespace Utopia::Models::AmeeMulti;
using namespace Utopia::Models::AmeeMulti::Utils;

using Genotype = std::vector<int>;
using Phenotype = std::vector<double>;
using RNG = Xoroshiro<>;
using APC = Agentstate_policy_complex<Genotype, Phenotype, RNG>;
using State = AgentState<Cell, APC>;

int main()
{
    auto rng = std::make_shared<RNG>(7564382);
    auto cell = std::make_shared<Cell>(Cell());
    std::vector<double> mtr{0.95, 1e-4, 1e-8}; // last entry unused for State

    State state = State(Genotype{1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4,
                                 1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4,
                                 1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4},
                        cell, 5., rng);

    State state2 =
        State(Genotype{1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4,
                       1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4},
              cell, 5., rng);

    State copied(state);
    State dummy = state;
    State copy_assigned = state;
    State moveassigned = std::move(dummy);
    State moveconstructed(
        State(Genotype{1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4,
                       1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4,
                       1, 2, 3, 3, 5, -2, -5, 5, 5, 3, 5, 6, 7, 3, 4},
              cell, 5., rng));

    State child(state, 1., mtr);

    // adam constructor
    ASSERT_EQ(int(state.sumlen), 4);
    ASSERT_EQ(state.divisor, 5.);
    ASSERT_EQ(state.start, 1);
    ASSERT_EQ(state.end, 4);
    ASSERT_EQ(state.start_mod, 3);
    ASSERT_EQ(state.end_mod, 3);
    ASSERT_EQ(state.intensity, 0.6);
    ASSERT_EQ(state.phenotype, (std::vector<double>{4.2, 2., 1.8, 1.6, 4.2, 0.8}));
    ASSERT_EQ(int(state.age), 0);
    ASSERT_EQ(int(state.fitness), 0);
    assert(state.habitat == cell);
    ASSERT_EQ(state.resources, 5.);

    // second adam constructor with cutoff
    ASSERT_EQ(int(state2.sumlen), 4);
    ASSERT_EQ(state2.divisor, 5.);
    ASSERT_EQ(state2.start, 1);
    ASSERT_EQ(state2.end, 2);
    ASSERT_EQ(state2.start_mod, 3);
    ASSERT_EQ(state2.end_mod, 3);
    ASSERT_EQ(state2.intensity, 0.6);
    ASSERT_EQ(state2.phenotype, (std::vector<double>{4.2, 1.4}));
    ASSERT_EQ(int(state2.age), 0);
    ASSERT_EQ(int(state2.fitness), 0);
    assert(state2.habitat == cell);
    ASSERT_EQ(state2.resources, 5.);

    // copy, move copy_assignment, move assignment
    // not usable with custom asserts, because of output operator
    assert(state == copied);
    assert(state == copy_assigned);
    assert(state == moveassigned);
    assert(state == moveconstructed);

    // reproduction constructor
    ASSERT_EQ(int(child.sumlen), 4);
    ASSERT_EQ(child.divisor, 5.);
    ASSERT_EQ(child.start, 1);
    ASSERT_EQ(child.end, 4);
    ASSERT_EQ(child.start_mod, 2); // originally is 3, but has been put to 2 because start_mod > end_mod => start_mod = end_mod
    ASSERT_EQ(child.end_mod, 2);
    ASSERT_EQ(child.intensity, 0.6);
    ASSERT_EQ(int(child.age), 0);
    ASSERT_EQ(int(child.fitness), 0);
    assert(child.habitat == cell);
    ASSERT_EQ(child.resources, 1.);
    ASSERT_NEQ(child.genotype, state.genotype);
    ASSERT_EQ(child.phenotype, state.phenotype);

    return 0;
}