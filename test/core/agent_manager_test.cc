#define BOOST_TEST_MODULE agent manager test

#include <cmath>
#include <cassert>
#include <iostream>
#include <algorithm>

#include <armadillo>

#include <boost/test/unit_test.hpp>
#include <utopia/core/testtools.hh>

#include "agent_manager_test.hh"

namespace Utopia {

using namespace Utopia::Test::AgentManager;
using namespace Utopia::TestTools;

// -- Types -------------------------------------------------------------------
using SpaceVec = MockModel<AgentTraitsDC>::SpaceVec;  // same for all


// -- Fixtures ----------------------------------------------------------------

struct Infrastructure : public BaseInfrastructure<> {

    Infrastructure() : BaseInfrastructure<>("agent_manager_test.yml") {}
};


struct AgentManagers : public Infrastructure {
    MockModel<AgentTraitsDC> mm_dc;
    MockModel<AgentTraitsCC> mm_cc;
    MockModel<AgentTraitsRC> mm_rc;
    MockModel<AgentTraitsEC> mm_ec;

    AgentManagers() :
        Infrastructure()
    ,   mm_dc("mm_dc", cfg["default"])
    ,   mm_cc("mm_cc", cfg["config"])
    ,   mm_rc("mm_rc", cfg["config_with_RNG"])
    ,   mm_ec("mm_ec", cfg["explicit"], AgentStateEC(2.34, "foobar", true))
    {}
};


// ++ Tests +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

BOOST_AUTO_TEST_SUITE(test_basics)

/// Tests construction using the fixture-defined AgentManager instances
BOOST_FIXTURE_TEST_CASE(test_construction, AgentManagers) {
    BOOST_TEST(mm_dc._am.id_counter() == 42);
    BOOST_TEST(mm_cc._am.id_counter() == 42);
    BOOST_TEST(mm_rc._am.id_counter() == 42);
    BOOST_TEST(mm_ec._am.id_counter() == 42);

    // construction with custom config
    MockModel<AgentTraitsCC> mm_cc_custom(
        "mm_cc_custom",
        cfg["default"],     // agent manager config from model is ignored
        cfg["custom"]       // ... and this agent manager config is used
    );
    BOOST_TEST(mm_cc_custom._am.id_counter() == 123);

    MockModel<AgentTraitsEC> mm_ec_custom(
        "mm_cc_custom",
        cfg["explicit"],    // agent manager config from model is ignored
        AgentStateEC(2.34, "foobar", true),
        cfg["custom"]       // ... and this agent manager config is used
    );
    BOOST_TEST(mm_ec_custom._am.id_counter() == 123);
}


BOOST_FIXTURE_TEST_CASE(test_agent_init, Infrastructure) {
    // Use the manager with default constructible agent state for that
    BOOST_TEST_MESSAGE("Checking number of agents ...");

    MockModel<AgentTraitsDC> mm_it1("mm_it1", cfg["init_test1"]);
    BOOST_TEST(mm_it1._am.agents().size() == 234);

    BOOST_TEST_MESSAGE("Checking that agent positions cover whole space ...");

    // Calculate relative positions
    using SpaceVec = MockModel<AgentTraitsDC>::SpaceVec;
    std::vector<SpaceVec> rel_positions;

    for (const auto& a : mm_it1._am.agents()) {
        rel_positions.push_back(a->position() / mm_it1._am.space()->extent);
    }

    // ... and the mean relative position
    SpaceVec pos_acc({0., 0.});
    for (const auto& rp : rel_positions) {
        pos_acc += rp;
    }
    const auto mean_rel_pos = pos_acc / rel_positions.size();
    mean_rel_pos.print("Mean relative agent position");
    BOOST_CHECK_CLOSE(mean_rel_pos[0], 0.5, 10.);  // 10% relative tolerance
    BOOST_CHECK_CLOSE(mean_rel_pos[1], 0.5, 10.);

    // ... as well as the standard deviation
    // NOTE The expected std. dev. of a uniform distr. is 1/sqrt(12) = 0.2887…
    SpaceVec dev_acc({0., 0.});
    for (const auto& rp : rel_positions) {
        dev_acc += pow((rp - mean_rel_pos), 2);
    }
    const SpaceVec std_rel_pos =
        arma::sqrt(dev_acc / (rel_positions.size()-1));
    std_rel_pos.print("Standard deviation of relative agent position");

    const auto expected_std = 1./sqrt(12.);
    BOOST_CHECK_CLOSE(std_rel_pos[0], expected_std, 10.);
    BOOST_CHECK_CLOSE(std_rel_pos[1], expected_std, 10.);
}


BOOST_FIXTURE_TEST_CASE(test_add_agent_overloads, AgentManagers) {
    // Spacevector for testing
    SpaceVec zerovec({0.0, 0.0});

    // Test for states that are constructed via default constructor...
    BOOST_TEST_MESSAGE("Check for config constructible");
    // ... with a specified position
    auto agt_dc  = mm_dc._am.add_agent(zerovec);
    BOOST_TEST(mm_dc._am.agents().size() == 43);
    // ... with a random position
    agt_dc = mm_dc._am.add_agent();
    BOOST_TEST(mm_dc._am.agents().size() == 44);

    // Test for states that are constructed by passing a config
    BOOST_TEST_MESSAGE("Check for config constructible");
    auto agt_cc = mm_cc._am.add_agent(zerovec);
    BOOST_TEST(mm_cc._am.agents().size() == 43);
    // ... with a random position
    agt_cc = mm_cc._am.add_agent();
    BOOST_TEST(mm_cc._am.agents().size() == 44);

    // Test for states that are constructed with config node and rng
    BOOST_TEST_MESSAGE("Check for config constructible with RNG");
    auto agt_rc = mm_rc._am.add_agent(zerovec);
    BOOST_TEST(mm_rc._am.agents().size() == 43);
    // ... with a random position
    agt_rc = mm_rc._am.add_agent();
    BOOST_TEST(mm_rc._am.agents().size() == 44);
}


BOOST_FIXTURE_TEST_CASE(test_add_and_remove_agent, Infrastructure) {
    MockModel<AgentTraitsDC> mm_add_remove_periodic("mm_add_remove_periodic",
                                                    cfg["default"]);

    // Add an agent and save for inspection
    BOOST_TEST_MESSAGE("Add an agent to the managed container");
    auto new_agt = mm_add_remove_periodic._am.add_agent(AgentStateDC(),
                                                        SpaceVec({0., 0.}));

    // Check wether the id counter was set correctly
    BOOST_TEST(mm_add_remove_periodic._am.id_counter() == 43);

    // Check wether there are 43 agents in the managed container
    BOOST_TEST(mm_add_remove_periodic._am.agents().size() == 43);


    // Make sure the agent has the right id
    BOOST_TEST(new_agt->id() == 42);

    // Remove the agent from the managed container
    BOOST_TEST_MESSAGE("Remove the agent from the managed container");
    mm_add_remove_periodic._am.remove_agent(new_agt);

    // Get the agent container
    auto agts = mm_add_remove_periodic._am.agents();

    // Make sure the agent container is back to its originial size
    BOOST_TEST(mm_add_remove_periodic._am.agents().size() == 42);

    // Check that the removed agent is not in the container anymore
    BOOST_TEST(
        bool(std::find(agts.cbegin(), agts.cend(), new_agt) == agts.cend())
    );

    BOOST_TEST_MESSAGE("Remove agents that fulfill some condition");
    // Remove agents under a certain condtion
    mm_add_remove_periodic._am.erase_agent_if(
                           [&] (auto agent) {return agent->id() % 2 == 0;});

    // Make sure only agents with odd ids are left over
    for (unsigned long i = 0; i < 21;  i++) {
        BOOST_TEST(mm_add_remove_periodic._am.agents()[i]->id() == 2 * i + 1);
    };
}

BOOST_AUTO_TEST_SUITE_END()


// -- Space-related tests -----------------------------------------------------
BOOST_AUTO_TEST_SUITE(test_embedding_in_space)

BOOST_FIXTURE_TEST_CASE(test_move_sync_periodic, Infrastructure) {

    MockModel<AgentTraitsCC_sync>
        mm_dyn_sync_periodic("mm_dyn_sync_periodic",
                             cfg["mm_dyn_sync_periodic_test"]);

    BOOST_TEST_MESSAGE("Checking that agents' positions are different ...");
    const auto agents = mm_dyn_sync_periodic._am.agents();
    BOOST_TEST(agents[0]->position()[0] != agents[1]->position()[0]);
    BOOST_TEST(agents[0]->position()[1] != agents[1]->position()[1]);

    BOOST_TEST_MESSAGE("Checking that move_to does not act immediately in "
                       "synchronous update...");
    auto agent = agents[0];
    auto& am = mm_dyn_sync_periodic._am;

    SpaceVec new_pos({0.2, 0.3});

    am.move_to(agent, new_pos);
    BOOST_TEST(agent->position()[0] != new_pos[0]);
    BOOST_TEST(agent->position()[1] != new_pos[1]);

    BOOST_TEST_MESSAGE(
        "...but after the agent's update, the positions should be updated! :)"
    );
    am.update_agents();
    BOOST_TEST(agent->position()[0] == new_pos[0]);
    BOOST_TEST(agent->position()[1] == new_pos[1]);

    BOOST_TEST_MESSAGE("Checking that move_by does not act immediately in "
                       "synchronous update...");
    am.move_by(agent, new_pos);
    BOOST_TEST(agent->position()[0] == new_pos[0]);
    BOOST_TEST(agent->position()[1] == new_pos[1]);

    BOOST_TEST_MESSAGE("...but after the agent's update, the positions should "
                       "be updated! :)");
    am.update_agents();
    BOOST_TEST(agent->position()[0] == new_pos[0] * 2.);
    BOOST_TEST(agent->position()[1] == new_pos[1] * 2.);
}


BOOST_FIXTURE_TEST_CASE(test_move_async_periodic, Infrastructure) {
    MockModel<AgentTraitsCC_async> mm_dyn_async_periodic(
                                    "mm_dyn_async_periodic",
                                    cfg["mm_dyn_async_periodic_test"]);

    BOOST_TEST_MESSAGE("Checking that agents' positions are different ...");
    const auto agents = mm_dyn_async_periodic._am.agents();
    BOOST_TEST(agents[0]->position()[0] != agents[1]->position()[0]);
    BOOST_TEST(agents[0]->position()[1] != agents[1]->position()[1]);

    BOOST_TEST_MESSAGE("Checking that move_to works nicely for the "
                       "asynchronous update...");
    auto agent = agents[0];
    auto& am = mm_dyn_async_periodic._am;

    SpaceVec new_pos({0.2, 0.3});

    am.move_to(agent, new_pos);
    BOOST_TEST(agent->position()[0] == new_pos[0]);
    BOOST_TEST(agent->position()[1] == new_pos[1]);

    BOOST_TEST_MESSAGE("Checking that move_by works nicely for the "
                       "asynchronous update...");
    am.move_by(agent, new_pos);
    BOOST_TEST(agent->position()[0] == new_pos[0] * 2.);
    BOOST_TEST(agent->position()[1] == new_pos[1] * 2.);

    BOOST_TEST_MESSAGE("Checking that a movement across the border is "
                       "correctly mapped into the space...");
    // Note that the space has the extent (2,3)
    am.move_to(agent, {3., 4.});
    BOOST_TEST(agent->position()[0] == 1.);
    BOOST_TEST(agent->position()[1] == 1.);

    am.move_by(agent, {-3, -3});
    BOOST_TEST(agent->position()[0] == 0.);
    BOOST_TEST(agent->position()[1] == 1.);
}


BOOST_FIXTURE_TEST_CASE(test_move_sync_nonperiodic, Infrastructure) {

    MockModel<AgentTraitsCC_sync>
        mm_dyn_sync_nonperiodic("mm_dyn_sync_nonperiodic",
                                cfg["mm_dyn_sync_nonperiodic_test"]);

    BOOST_TEST_MESSAGE("Checking that agents' positions are different ...");
    const auto agents = mm_dyn_sync_nonperiodic._am.agents();
    BOOST_TEST(agents[0]->position()[0] != agents[1]->position()[0]);
    BOOST_TEST(agents[0]->position()[1] != agents[1]->position()[1]);

    BOOST_TEST_MESSAGE("Checking that move_to does not act immediately in "
                       "synchronous update...");
    auto agent = agents[0];
    auto& am = mm_dyn_sync_nonperiodic._am;

    SpaceVec new_pos({0.2, 0.3});

    am.move_to(agent, new_pos);
    BOOST_TEST(agent->position()[0] != new_pos[0]);
    BOOST_TEST(agent->position()[1] != new_pos[1]);

    BOOST_TEST_MESSAGE("...but after the agent's update, the positions should "
                       "be updated! :)");
    am.update_agents();
    BOOST_TEST(agent->position()[0] == new_pos[0]);
    BOOST_TEST(agent->position()[1] == new_pos[1]);

    BOOST_TEST_MESSAGE("Checking that move_by does not act immediately in "
                       "synchronous update...");
    am.move_by(agent, new_pos);
    BOOST_TEST(agent->position()[0] == new_pos[0]);
    BOOST_TEST(agent->position()[1] == new_pos[1]);

    BOOST_TEST_MESSAGE("...but after the agent's update, the positions should "
                       "be updated! :)");
    am.update_agents();
    BOOST_TEST(agent->position()[0] == new_pos[0] * 2.);
    BOOST_TEST(agent->position()[1] == new_pos[1] * 2.);

    // OutOfSpace exception for invalid position
    check_exception<Utopia::OutOfSpace>(
        [&](){
            am.move_to(agent, {5.,5.});
        },
        "Could not move agent!"
    );

}


BOOST_FIXTURE_TEST_CASE(test_move_async_nonperiodic, Infrastructure) {
    MockModel<AgentTraitsCC_async>
        mm_dyn_async_nonperiodic("mm_dyn_async_nonperiodic",
                                 cfg["mm_dyn_async_nonperiodic_test"]);

    BOOST_TEST_MESSAGE("Checking that agents' positions are different ...");
    const auto agents = mm_dyn_async_nonperiodic._am.agents();
    BOOST_TEST(agents[0]->position()[0] != agents[1]->position()[0]);
    BOOST_TEST(agents[0]->position()[1] != agents[1]->position()[1]);

    BOOST_TEST_MESSAGE("Checking that move_to works nicely for the "
                       "asynchronous update...");
    auto agent = agents[0];
    auto& am = mm_dyn_async_nonperiodic._am;

    SpaceVec new_pos({0.2, 0.3});

    am.move_to(agent, new_pos);
    BOOST_TEST(agent->position()[0] == new_pos[0]);
    BOOST_TEST(agent->position()[1] == new_pos[1]);

    BOOST_TEST_MESSAGE("Checking that move_by works nicely for the "
                       "asynchronous update...");
    am.move_by(agent, new_pos);
    BOOST_TEST(agent->position()[0] == new_pos[0] * 2.);
    BOOST_TEST(agent->position()[1] == new_pos[1] * 2.);

    check_exception<Utopia::OutOfSpace>(
        [&](){
            am.move_to(agent, {5.,5.});
        },
        "Could not move agent!"
    );
}


/// Using a non-square periodic grid, check that agent positions are correct
BOOST_FIXTURE_TEST_CASE(test_displacement_and_distance, AgentManagers,
                        *utf::tolerance(1.e-10))
{
    MockModel<AgentTraitsCC_sync> mm("mm", cfg["mm_dyn_sync_periodic_test"]);

    // consistency check: Have two agents with component-wise unequal positions
    auto& am = mm._am;
    auto& agents = am.agents();
    BOOST_TEST(agents.size() == 2);
    auto& a0 = agents[0];
    auto& a1 = agents[1];

    BOOST_TEST(a0->position()[0] != a1->position()[0]);
    BOOST_TEST(a0->position()[1] != a1->position()[1]);

    // interface check
    BOOST_TEST(arma::norm(am.displacement(a0, a0)) == 0.);
    BOOST_TEST(arma::norm(am.displacement(a0, a1)) > 0.);

    BOOST_TEST(am.distance(a0, a0) == 0.);
    BOOST_TEST(am.distance(a0, a1) > 0.);

    BOOST_TEST(am.distance(a0, a0, 1) == 0.);
    BOOST_TEST(am.distance(a0, a1, 1) > 0.);

    // exact numerical check, also across boundaries
    BOOST_TEST(am.space()->extent[0] == 2.);
    BOOST_TEST(am.space()->extent[1] == 3.);

    // .. of distance
    am.move_to(a0, {0.1, 0.1});
    am.move_to(a1, {1.9, 0.1});
    BOOST_TEST(am.distance(a0, a1) != 0.2);
    am.update_agents();
    BOOST_TEST(am.distance(a0, a1) == 0.2);

    am.move_to(a0, {1.9, 2.9});
    am.move_to(a1, {1.9, 0.1});
    am.update_agents();
    BOOST_TEST(am.distance(a0, a1) == 0.2);

    // .. of displacement
    const auto test_approx_equal = [](auto p1, auto p2, double prec = 1.e-8) {
        BOOST_TEST_CONTEXT(
            "Checking approx equality of:\n" << p1 << "\nand:\n" << p2 << "\n"
        ){
            BOOST_TEST(arma::approx_equal(p1, p2, "reldiff", prec));
        }
    };

    am.move_to(a0, {0.5, 0.5});
    am.move_to(a1, {1.0, 1.5});
    am.update_agents();
    test_approx_equal(am.displacement(a0, a1), SpaceVec({0.5, 1.}));
    test_approx_equal(am.displacement(a0, a1), -am.displacement(a1, a0));

    am.move_to(a0, {0.2, 0.1});
    am.move_to(a1, {1.7, 2.9});
    am.update_agents();
    test_approx_equal(am.displacement(a0, a1), SpaceVec({-0.5, -0.2}));
    test_approx_equal(am.displacement(a0, a1), -am.displacement(a1, a0));
}


/// Test the spatial neighborhood of agents is correctly represented
BOOST_FIXTURE_TEST_CASE(test_neighbors, AgentManagers, *utf::tolerance(1.e-10))
{
    MockModel<AgentTraitsCC_sync> mm("mm", cfg["mm_dyn_sync_periodic_test"]);

    // consistency check: Have two agents with component-wise unequal positions
    auto& am = mm._am;
    auto& agents = am.agents();
    BOOST_TEST(agents.size() == 2);
    auto& a0 = agents[0];
    auto& a1 = agents[1];

    BOOST_TEST(a0->position()[0] != a1->position()[0]);
    BOOST_TEST(a0->position()[1] != a1->position()[1]);

    BOOST_TEST(am.space()->extent[0] == 2.);
    BOOST_TEST(am.space()->extent[1] == 3.);

    // check neighborhood relations hold (also across boundaries)
    // .. set position to specific values
    am.move_to(a0, {0.1, 0.1});
    am.move_to(a1, {1.9, 0.1});
    am.update_agents();
    BOOST_TEST(am.distance(a0, a1) == 0.2);

    // .. now check they are in each other's neighborhood for a sufficiently
    //    large radius
    auto nbs0 = am.neighbors_of(a0, 0.25);
    auto nbs1 = am.neighbors_of(a1, 0.25);

    BOOST_TEST(nbs0.size() == 1);
    BOOST_TEST(nbs1.size() == 1);

    BOOST_TEST(nbs0[0]->id() == a1->id());
    BOOST_TEST(nbs1[0]->id() == a0->id());

    // .. how about a smaller radius, though? Should have an empty neighborhood
    BOOST_TEST(am.neighbors_of(a0, 0.1).size() == 0);
    BOOST_TEST(am.neighbors_of(a1, 0.1).size() == 0);

    // .. check again for a different position in space, not across a boundary
    am.move_to(a0, {1.0, 1.0});
    am.move_to(a1, {1.5, 1.5});
    am.update_agents();

    auto distance = am.distance(a0, a1);
    BOOST_TEST(am.neighbors_of(a0, distance).size() == 1);
    BOOST_TEST(am.neighbors_of(a1, distance).size() == 1);

    BOOST_TEST(am.neighbors_of(a0, distance - 0.01).size() == 0);
    BOOST_TEST(am.neighbors_of(a1, distance - 0.01).size() == 0);

    // .. once more with many agents
    for (auto i = 0u; i < 98; i++) {
        am.add_agent();
    }
    BOOST_TEST(am.agents().size() == (2 + 98));
    BOOST_TEST(am.neighbors_of(am.agents()[0], 10000.).size() == 100 - 1);
    BOOST_TEST(am.neighbors_of(am.agents()[0], 0.).size() == 0);
}

BOOST_AUTO_TEST_SUITE_END()  // space-embedding

}  // namespace
