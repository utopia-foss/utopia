#ifndef EXCEPTIONS_HH
#define EXCEPTIONS_HH

#include <exception>
#include <iostream>


namespace Utopia {

/// The base exception class to derive Utopia-specific exceptions from
class Exception : public std::runtime_error {
public:
    // -- Custom members (not part of std::exception interface) -- //
    /// The exit code to use when exiting due to this exception
    const int exit_code;

    // -- Constructors -- //
    /// Construct an Utopia-specific exception
    /** @param  what_arg  The std::exception what argument
      * @param  exit_code The code that can (and should) be used in case this
      *                   exceptions leads to exiting of the program.
      */
    template<class what_arg_t>
    Exception(const what_arg_t what_arg, int exit_code = 1)
    :
        // Pass the what argument on to base class constructor
        std::runtime_error(what_arg),
        // Store the exit code
        exit_code(exit_code)
    {}
};


/// An exception for when the program should end due to handling of a signal
/** @detail This exception can only be initialized with a signal number. From
  *         that, a standardized what-argument is generated and the exit code
  *         is calculated according to Unix convention as 128 + abs(signum)
  */
class GotSignal : public Exception {
public:
    GotSignal(const int signum)
    :
        Exception("Received signal: " + std::to_string(signum),
                  128 + abs(signum))
    {}
};


/// A helper function to handle a Utopia-specific exception
/** @param    exc  The exception to handle
  * @returns  int  The exit code from the exception
  */
template<class exc_t>
int handle_exception(exc_t exc) {
    std::cerr << exc.what() << std::endl;
    return exc.exit_code;
}

} // namespace Utopia

#endif // EXCEPTIONS_HH
