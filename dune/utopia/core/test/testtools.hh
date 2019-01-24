#ifndef UTOPIA_CORE_TEST_TESTTOOLS_HH
#define UTOPIA_CORE_TEST_TESTTOOLS_HH

#include <iostream>

/// Helper method to test whether the expected error type and message is thrown
template<typename err_t>
bool check_error_message(std::string desc,
                         std::function<void()> func,
                         std::string to_find) {
    std::cout << "Checking exceptions for case:  " << desc << std::endl;
    try {
        func();

        std::cerr << "Did not throw!" << std::endl;
        return false;
    }
    catch (err_t& e) {
        if (((std::string) e.what()).find(to_find) == -1) {
            std::cerr << "Did not throw expected error message!" << std::endl;
            std::cerr << "  Expected to find:  " << to_find << std::endl;
            std::cerr << "  But got         :  " << e.what() << std::endl;

            return false;
        }
    }
    catch (...) {
        std::cerr << "Threw error of unexpected type!" << std::endl;
        throw;
    }
    std::cout << "... success."
              << std::endl << std::endl;
    return true;
}

#endif // UTOPIA_CORE_TEST_TESTTOOLS_HH
