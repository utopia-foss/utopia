#ifndef EXCEPTIONS_HH
#define EXCEPTIONS_HH

namespace Utopia {
/** \page exceptions Custom Exceptions in Utopia
 *
 * \section idea The General Idea
 * With Utopia-specific custom exceptions, the idea is to give the user
 * a well-understandable feedback on the cause of a certain error, and ideally
 * also of how to best resolve it.
 *
 * Currently, the Utopia-specific exceptions have no more abilities than
 * std::runtime_error, but they will be extended.
 *
 * \section impl Implementation
 * Custom exceptions derive from std::runtime_error, which implements all the
 * base methods. The classes can then be expanded with further functionality.
 *
 * All Utopia exceptions should derive from Utopia::exception.
 */

// TODO does it make sense to use Dune::Exception here? Their implementation
// does not derive from std::exception!

/// The Utopia base exception class
class Exception: public std::exception
{
public:
    /** Constructor (C strings).
     *  \param message C-style string error message.
     *                 The string contents are copied upon construction.
     *                 Hence, responsibility for deleting the char* lies
     *                 with the caller. 
     */
    explicit Exception(const char* message):
      _msg(message)
      {}

    /** Constructor (C++ STL strings).
     *  \param message The error message.
     */
    explicit Exception(const std::string& message):
      _msg(message)
      {}

    /** Destructor.
     * Virtual to allow for subclassing.
     */
    virtual ~Exception() throw (){}

    /** Returns a pointer to the (constant) error description.
     *  \return A pointer to a const char*. The underlying memory
     *          is in posession of the Exception object. Callers must
     *          not attempt to free the memory.
     */
    virtual const char* what() const throw (){
       return _msg.c_str();
    }

protected:
    /** Error message.
     */
    std::string _msg;
};

/// For value errors, e.g. invalid configuration parameters
class ValueError : public Exception
{
public:
    ValueError (const std::string& msg) : Exception(msg) {}
};

}; // namespace Utopia

#endif // EXCEPTIONS_HH
