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
    /** @param  what_arg  The std::runtime_error `what` argument
      * @param  exit_code The code that can (and should) be used in case this
      *                   exceptions leads to exiting of the program. It is
      *                   accessible via the exit_code member.
      */
    explicit Exception(const std::string& what_arg, const int exit_code_arg=1)
    :
        std::runtime_error(what_arg),
        exit_code(exit_code_arg)
    {};

    /// Construct an Utopia-specific exception
    /** @param  what_arg  The std::runtime_error `what` argument
      * @param  exit_code The code that can (and should) be used in case this
      *                   exceptions leads to exiting of the program. It is
      *                   accessible via the exit_code member.
      */
    explicit Exception(const char* what_arg, const int exit_code_arg=1)
    :
        std::runtime_error(what_arg),
        exit_code(exit_code_arg)
    {};
};


/// An exception for when the program should end due to handling of a signal
/** @detail This exception can only be initialized with a signal number. From
  *         that, a standardized what-argument is generated and the exit code
  *         is calculated according to Unix convention as 128 + abs(signum)
  */
class GotSignal : public Exception {
public:
    /// Construct a GotSignal exception, which has a standardized what message
    /** @detail The constructor takes the number of the signal that triggered
      *         this exception and computes an exit code from that according to
      *         Unix convention, i.e.: 128 + abs(signum)
      */
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
int handle_exception(exc_t& exc) {
    std::cerr << exc.what() << std::endl;
    return exc.exit_code;
}

} // namespace Utopia

#endif // EXCEPTIONS_HH
