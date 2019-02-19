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
  * \param prefix   A prefix string to all std::cout message
  * \param cout_error_msg  Whether to cout the received error message even if
  *                 it was the expected one.
  */
template<typename err_t, typename Callable=std::function<void()>>
bool check_error_message(std::string desc,
                         Callable func,
                         std::string to_find,
                         std::string prefix = "",
                         bool cout_error_msg = false)
{
    std::cout << prefix << "Checking exceptions for case:  " << desc
              << std::endl;
    try {
        func();

        std::cerr << prefix << "Did not throw!" << std::endl;
        return false;
    }
    catch (err_t& e) {
        if (((std::string) e.what()).find(to_find) == std::string::npos) {
            std::cerr << "Did not throw expected error message!" << std::endl;
            std::cerr << "  Expected to find:  " << to_find << std::endl;
            std::cerr << "  But got         :  " << e.what() << std::endl;

            return false;
        }
        // else: found the string pattern in the error message

        if (cout_error_msg) {
            std::cout << prefix << "Received the expected error message:  "
                      << e.what() << std::endl;
        }
    }
    catch (...) {
        std::cerr << prefix << "Threw error of unexpected type!" << std::endl;
        throw;
    }
    std::cout << prefix << "Exception raised as expected."
              << std::endl << std::endl;
    return true;
}

#endif // UTOPIA_CORE_TEST_TESTTOOLS_HH
