#include <cassert>
#include <iostream>

#include "../agent_new.hh"

// Test state struct
struct AgentState {
    int foo;
};

using namespace Utopia;
using Utopia::DefaultSpace;
using SpaceVec = typename DefaultSpace::SpaceVec;

using AgentTraitsSync = AgentTraits<AgentState, UpdateMode::sync>;
using AgentTraitsAsync = AgentTraits<AgentState, UpdateMode::async>;

int main(int, char**) {

    try {
        // Initial position for agent setup
        SpaceVec initial_pos{4.2, 0.0};

        // Initial state for agent setup
        AgentState State;
        State.foo = 42;

        // -------------------------------------------------------------------
        
        std::cout << "------ Testing agent setup and member functions... ------"
                  << std::endl;

        // Initialize a synchronous and an asynchronous agent
        std::cout << "Initialize sync and async agent ..." << std::endl;

        // Create synchronous and asynchronous agent
        __Agent<AgentTraitsSync, DefaultSpace> agt_sync(0, State, initial_pos);
        __Agent<AgentTraitsAsync, DefaultSpace> agt_async(0, State, initial_pos);

        std::cout << "Success." << std::endl;

        // Check if the positions were set correctly
        std::cout << "Checking initial positions ..." << std::endl;

        assert(all(agt_sync.position() == initial_pos));
        assert(all(agt_sync.position_new() == initial_pos));

        assert(all(agt_async.position() == initial_pos));

        std::cout << "Success." << std::endl;

    

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