#include <assert.h>
#include <iostream>

#include "../Environment.hh"
#include "../env_param_func_collection.hh"
#include "../env_state_func_collection.hh"

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
    using EnvStateFunc = typename Model::EnvStateFunc;
    using EnvParamFunc = typename Model::EnvParamFunc;

    // Add a custom lambda
    model.add_env_param_func("test_param", []() { return -1; },
                             "some_glob_parameter", {true, true, {}});

    EnvParamFunc epf = []() { return -2; };

    // Add with update mode
    model.add_env_param_func("another param test", epf, "some_glob_parameter",
                             {true, true, {}});


    // Add a custom lambda
    model.add_env_state_func("test_state",
        [](const auto& env_cell) {
            auto& env_state = env_cell->state;
            env_state.set_env("some_het_parameter", -1.);
            return env_state;
        }
    );

    EnvStateFunc esf = [](const auto& env_cell) {
        auto& env_state = env_cell->state;
        env_state.set_env("some_het_parameter", -1.);
        return env_state;
    };

    // Add with update mode
    model.add_env_state_func("another state test", esf, Update::async);

    // Add initial env func
    model.template add_env_state_func<true>("initial state", esf);
    // will not be invoked, but this still tests the interface

    // Use getter
    double my_param = model.get_parameter("some_glob_parameter");
    (void)(my_param);

    // Iterate
    model.iterate();
}

/// A non-abstract EnvParam, derived from the base class
/** \note This is used to allow standalone operation.
  */
struct EnvParam : BaseEnvParam
{
    double some_glob_parameter;

    EnvParam(const Utopia::DataIO::Config& cfg)
    : 
        some_glob_parameter(Utopia::get_as<double>("some_glob_parameter",
                                                     cfg, 0.))
    { }

    ~EnvParam() = default;

    /// Getter
    double get_env(const std::string& key) const override {
        if (key == "some_glob_parameter") {
            return some_glob_parameter;
        }
        throw std::invalid_argument("No access method for key '" + key
                                    + "' in EnvParam!");
    }

    /// Setter
    void set_env(const std::string& key,
                 const double& value) override
    {
        if (key == "some_glob_parameter") {
            some_glob_parameter = value;
        }
        else {
            throw std::invalid_argument("No setter method for key '" + key
                                        + "' in EnvParam!");
        }
    }
};

/// Create a non-abstract EnvCellState
struct EnvCellState : BaseEnvCellState
{
    double some_het_parameter;

    EnvCellState(const Utopia::DataIO::Config& cfg)
    : 
        some_het_parameter(Utopia::get_as<double>("some_het_parameter",
                                                cfg, 0.))
    { }

    ~EnvCellState() { }

    /// Getter
    double get_env(const std::string& key) const {
        if (key == "some_het_parameter") {
            return some_het_parameter;
        }
        throw std::invalid_argument("No access method to the key '" 
                                    + key + "' in EnvCellState!");
    }

    /// Setter
    void set_env(const std::string& key, const double& value) {
        if (key == "some_het_parameter") {
            some_het_parameter = value;
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
        // Use the constructed EnvParam and EnvCellState; Have the model in
        // standalone mode.
        auto model = Environment<EnvParam, EnvCellState, true>(
                            "Environment", pp);

        // use the register function
        model.track_state("some_het_parameter");
        model.track_parameter("some_glob_parameter");

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
