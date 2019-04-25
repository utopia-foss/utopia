#include <assert.h>
#include <iostream>

#include "../Environment.hh"

using namespace Utopia;
using namespace Utopia::Models::Environment;

// Function that asserts equality within an epsilon range
template <typename T>
void assert_eq(T arg1, T arg2, const double epsilon=1e-12) {
    T diff = std::abs(arg1 - arg2);
    assert(diff < epsilon);
}

template <typename Model>
void test_add_env_func(Model& model) {
    using EnvFunc = typename Model::EnvFunc;

    // Add a custom lambda
    model.add_env_func("test",
        [](const auto& env_cell) {
            auto& env_state = env_cell->state;
            env_state.set_env("some_parameter", -1.);
            return env_state;
        }
    );

    EnvFunc ef = [](const auto& env_cell) {
        auto& env_state = env_cell->state;
        env_state.set_env("some_parameter", -1.);
        return env_state;
    };

    // Add with update mode
    model.add_env_func("another test", ef, Update::async);

    // Add initial env func
    model.template add_env_func<true>("initial", ef);
    // will not be invoked, but this still tests the interface

    // Iterate
    model.iterate();
}

/// Create a non-abstract EnvCellState
struct EnvCellState : Utopia::Models::Environment::BaseEnvCellState
{
    double some_parameter;

    EnvCellState(const Utopia::DataIO::Config& cfg)
    : 
        some_parameter(Utopia::get_as<double>("some_parameter",
                                                cfg, 0.))
    { }

    ~EnvCellState() { }

    /// Getter
    double get_env(const std::string& key) const {
        if (key == "some_parameter") {
            return some_parameter;
        }
        throw std::invalid_argument("No access method to the key '" 
                                    + key + "' in EnvCellState!");
    }

    /// Setter
    void set_env(const std::string& key, const double& value) {
        if (key == "some_parameter") {
            some_parameter = value;
            return;
        }
        throw std::invalid_argument("No setter method to the key '"
                                    + key + "' in EnvCellState!");
    }
};

int main ()
{
    try {
        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent pp("test.yml");

        // Initialize the main model instance and directly run it
        // Use the constructed EnvCellState and don't associate with the 
        // PseudoParents cells (they don't exist).
        auto model = Environment<EnvCellState, false>("Environment", pp);

        // use the register function
        model.track_parameter("some_parameter");

        // Use the push rule function
        test_add_env_func(model);
        
        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Exception occurred!" << std::endl;
        return 1;
    }
}
