#ifndef EXCEPTIONS_HH
#define EXCEPTIONS_HH

#include <exception>
#include <iostream>


namespace Utopia {


class Exception : public std::runtime_error {
public:
    // Custom members (not part of std::exception interface)
    const int exit_code;

    // Custom constructor that stores the exit code
    template<class what_arg_t>
    Exception(const what_arg_t what_arg, int exit_code = 1)
    :
        // Pass what argument on to base class constructor
        std::runtime_error(what_arg),
        // Store the exit code
        exit_code(exit_code)
    {}
};


class GotSignal : public Exception {
public:
    GotSignal(const int signum)
    :
        Exception("Received signal: " + std::to_string(signum),
                  128 + abs(signum))
    {}
};

template<class exc_t>
int handle_exception(exc_t exc) {
    std::cerr << exc.what() << std::endl;
    return exc.exit_code;
}

} // namespace Utopia

#endif // EXCEPTIONS_HH
