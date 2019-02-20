#include "../agent_new.hh"

struct AgentState {
    int foo;
};

using namespace Utopia;
using Utopia::DefaultSpace;

using AgentTraitsSync = AgentTraits<AgentState, UpdateMode::sync>;
using AgentTraitsAsync = AgentTraits<AgentState, UpdateMode::async>;

int main(int, char**) {
    AgentState State;
    State.foo = 42;
    __Agent<AgentTraitsSync, DefaultSpace> agent_sync(0, State);
    auto agent_async = __Agent<AgentTraitsAsync, DefaultSpace>(0, State);

}