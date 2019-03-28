#ifndef UTOPIA_TEST_AGENT_MANAGER_INTEGRATION_TEST_HH
#define UTOPIA_TEST_AGENT_MANAGER_INTEGRATION_TEST_HH

#include <utopia/core/model.hh>
#include <utopia/core/agent_manager.hh>


namespace Utopia {
namespace Test {

/// Dummy agent state type
struct AgentState {
    /// Some property
    int foo;

    /// The (required) config constructor
    AgentState(const DataIO::Config& cfg)
    :
        foo(get_as<int>("foo", cfg))
    {}
};

/// Specialize the agent traits struct with a dummy agent state type
using AgentTraits = Utopia::AgentTraits<AgentState, Update::sync>;

/// Define data types for the agent manager test model
using AMTestModelTypes = ModelTypes<DefaultRNG, DefaultSpace>;

/// Model to test function and integration of AgentManager into a model
class AMTest:
    public Model<AMTest, AMTestModelTypes>
{
public:
    /// The base model class
    using Base = Model<AMTest, AMTestModelTypes>;

    /// Public agent manager (for easier testing)
    AgentManager<AgentTraits, AMTest> am;

public:
    /// Construct the test model with an initial state
    /** \param state Initial state of the model
     */
    template<class ParentModel>
    AMTest (const std::string name, const ParentModel &parent_model)
    :
        // Pass arguments to the base class constructor
        Base(name, parent_model),
        am(*this)
    {}

    void perform_step () {}

    void monitor () {}

    void write_data () {}
};

} // namespace Test
} // namespace Utopia


#endif // UTOPIA_TEST_AGENT_MANAGER_INTEGRATION_TEST_HH
