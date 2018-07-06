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

        
        // -- Assert that exceptions are raised -- //
        std::cout << std::endl
                  <<"Checking for correct exceptions being thrown ..."
                  << std::endl;

        // Bad type conversion, string as double
        try {
            as_double(cfg["foo"]);
        }
        catch (YAML::Exception& e) {
            // is the expected exception
            std::string e_msg = e.what();
            std::cout << "  Got error message: " << e_msg << std::endl;
            
            // check message includes line information
            assert(e_msg.find("yaml-cpp: error at line") >= 0);
            assert(e_msg.find("matches the desired type conversion") >= 0);
        }
        catch (...) {
            std::cerr << "Wrong exception type thrown!" << std::endl;
            return 1;
        }

        // Zombie node
        try {
            as_double(cfg["i_do_not_exist"]);
        }
        catch (YAML::Exception& e) {
            // is the expected exception
            // this message cannot include line information due to being zombie
            std::string e_msg = e.what();
            std::cout << "  Got error message: " << e_msg << std::endl;

            // check the error message hints at the zombie node
            assert(e_msg.find("yaml-cpp: error at line") == -1); // no mark!
            assert(e_msg.find("Perhaps the node was a zombie?") >= 0);
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
