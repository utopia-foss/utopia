#include <cmath>
#include <cassert>
#include <iostream>

#include "agent_manager_test.hh"
#include "testtools.hh"

using namespace Utopia::Test;

int main(int, char *[]) {
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
        
        // Initialize the mock model with config-constructible agent state
        std::cout << "... DataIO::Config-constructible state" << std::endl;
        MockModel<AgentTraitsCC> mm_cc("mm_cc", cfg["config"]);
        std::cout << "Success." << std::endl << std::endl;
        
        // Initialize the mock model with config-constructible agent state
        std::cout << "... DataIO::Config-constructible state (with RNG)"
                  << std::endl;
        MockModel<AgentTraitsRC> mm_rc("mm_rc", cfg["config_with_RNG"]);
        std::cout << "Success." << std::endl << std::endl;
        
        // Initialize the mock model with config-constructible agent state
        std::cout << "... only explicitly constructible state" << std::endl;
        const auto initial_state = AgentStateEC(2.34, "foobar", true);
        MockModel<AgentTraitsEC> mm_ec("mm_ec", cfg["explicit"],
                                       initial_state);
        std::cout << "Success." << std::endl << std::endl;
        

        // -------------------------------------------------------------------
        
        std::cout << "------ Testing agent initialization ... ------"
                  << std::endl;

        // Use the manager with default constructible agent state for that
        std::cout << "Checking number of agents ..." << std::endl;
        
        MockModel<AgentTraitsDC> mm_it1("mm_it1", cfg["init_test1"]);
        assert(mm_it1.agents().size() == 234);
        
        std::cout << "Correct." << std::endl << std::endl;

        std::cout << "Checking that agent positions cover whole space ..."
                  << std::endl;
        
        // Calculate relative positions
        using SpaceVec = MockModel<AgentTraitsDC>::SpaceVec;
        std::vector<SpaceVec> rel_positions;

        for (const auto& a : mm_it1.agents()) {
            rel_positions.push_back(a->position() / mm_it1.space()->extent);
        }

        // ... and the mean relative position
        SpaceVec pos_acc({0., 0.});
        for (const auto& rp : rel_positions) {
            pos_acc += rp;
        }
        const auto mean_rel_pos = pos_acc / rel_positions.size();
        std::cout << "Mean relative agent position: " << std::endl
                  << mean_rel_pos;
        assert(0.45 < mean_rel_pos[0] < 0.55);

        // ... as well as the standard deviation
        SpaceVec dev_acc({0., 0.});
        for (const auto& rp : rel_positions) {
            dev_acc += pow((rp - mean_rel_pos), 2);
        }
        const auto std_rel_pos = sqrt(pos_acc / (rel_positions.size()-1));

        std::cout << "Standard deviation of relative agent position: "
                  << std::endl << std_rel_pos;
        assert(0.35 < std_rel_pos[0] < 0.5); // TODO Which value to use here?

        std::cout << "Correct." << std::endl << std::endl;




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
