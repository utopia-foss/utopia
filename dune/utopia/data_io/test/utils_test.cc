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

        // String access
        assert(as_<std::string>(cfg["foo"]) == "bar");
        assert(as_str(cfg["spam"]) == "eggs");

        // Double, bool, int
        assert(as_double(cfg["a_double"]) == 3.14159);
        assert(as_bool(cfg["a_bool"]));
        assert(as_<int>(cfg["an_int"]) == 42);

        
        // Assert that exceptions are raised
        // Bad conversion
        try {
            as_double(cfg["foo"]);
        }
        catch (YAML::BadConversion& e) {
            std::cerr << "(line above is the expected error message) "
                      << std::endl << std::endl;
        }
        catch (...) {
            std::cerr << "Wrong exception type thrown!" << std::endl;
            return 1;
        }


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
