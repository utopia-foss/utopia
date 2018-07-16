#ifndef UTOPIA_CORE_LOGGING_HH
#define UTOPIA_CORE_LOGGING_HH

#include <string>
#include <spdlog/spdlog.h>

namespace Utopia {

/** \defgroup logging Output Logging
 *  Utility functions for all logging output based on the spdlog backend.
 *  (see https://github.com/gabime/spdlog).
 * 
 *  \detail
 *  All output of the simulation backend is intended to be controlled via
 *  loggers. Regular output via the standard pipes `std::cout` and `std::cerr`
 *  is strongly discouraged.
 * 
 *  Utopia generates three types of loggers: One logger for Data I/O functions,
 *  one for Core (backend) functions, and one for each model instance. 
 *  The Utopia::Model base class holds a logger instance which should be used
 *  for information on the current model. To write log messages from within
 *  Data I/O or Core backend functions, the respective logger first has to be
 *  retrieved. This is achieved by using `spdlog::get`
 *  (https://github.com/gabime/spdlog/wiki/2.-Creating-loggers#accessing-loggers-using-spdlogget).
 *  The names for the two loggers are exported within the Utopia namespace.
 *  All log levels are handled through the input configuration files.
 * 
 *  The Utopia::PseudoParent automatically creates the utility loggers. For
 *  executables without Models (like tests), the loggers have to be created
 *  explicitly by manually calling Utopia::setup_loggers.
 * @{
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
    const bool throw_on_exist = true)
{
    auto logger = spdlog::get(name);
    if (not logger || throw_on_exist) {
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
 * @}
 */

} // namespace Utopia

#endif // UTOPIA_CORE_LOGGING_HH