#ifndef UTOPIA_CORE_LOGGING_HH
#define UTOPIA_CORE_LOGGING_HH

#include <string>
#include <spdlog/spdlog.h>

namespace Utopia {

/**
 *  \addtogroup Logging
 *  \{
 */

inline const std::string log_data_io = "data_io";
inline const std::string log_core = "core";

/// Initialize a logger with a certain name and log level
/** If the logger already exists, issue a warning and only set the log level.
 *  Optionally, this function can throw an exception.
 *  \param name Name of the logger. This is also the registered logger name.
 *  \param level Log level of the logger. See spdlog::level::level_enum
 *  \param throw_on_exist Throw an exception if the logger exists
 */
inline std::shared_ptr<spdlog::logger> init_logger (
    const std::string name,
    const spdlog::level::level_enum level,
    const bool throw_on_exist = true
)
{
    auto logger = spdlog::get(name);
    
    if (not logger || throw_on_exist) {
        // Create it; spdlog throws an exception if it already exists
        logger = spdlog::stdout_color_mt(name);
    }
    else {
        logger->warn("Skipping initialization of logger '{}' because "
                     "it already exists.", name);
    }
    logger->set_level(level);
    return logger;
};

/// Set up the global loggers and register them.
/** Utopia employs two global loggers, one for the Core backend, 
 *  and one for the Data I/O functions.
 *  They can be retrieved with the spdlog::get(name) function, where 'name'
 *  can be one of the 'log_' strings stored in the Utopia namespace.
 *  
 *  This function only (re)sets the log levels if the loggers already exist.
 */
inline void setup_loggers (
    const spdlog::level::level_enum level_core = spdlog::level::warn,
    const spdlog::level::level_enum level_data_io = spdlog::level::warn
)
{
    // initialize
    init_logger(log_core, level_core, false);
    init_logger(log_data_io, level_data_io, false);
    spdlog::flush_on(spdlog::level::err);

    // set global pattern to
    // "[HH:MM:SS.mmm] [level(colored)] [logger]  <message>"
    spdlog::set_pattern("[%T.%e] [%^%l%$] [%n]  %v");
}

// end group logging
/**
 *  \}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_LOGGING_HH
