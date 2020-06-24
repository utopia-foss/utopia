#ifndef EXCEPTIONS_HH
#define EXCEPTIONS_HH

#include <exception>
#include <iostream>

#include <yaml-cpp/yaml.h>

#include "types.hh"


namespace Utopia {

/// The base exception class to derive Utopia-specific exceptions from
class Exception : public std::runtime_error {
public:
    /// The exit code to use when exiting due to this exception
    const int exit_code;

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
/** @details This exception can only be initialized with a signal number. From
  *         that, a standardized what-argument is generated and the exit code
  *         is calculated according to Unix convention as 128 + abs(signum)
  */
class GotSignal : public Exception {
public:
    /// Construct a GotSignal exception, which has a standardized what message
    /** The constructor takes the number of the signal that triggered this
      * exception and computes an exit code from that according to Unix
      * convention, i.e.: `128 + abs(signum)`
      */
    GotSignal(const int signum)
    :
        Exception("Received signal: " + std::to_string(signum),
                  128 + abs(signum))
    {}
};


/// For access to a dict-like structure with a bad key
class KeyError : public Exception {
public:
    /// Construct a KeyError exception, which has a standardized what message
    KeyError(const std::string& key,
             const DataIO::Config& node,
             const std::string& prefix = "")
    :
        Exception(generate_what_arg(key, node, prefix))
    {}

private:
    /// Generates the what argument for the key error
    std::string generate_what_arg(std::string key,
                                  DataIO::Config node,
                                  std::string prefix)
    {
        std::stringstream msg;

        if (prefix.length()) {
            msg << prefix << std::endl;
        }

        msg << "KeyError: " << key << std::endl;

        if (not node) {
            msg << "The given node is a Zombie! Make sure the node you are "
                << "trying to read from is valid." << std::endl;
        }
        else if (node.size() == 0) {
            msg << "The given node contains no entries! Make sure the desired "
                << "key is available."
                << std::endl;
        }
        else {
            // Emit the node to give more clues at what might have gone wrong
            msg << "Make sure the desired key is available. The content of "
                << "the given node is as follows:" << std::endl
                << YAML::Dump(node) << std::endl;
        }

        return msg.str();
    }
};


/// An exception class for invalid positions in Utopia::Space
class OutOfSpace : public Exception {
public:
    /// Construct the exception with the invalid position and the space given
    template<class VecT, class Space>
    OutOfSpace(const VecT& invalid_pos,
               const std::shared_ptr<Space>& space,
               const std::string prefix = {})
    :
        Exception([&](){
            std::stringstream emsg;
            if (prefix.length() > 0) {
                emsg << prefix << " ";
            }
            emsg << "The given position " << std::endl << invalid_pos
                 << "is not within the non-periodic space with extent"
                 << std::endl << space->extent;
            return emsg.str();
        }())
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
