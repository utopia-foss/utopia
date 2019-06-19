#include <iostream>

#include "Environment.hh"

using namespace Utopia::Models::Environment;

/// A non-abstract EnvParam, derived from the base class
/** \note This is used to allow standalone operation.
  */
struct EnvParam : BaseEnvParam
{
    double some_global_parameter;

    EnvParam(const Utopia::DataIO::Config& cfg)
    : 
        some_global_parameter(Utopia::get_as<double>("some_global_parameter",
                                                     cfg, 0.))
    { }

    ~EnvParam() = default;

    /// Getter
    double get_env(const std::string& key) const override {
        if (key == "some_global_parameter") {
            return some_global_parameter;
        }
        throw std::invalid_argument("No access method for key '" + key
                                    + "' in EnvParam!");
    }

    /// Setter
    void set_env(const std::string& key,
                 const double& value) override
    {
        if (key == "some_global_parameter") {
            some_global_parameter = value;
        }
        else {
            throw std::invalid_argument("No setter method for key '" + key
                                        + "' in EnvParam!");
        }
    }
};


/// A non-abstract EnvCellState, derived from the base class
/** \note This is used to allow standalone operation.
  */
struct EnvCellState : BaseEnvCellState
{
    double some_heterogeneous_parameter;

    EnvCellState(const Utopia::DataIO::Config& cfg)
    : 
        some_heterogeneous_parameter(
            Utopia::get_as<double>("some_heterogeneous_parameter", cfg, 0.))
    { }

    ~EnvCellState() = default;

    /// Getter
    double get_env(const std::string& key) const override {
        if (key == "some_heterogeneous_parameter") {
            return some_heterogeneous_parameter;
        }
        throw std::invalid_argument("No access method for key '" + key
                                    + "' in EnvCellState!");
    }

    /// Setter
    void set_env(const std::string& key,
                 const double& value) override
    {
        if (key == "some_heterogeneous_parameter") {
            some_heterogeneous_parameter = value;
        }
        else {
            throw std::invalid_argument("No setter method for key '" + key
                                        + "' in EnvCellState!");
        }
    }
};

int main (int, char** argv) {
    try {
        // Initialize the PseudoParent from a config file path
        Utopia::PseudoParent pp(argv[1]);

        // Initialize the main model instance and directly run it
        // Use the constructed EnvParam and EnvCellState; Have the model in
        // standalone mode.
        auto model =
            Environment<EnvParam, EnvCellState, true>("Environment", pp);

        model.track_parameter("some_global_parameter");
        model.track_state("some_heterogeneous_parameter");
        
        model.run();

        // Done.
        return 0;
    }
    catch (Utopia::Exception& e) {
        return Utopia::handle_exception(e);
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
