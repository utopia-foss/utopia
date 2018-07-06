#include <cassert>
#include <iostream>

#include <yaml-cpp/yaml.h>

#include <dune/utopia/data_io/utils.hh>

int main()
{
    try {
        using namespace Utopia::DataIO;

        std::cout << "Loading test config file..." << std::endl;
        auto cfg = YAML::LoadFile("utils_test.yml");
        std::cout << "  Loaded" << std::endl << std::endl;

        // -- Tests -- //
        std::cout << "Commencing tests ..." << std::endl;

        auto foo = as_<std::string>(cfg["foo"]);

        std::cout << "Tests successful." << std::endl;
        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cout << "Exception occurred!" << std::endl;
        return 1;
    }
}
