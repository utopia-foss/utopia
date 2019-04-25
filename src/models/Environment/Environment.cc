#include <iostream>

#include "Environment.hh"

using namespace Utopia::Models::Environment;

/// A non-abstract EnvCellState, derived from the base class
/** \note This is used to allow standalone operation.
  */
struct EnvCellState : BaseEnvCellState
{
    double some_parameter;

    EnvCellState(const Utopia::DataIO::Config& cfg)
    : 
        some_parameter(Utopia::get_as<double>("some_parameter",
                                              cfg, 0.))
    { }

    ~EnvCellState() = default;

    /// Getter
    double get_env(const std::string& key) const override {
        if (key == "some_parameter") {
            return some_parameter;
        }
        throw std::invalid_argument("No access method for key '" + key
                                    + "' in EnvCellState!");
    }

    /// Setter
    void set_env(const std::string& key,
                 const double& value) override
    {
        if (key == "some_parameter") {
            some_parameter = value;
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
        // Use the constructed EnvCellState and don't associate with the 
        // PseudoParents cells (they don't exist).
        auto model = Environment<EnvCellState, false>("Environment", pp);
        model.track_parameter("some_parameter");
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
