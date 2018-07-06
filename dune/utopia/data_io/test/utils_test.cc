#include <cassert>
#include <iostream>

#include <yaml-cpp/yaml.h>

#include <dune/utopia/data_io/utils.hh>

bool str_found(std::string search, std::string find) {
    return search.find(find) != std::string::npos;
}


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

        // vector
        std::vector vec({1, 2, 3});
        assert(as_<std::vector<int>>(cfg["a_vector"]) == vec);
        
        // array
        std::array<std::array<int,2>,2> a1({{{{1, 2}}, {{3, 4}}}});
        auto a2 = as_<std::array<std::array<int,2>,2>>(cfg["an_array"]);
        assert(a1 == a2);
        // assert(as_<std::array<std::array<int,2>,2>>(cfg["an_array"]) == arr);
        
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
            assert(str_found(e_msg, "yaml-cpp: error at line"));
            assert(str_found(e_msg, "matches the desired type conversion"));
            assert(str_found(e_msg, "The value of the node is:  bar"));
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
            assert(!str_found(e_msg, "yaml-cpp: error at line")); // no mark
            assert(str_found(e_msg, "Perhaps the node was a zombie?"));
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
