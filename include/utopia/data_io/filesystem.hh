#ifndef DATAIO_FILESYSTEM_HH
#define DATAIO_FILESYSTEM_HH

#include <cstdlib>
#include <filesystem>
#include <string>

#include "../core/exceptions.hh"
#include "../core/types.hh"
#include "cfg_utils.hh"

namespace Utopia::DataIO {

/*!
 * \addtogroup DataIO
 * \{
 */

/*!
 * \addtogroup Filesystem
 * \{
 */

/**
 * \page Filesystem Filesystem tools
 *
 * The Filesystem module contains tools that make interacting with the
 * filesystem more convenient, e.g. to generate file paths from the
 * configuration.
 */

/// Expands a path with a leading `~` character into an absolute path
/** This function uses the environment variable `HOME` and replaces a leading
  * `~` character with that path.
  * If there was no leading `~` character, the given path is returned.
  *
  * \param   path   The path to expand
  *
  * \throws  If the given path needs expansion but no `HOME` environment
  *          variable was set.
  */
std::string expanduser (const std::string& path) {
    using namespace std::string_literals;

    if (path.empty() or path.substr(0, 1) != "~"s) {
        return path;
    }

    const std::string HOME = std::getenv("HOME");
    if (HOME.empty()) {
        throw std::invalid_argument(
            "Cannot expand path because the environment variable 'HOME' was "
            "not set! Use an absolute path to specify the given path: "
            + path
        );
    }
    return HOME + path.substr(1, std::string::npos);
}

/// Extracts an absolute file path from a configuration
/** Expected keys: `filename`, `base_dir` (optional). If no `base_dir` key is
  * present, will prepend the current working directory.
  *
  * If the base directory or the filename specify a relative directory, the
  * resulting absolute path will start from the current working directory.
  *
  * Furthermore, this function will call expanduser to allow using the `~`
  * character to refer to the home directory.
  *
  * \param  cfg     The configuration node with required key `filename` and
  *                 optional key `base_dir`.
  */
std::string get_abs_filepath (const Config& cfg) {
    using std::filesystem::path;
    using std::filesystem::current_path;

    const path filename = expanduser(get_as<std::string>("filename", cfg));

    path p = current_path();
    if (cfg["base_dir"]) {
        // By using append, we ensure that p remains an absolute path:
        //  - if base_dir is relative, will append, making it absolute
        //  - if base_dir is absolute, will overwrite current_path
        p.append(expanduser(get_as<std::string>("base_dir", cfg)));
    }
    p /= filename;
    return expanduser(p);
}



// end group Filesystem
/**
 *  \}
 */
/*! \} */ // end of group DataIO

} // namespace Utopia::DataIO

#endif // DATAIO_FILESYSTEM_HH
