#ifndef UTOPIA_CORE_TESTTOOLS_EXCEPTIONS_HH
#define UTOPIA_CORE_TESTTOOLS_EXCEPTIONS_HH

#include <string>
#include <functional>
#include <string_view>
#include <type_traits>

#include <boost/test/unit_test.hpp>
#include <boost/core/demangle.hpp>

#include "../exceptions.hh"
#include "utils.hh"


namespace Utopia::TestTools {

/**
 *  \addtogroup TestTools
 *  \{
 */

/// Checks if a callable throws with the expected error type and message
/** An unexpected error type, error message, or lack of throwing an error is
  * reported via BOOST_ERROR.
  *
  * \note   This goes beyond BOOST_CHECK_EXCEPTION by providing more
  *         information on the error message match, which is not easily
  *         possible via the BOOST_CHECK_EXCEPTION predicate function.
  *
  * \tparam ExcT   The expected exception type
  *
  * \param  func   The callable that is expected to throw the exception. It
  *                must take no arguments.
  * \param  match  If given, it will be checked whether the error message
  *                contains this string. This can *not* be a regex or glob
  *                pattern. Uses ::contains to perform the check.
  * \param  loc    Location information. If provided, the BOOST_ERROR will
  *                include line and file information.
  */
template<typename ExcT, typename Callable=std::function<void()>>
void check_exception(Callable&& func,
                     const std::string_view match = "",
                     const LocationInfo loc = {})
{
    // Check if Callable can be invoked
    static_assert(std::is_invocable_v<Callable>,
                  "Callable needs to be invocable without any arguments!");

    // We need to handle the case where ExcT is std::exception separately,
    // because it produces the -Wexceptions compiler warning (which currently
    // cannot be pragma-ignored for gcc).
    if constexpr (std::is_same<ExcT, std::exception>()) {
        try {
            func();
            BOOST_ERROR(loc << "Should have thrown but did not!");
        }
        catch (ExcT& e) {
            if (not match.empty() and not contains(e.what(), match)) {
                BOOST_ERROR(loc << "Did not throw expected error message!\n"
                            "  Expected match :  " << match << "\n"
                            "  But got        :  " << e.what() << "\n");
            }
            // else: everything ok
        }
        catch (...) {
            BOOST_ERROR(loc << "Threw non-std::exception!");
        }
    }
    else {
        // For ExcT not being std::exception, also catch std::exception and
        // supply type information, which can be useful if some other than the
        // expected type was thrown.
        try {
            func();
            BOOST_ERROR(loc << "Should have thrown but did not!");
        }
        catch (ExcT& e) {
            if (not match.empty() and not contains(e.what(), match)) {
                BOOST_ERROR(loc << "Did not throw expected error message!\n"
                            "  Expected match :  " << match << "\n"
                            "  But got        :  " << e.what() << "\n");
            }
            // else: everything ok
        }
        catch (std::exception& e) {
            BOOST_ERROR(loc << "Threw error of unexpected type ("
                        << boost::core::demangle(typeid(e).name())
                        << ") with message: " << e.what());
        }
        catch (...) {
            BOOST_ERROR(loc << "Threw non-std::exception!");
        }
    }
}


// end group TestTools
/**
 *  \}
 */

} // namespace Utopia::TestTools

#endif // UTOPIA_CORE_TESTTOOLS_EXCEPTIONS_HH
