#ifndef UTOPIA_CORE_TESTTOOLS_UTILS_HH
#define UTOPIA_CORE_TESTTOOLS_UTILS_HH

#include <iostream>
#include <filesystem>
#include <string>
#include <string_view>

#include <spdlog/spdlog.h>  // fmt library included here

/**
 *  \addtogroup TestTools
 *  \{
 */

namespace Utopia::TestTools {

/// Returns true if the ``match`` string is contained within the given string.
bool contains (const std::string_view s, const std::string_view match) {
    return not (s.find(match) == std::string_view::npos);
}


/// Bundles and handles file location information: file path and line number
struct LocationInfo {
    /// Some line, e.g. as provided by __LINE__ macro
    std::size_t line = 0;

    /// Some file path, e.g. as provided by __FILE__ macro
    std::filesystem::path file_path = "";

    /// A fmt library format string
    /** Available keys: ``file_name``, ``file_path``, ``line``
      */
    std::string_view fstr = "@ {file_name:}::{line:d} : ";

    /// Constructs a location object without information
    LocationInfo() = default;

    /// Constructs a location object from line and file path information
    LocationInfo(const std::size_t line, const std::string_view file_path)
        : line(line)
        , file_path(file_path)
    {}

    /// A string representation of the location, using the ``fstr`` member
    /** Will return an empty string if no location was specified.
      */
    std::string string () const {
        if (file_path.empty()) {
            return "";
        }

        using namespace fmt::literals;
        return fmt::vformat(
            fstr,
            fmt::make_format_args(
                "file_path"_a=file_path.string(),
                "file_name"_a=file_path.filename().string(),
                "line"_a=line
            )
        );
    }
};

/// Overload for allowing LocationInfo output streams
std::ostream& operator<< (std::ostream& out, const LocationInfo& loc) {
    out << loc.string();
    return out;
}

} // namespace Utopia::TestTools

// end group TestTools
/**
 *  \}
 */

#endif // UTOPIA_CORE_TESTTOOLS_UTILS_HH
