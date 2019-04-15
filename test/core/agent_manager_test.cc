#include <cmath>
#include <cassert>
#include <iostream>
#include <algorithm>

#include <utopia/core/exceptions.hh>

#include "agent_manager_test.hh"
#include "testtools.hh"

using namespace Utopia::Test;


int main() {
    try {
        std::cout << "Getting config file ..." << std::endl;
        auto cfg = YAML::LoadFile("agent_manager_test.yml");
        std::cout << "Success." << std::endl << std::endl;

        // -------------------------------------------------------------------
        std::cout << "------ Testing mock model initialization via ... ------"
                  << std::endl;
        
        // Initialize the mock model with default-constructible agent state
        std::cout << "... default-constructible state" << std::endl;
        MockModel<AgentTraitsDC> mm_dc("mm_dc", cfg["default"]);
        std::cout << "Success." << std::endl << std::endl;

        assert(mm_dc._am.id_counter() == 42);
        
        // Initialize the mock model with config-constructible agent state
        std::cout << "... DataIO::Config-constructible state" << std::endl;
        MockModel<AgentTraitsCC> mm_cc("mm_cc", cfg["config"]);
        std::cout << "Success." << std::endl << std::endl;
        
        assert(mm_cc._am.id_counter() == 42);

        // Initialize the mock model with config-constructible agent state
        std::cout << "... DataIO::Config-constructible state (with RNG)"
                  << std::endl;
        MockModel<AgentTraitsRC> mm_rc("mm_rc", cfg["config_with_RNG"]);
        std::cout << "Success." << std::endl << std::endl;

        assert(mm_rc._am.id_counter() == 42);
        
        // Initialize the mock model with config-constructible agent state
        std::cout << "... only explicitly constructible state" << std::endl;
        const auto initial_state = AgentStateEC(2.34, "foobar", true);
        MockModel<AgentTraitsEC> mm_ec("mm_ec", cfg["explicit"],
                                       initial_state);

        assert(mm_ec._am.id_counter() == 42);
        
        std::cout << "Success." << std::endl << std::endl;
        
        // -------------------------------------------------------------------
        
        std::cout << "------ Testing agent initialization ... ------"
                  << std::endl;

        // Use the manager with default constructible agent state for that
        std::cout << "Checking number of agents ..." << std::endl;
        
        MockModel<AgentTraitsDC> mm_it1("mm_it1", cfg["init_test1"]);
        assert(mm_it1._am.agents().size() == 234);
        
        std::cout << "Correct." << std::endl << std::endl;

        std::cout << "Checking that agent positions cover whole space ..."
                  << std::endl;
        
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
        std::cout << "Mean relative agent position: " << std::endl
                  << mean_rel_pos;
        assert((0.45 < mean_rel_pos[0])  and (mean_rel_pos[0] < 0.55));

        // ... as well as the standard deviation
        SpaceVec dev_acc({0., 0.});
        for (const auto& rp : rel_positions) {
            dev_acc += pow((rp - mean_rel_pos), 2);
        }
        const auto std_rel_pos = sqrt(dev_acc / (rel_positions.size()-1));

        std::cout << "Standard deviation of relative agent position: "
                  << std::endl << std_rel_pos;
        assert((1./sqrt(12.) - 0.1 < std_rel_pos[0]) and 
               (std_rel_pos[0] < 1./sqrt(12.) + 0.1));
        // Note: The expected std of a uniform distribution is 1/sqrt(12)

        std::cout << "Correct." << std::endl << std::endl;

        // -------------------------------------------------------------------
        std::cout << "------ Testing overloads for add_agent ... ------"
                  << std::endl;  

        // Spacevector for testing
        SpaceVec zerovec({0.0, 0.0});

        // Test for states that are constructed via default constructor...
        std::cout << "Check for config constructible" << std::endl;
        // ... with a specified position
        auto agt_dc  = mm_dc._am.add_agent(zerovec);
        assert(mm_dc._am.agents().size() == 43);
        // ... with a random position
        agt_dc = mm_dc._am.add_agent();
        assert(mm_dc._am.agents().size() == 44);  
        
        // Test for states that are constructed by passing a config
        std::cout << "Check for config constructible" << std::endl;
        auto agt_cc = mm_cc._am.add_agent(zerovec);
        assert(mm_cc._am.agents().size() == 43);
        // ... with a random position
        agt_cc = mm_cc._am.add_agent();
        assert(mm_cc._am.agents().size() == 44);  

        // Test for states that are constructed with config node and rng
        std::cout << "Check for config constructible with RNG" << std::endl;
        auto agt_rc = mm_rc._am.add_agent(zerovec);
        assert(mm_rc._am.agents().size() == 43);
        // ... with a random position
        agt_rc = mm_rc._am.add_agent();
        assert(mm_rc._am.agents().size() == 44);

        std::cout << "Works fine." << std::endl << std::endl;    

        // -------------------------------------------------------------------
        std::cout << "------ Testing removal and adding of agents"
                     "... ------" << std::endl;
        
        // Create a scope to prevent hanging variables afterwards
        {
        std::cout << "Create a test model " << std::endl;
        MockModel<AgentTraitsDC> mm_add_remove_periodic(
                                        "mm_add_remove_periodic", 
                                        cfg["default"]);

        // Add an agent and save fpr inspection
        std::cout << "Add an agent to the managed container" << std::endl;
        auto new_agt = mm_add_remove_periodic._am.add_agent(AgentStateDC(), 
                                                            SpaceVec({0., 0.}));        
        
        // Check wether the id counter was set correctly
        assert(mm_add_remove_periodic._am.id_counter() == 43);

        // Check wether there are 43 agents in the managed container
        assert(mm_add_remove_periodic._am.agents().size() == 43);


        
        // Make sure the agent has the right id
        assert(new_agt->id() == 42);

        // Remove the agent from the managed container
        std::cout << "Remove the agent from the managed container" << std::endl;
        mm_add_remove_periodic._am.remove_agent(new_agt);

        // Get the agent container
        auto agts = mm_add_remove_periodic._am.agents();

        // Make sure the agent container is back to its originial size
        assert(mm_add_remove_periodic._am.agents().size() == 42);

        // Check that the removed agent is not in the container anymore
        assert(std::find(agts.cbegin(), agts.cend(), new_agt) == agts.cend());

        std::cout << "Remove agents that fulfill some condition" << std::endl;
        // Remove agents under a certain condtion
        mm_add_remove_periodic._am.erase_agent_if(
                               [&] (auto agent) {return agent->id() % 2 == 0;});
        
        // Make sure only agents with odd ids are left over
        for (unsigned long i = 0; i < 21;  i++) {
            assert(mm_add_remove_periodic._am.agents()[i]->id() == 2 * i + 1); 
        };

        std::cout << "So far, so good." << std::endl << std::endl; 

        };
        // -------------------------------------------------------------------
        
        std::cout << "------ Testing agent dynamics (synchronous, periodic)"
                     "... ------" << std::endl;
        
        // Create a scope to prevent hanging variables afterwards
        { 
        std::cout << "Create a test model " << std::endl;
        MockModel<AgentTraitsCC_sync> mm_dyn_sync_periodic(
                                        "mm_dyn_sync_periodic", 
                                        cfg["mm_dyn_sync_periodic_test"]);
        
        std::cout << "Checking that agent's positions are different ..."
                  << std::endl;

        const auto agents = mm_dyn_sync_periodic._am.agents();
        assert(agents[0]->position()[0] != agents[1]->position()[0]);
        assert(agents[0]->position()[1] != agents[1]->position()[1]);

        std::cout << "Checking that move_to does not work immediately in "
                     "synchronous update..."
                  << std::endl;

        auto agent = agents[0];
        auto& am = mm_dyn_sync_periodic._am;

        SpaceVec new_pos({0.2, 0.3});

        am.move_to(agent, new_pos);
        assert(agent->position()[0] != new_pos[0]);
        assert(agent->position()[1] != new_pos[1]);

        std::cout << "...but after the agent's update, the positions should be "
                     "updated! :)"
                  << std::endl;

        am.update_agents();
        assert(agent->position()[0] == new_pos[0]);
        assert(agent->position()[1] == new_pos[1]);

        std::cout << "Checking that move_by does not work immediately in "
                     "synchronous update..."
                  << std::endl;

        am.move_by(agent, new_pos);
        assert(agent->position()[0] == new_pos[0]);
        assert(agent->position()[1] == new_pos[1]);

        std::cout << "...but after the agent's update, the positions should be "
                     "updated! :)"
                  << std::endl;

        am.update_agents();
        assert(agent->position()[0] == new_pos[0] * 2.);
        assert(agent->position()[1] == new_pos[1] * 2.);

        std::cout << "Checking that the id counter works..."
                  << std::endl;

        std::cout << "Correct." << std::endl << std::endl;
        };


        std::cout << "------ Testing agent dynamics (asynchronous, periodic)... ------"
                  << std::endl;
        
        // Create a scope to prevent hanging variables afterwards
        { 
        std::cout << "Create a test model " << std::endl;
        MockModel<AgentTraitsCC_async> mm_dyn_async_periodic(
                                        "mm_dyn_async_periodic", 
                                        cfg["mm_dyn_async_periodic_test"]);
        
        std::cout << "Checking that agent's positions are different ..."
                  << std::endl;

        const auto agents = mm_dyn_async_periodic._am.agents();
        assert(agents[0]->position()[0] != agents[1]->position()[0]);
        assert(agents[0]->position()[1] != agents[1]->position()[1]);

        std::cout << "Checking that move_to works nicely for the asynchronous "
                     "update..."
                  << std::endl;

        auto agent = agents[0];
        auto& am = mm_dyn_async_periodic._am;

        SpaceVec new_pos({0.2, 0.3});

        am.move_to(agent, new_pos);
        assert(agent->position()[0] == new_pos[0]);
        assert(agent->position()[1] == new_pos[1]);

        std::cout << "Checking that move_by works nicely for the asynchronous "
                     "update..."
                  << std::endl;

        am.move_by(agent, new_pos);
        assert(agent->position()[0] == new_pos[0] * 2.);
        assert(agent->position()[1] == new_pos[1] * 2.);
    
        std::cout << "Checking that a movement across the border is correctly "
                     "mapped into the space..."
                  << std::endl;

        // Note that the space has the extent (2,3)
        am.move_to(agent, {3., 4.});
        assert(agent->position()[0] == 1.);
        assert(agent->position()[1] == 1.);

        am.move_by(agent, {-3, -3});
        assert(agent->position()[0] == 0.);
        assert(agent->position()[1] == 1.);

        std::cout << "Checking that the id counter works..."
                  << std::endl;

        std::cout << "Correct." << std::endl << std::endl;
        };


        std::cout << "------ Testing agent dynamics (synchronous, nonperiodic)... ------"
                  << std::endl;
        
        // Create a scope to prevent hanging variables afterwards
        { 
        std::cout << "Create a test model " << std::endl;
        MockModel<AgentTraitsCC_sync> mm_dyn_sync_nonperiodic(
                                        "mm_dyn_sync_nonperiodic", 
                                        cfg["mm_dyn_sync_nonperiodic_test"]);
        
        std::cout << "Checking that agent's positions are different ..."
                  << std::endl;

        const auto agents = mm_dyn_sync_nonperiodic._am.agents();
        assert(agents[0]->position()[0] != agents[1]->position()[0]);
        assert(agents[0]->position()[1] != agents[1]->position()[1]);

        std::cout << "Checking that move_to does not work immediately in "
                     "synchronous update..."
                  << std::endl;

        auto agent = agents[0];
        auto& am = mm_dyn_sync_nonperiodic._am;

        SpaceVec new_pos({0.2, 0.3});

        am.move_to(agent, new_pos);
        assert(agent->position()[0] != new_pos[0]);
        assert(agent->position()[1] != new_pos[1]);

        std::cout << "...but after the agent's update, the positions should be "
                     "updated! :)"
                  << std::endl;

        am.update_agents();
        assert(agent->position()[0] == new_pos[0]);
        assert(agent->position()[1] == new_pos[1]);

        std::cout << "Checking that move_by does not work immediately in "
                     "synchronous update..."
                  << std::endl;

        am.move_by(agent, new_pos);
        assert(agent->position()[0] == new_pos[0]);
        assert(agent->position()[1] == new_pos[1]);

        std::cout << "...but after the agent's update, the positions should be "
                     "updated! :)"
                  << std::endl;

        am.update_agents();
        assert(agent->position()[0] == new_pos[0] * 2.);
        assert(agent->position()[1] == new_pos[1] * 2.);

        // Check error messages are as expected
        assert(check_error_message<Utopia::OutOfSpace>(
            "OutOfSpace exception for invalid position",
            [&](){
                am.move_to(agent, {5.,5.});
            },
            "Could not move agent!", "", true)
        );

        std::cout << "Correct." << std::endl << std::endl;
        };


        std::cout << "------ Testing agent dynamics (asynchronous, nonperiodic)... ------"
                  << std::endl;
        
        // Create a scope to prevent hanging variables afterwards
        { 
        std::cout << "Create a test model " << std::endl;
        MockModel<AgentTraitsCC_async> mm_dyn_async_nonperiodic(
                                        "mm_dyn_async_nonperiodic", 
                                        cfg["mm_dyn_async_nonperiodic_test"]);
        
        std::cout << "Checking that agent's positions are different ..."
                  << std::endl;

        const auto agents = mm_dyn_async_nonperiodic._am.agents();
        assert(agents[0]->position()[0] != agents[1]->position()[0]);
        assert(agents[0]->position()[1] != agents[1]->position()[1]);

        std::cout << "Checking that move_to works nicely for the asynchronous "
                     "update..."
                  << std::endl;

        auto agent = agents[0];
        auto& am = mm_dyn_async_nonperiodic._am;

        SpaceVec new_pos({0.2, 0.3});

        am.move_to(agent, new_pos);
        assert(agent->position()[0] == new_pos[0]);
        assert(agent->position()[1] == new_pos[1]);

        std::cout << "Checking that move_by works nicely for the asynchronous "
                     "update..."
                  << std::endl;

        am.move_by(agent, new_pos);
        assert(agent->position()[0] == new_pos[0] * 2.);
        assert(agent->position()[1] == new_pos[1] * 2.);

        // Check error messages are as expected
        assert(check_error_message<Utopia::OutOfSpace>(
            "OutOfSpace exception for invalid position",
            [&](){
                am.move_to(agent, {5.,5.});
            },
            "Could not move agent!", "", true)
        );
        
        std::cout << "Correct." << std::endl << std::endl;
        };

        // -------------------------------------------------------------------
        // Done.
        std::cout << "------ Total success. ------" << std::endl << std::endl;
        return 0;
    }
    catch (std::exception& e) {
        std::cerr << "Exception occurred: " << e.what() << std::endl;
        // NOTE cannot call cleanup here because the scope is not shared
        return 1;
    }
    catch (...) {
        std::cout << "Exception occurred!" << std::endl;
        // NOTE cannot call cleanup here because the scope is not shared
        return 1;
    }
}
