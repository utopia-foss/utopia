#ifndef UTOPIA_CORE_TEST_TESTTOOLS_HH
#define UTOPIA_CORE_TEST_TESTTOOLS_HH

#include <iostream>

/// Helper method to test whether the expected error type and message is thrown
/** \tparam err_t   The expected error type, has to supply .what() method
  *
  * \param desc     A describing string that is emitted before the function is
  *                 called
  * \param func     The callable whose exception handling is to be checked
  * \param to_find  The string that is to be matched (via std::string::find)
  *                 in the produced error message
  */
template<typename err_t, typename Callable=std::function<void()>>
bool check_error_message(std::string desc,
                         Callable func,
                         std::string to_find)
{
    std::cout << "Checking exceptions for case:  " << desc << std::endl;
    try {
        func();

        std::cerr << "Did not throw!" << std::endl;
        return false;
    }
    catch (err_t& e) {
        if (((std::string) e.what()).find(to_find) < 0) {
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
    std::cout << "Exception raised as expected." << std::endl << std::endl;
    return true;
}

#endif // UTOPIA_CORE_TEST_TESTTOOLS_HH
