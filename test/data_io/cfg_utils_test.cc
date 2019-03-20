#include <cassert>
#include <iostream>

#include <yaml-cpp/yaml.h>

#include <utopia/data_io/cfg_utils.hh>
#include <utopia/core/exceptions.hh>


using namespace Utopia;

/// Helper function to check for match in string s
bool str_found(std::string s, std::string match)
{
    return s.find(match) != std::string::npos;
}

int main () {
    try {
        std::cout << "Loading test config file..." << std::endl;
        auto cfg = YAML::LoadFile("cfg_utils_test.yml");
        std::cout << "Done." << std::endl << std::endl;

        // -- Test get_as method ----------------------------------------------
        std::cout << "----- Checking get_as_method ... -----" << std::endl;

        { // Local test scope

        // String access
        assert(get_as<std::string>("foo", cfg) == "bar");
        assert(get_as<std::string>("spam", cfg) == "eggs");

        // Double, bool, int
        assert(get_as<double>("a_double", cfg) == 3.14159);
        assert(get_as<bool>("a_bool", cfg));
        assert(get_as<int>("an_int", cfg) == 42);

        // vector
        std::vector vec({1, 2, 3});
        assert(get_as<std::vector<int>>("a_vector", cfg) == vec);

        // array
        std::array<std::array<int, 2>, 2> a1({{{{1, 2}}, {{3, 4}}}});

        auto a2 = get_as<std::array<std::array<int, 2>, 2>>("an_array", cfg);
        assert(a1 == a2);

        // Optional value returned if KeyError was raised
        assert(get_as<std::string>("foo", cfg, "foo") == "bar"); // foo exists
        assert(get_as<std::string>("not_a_key", cfg, "foo") == "foo");

        // armadillo vector
        auto sv1 = get_as_SpaceVec<3>("a_vector", cfg);
        assert(sv1[0] == 1.);
        assert(sv1[1] == 2.);
        assert(sv1[2] == 3.);

        auto mi1 = get_as_MultiIndex<3>("a_vector", cfg);
        assert(mi1[0] == 1);
        assert(mi1[1] == 2);
        assert(mi1[2] == 3);

        std::cout << "Success." << std::endl << std::endl;

        } // End of local test scope



        std::cout << "----- Checking KeyError ... -----" << std::endl;
        
        // Key missing
        try {
            get_as<bool>("i_do_not_exist", cfg);
        }
        catch (Utopia::KeyError& e) {
            // is the expected exception
            std::string e_msg = e.what();
            std::cout << "  Got error message: " << e_msg << std::endl;
        
            assert(str_found(e_msg, "The content of the given node is"));

            std::cout << "  ... as expected" << std::endl << std::endl;
        }
        catch (...) {
            std::cerr << "Wrong exception type thrown!" << std::endl;
            return 1;
        }

        // Zombie node
        try {
            get_as<bool>("invalid_key2", cfg["invalid_key1"]);
        }
        catch (Utopia::KeyError& e) {
            // is the expected exception
            std::string e_msg = e.what();
            std::cout << "  Got error message: " << e_msg << std::endl;
        
            assert(str_found(e_msg, "The given node is a Zombie!"));

            std::cout << "  ... as expected" << std::endl << std::endl;
        }
        catch (...) {
            std::cerr << "Wrong exception type thrown!" << std::endl;
            return 1;
        }

        // Empty node
        try {
            get_as<bool>("some_key", cfg["empty_map"]);
        }
        catch (Utopia::KeyError& e) {
            // is the expected exception
            std::string e_msg = e.what();
            std::cout << "  Got error message: " << e_msg << std::endl;
        
            assert(str_found(e_msg, "The given node contains no entries!"));

            std::cout << "  ... as expected" << std::endl << std::endl;
        }
        catch (...) {
            std::cerr << "Wrong exception type thrown!" << std::endl;
            return 1;
        }

        // Conversion error still thrown
        try {
            get_as<double>("foo", cfg);
        }
        catch (YAML::Exception& e) {
            // is the expected exception
            std::string e_msg = e.what();
            std::cout << "  Got error message: " << e_msg << std::endl;
        
            assert(str_found(e_msg, "Got YAML::TypedBadConversion<double>"));

            std::cout << "  ... as expected" << std::endl << std::endl;
        }
        catch (...) {
            std::cerr << "Wrong exception type thrown!" << std::endl;
            return 1;
        }

        // Conversion error still thrown, even with default given
        try {
            get_as<double>("foo", cfg, 3.14);
        }
        catch (YAML::Exception& e) {
            // is the expected exception
            std::string e_msg = e.what();
            std::cout << "  Got error message: " << e_msg << std::endl;
        
            assert(str_found(e_msg, "Got YAML::TypedBadConversion<double>"));

            std::cout << "  ... as expected" << std::endl << std::endl;
        }
        catch (...) {
            std::cerr << "Wrong exception type thrown!" << std::endl;
            return 1;
        }


        std::cout << "----- Tests successful. -----" << std::endl << std::endl;
        return 0;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cout << "Exception occurred!" << std::endl;
        return 1;
    }
}
