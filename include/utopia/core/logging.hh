#ifndef UTOPIA_CORE_LOGGING_HH
#define UTOPIA_CORE_LOGGING_HH

#include <string>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "ostream.hh"

namespace Utopia {

/**
 *  \addtogroup Logging
 *  \{
 */

inline const std::string log_core = "core";
inline const std::string log_data_io = "data_io";
inline const std::string log_data_mngr = "data_mngr";

/// Initialize a logger with a certain name and log level
/** If the logger already exists, issue a warning and only set the log level.
 *  Optionally, this function can throw an exception.
 *
 *  \param name           Name of the logger. This is also the registered
 *                        logger name.
 *  \param level          Level of the logger. See spdlog::level::level_enum
 *  \param throw_on_exist Throw an exception if the logger exists
 */
inline std::shared_ptr<spdlog::logger> init_logger (
    const std::string name,
    const spdlog::level::level_enum level,
    const bool throw_on_exist = true
)
{
    auto logger = spdlog::get(name);

    // Create it if it does not exist yet; without a check, spdlog would throw
    if (not logger || throw_on_exist) {
        logger = spdlog::stdout_color_mt(name);
    }

    logger->set_level(level);
    return logger;
};

/// Set up and register the global loggers and set the global log pattern
/** Utopia employs the following global loggers:
 *
 *     * ``core``:      for the Core module
 *     * ``data_io``:   for the Data I/O module in general
 *     * ``data_mngr``: for the DataIO::DataManager
 *
 *  They can be retrieved with the spdlog::get(name) function, where 'name'
 *  can be one of the 'log_' strings stored in the Utopia namespace.
 *
 *  This function only (re)sets the log levels if the loggers already exist.
 *
 * \param  level_core      Log level of the ``core`` logger
 * \param  level_data_io   Log level of the ``data_io`` logger
 * \param  level_data_mngr Log level of the ``data_mngr`` logger
 * \param  log_pattern     The global log pattern. If empty, a pre-defined log
 *                         pattern will be set instead of the spdlog default.
 */
inline void setup_loggers (
    const spdlog::level::level_enum level_core = spdlog::level::warn,
    const spdlog::level::level_enum level_data_io = spdlog::level::warn,
    const spdlog::level::level_enum level_data_mngr = spdlog::level::warn,
    const std::string& log_pattern = ""
)
{
    // initialize
    init_logger(log_core, level_core, false);
    init_logger(log_data_io, level_data_io, false);
    init_logger(log_data_mngr, level_data_mngr, false);
    spdlog::flush_on(spdlog::level::err);

    // set global pattern to
    // "[HH:MM:SS.mmm] [level(colored)] [logger]  <message>"
    if (not log_pattern.empty()) {
        spdlog::set_pattern(log_pattern);
    }
    else {
        spdlog::set_pattern("[%T.%e] [%^%l%$] [%n]  %v");
    }

    spdlog::get("core")->info("Set up loggers: core, data_io, data_mngr.");
}

// end group logging
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_LOGGING_HH
