#ifndef UTOPIA_CORE_TESTTOOLS_FIXTURES_HH
#define UTOPIA_CORE_TESTTOOLS_FIXTURES_HH

#include <memory>
#include <string_view>
#include <random>

#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "../types.hh"
#include "../../data_io/cfg_utils.hh"

/**
 *  \addtogroup TestTools
 *  \{
 */

namespace Utopia::TestTools {

/// A base class for an infrastructure fixture
/** If a YAML configuration file is to be loaded, the fixture needs to be
  * derived from and the path to the YAML file needs to be specified
  */
template<class RNGType=Utopia::DefaultRNG>
struct BaseInfrastructure {
    /// The random number generator type to use
    using RNG = RNGType;

    /// Make the config type more easily available
    using Config = DataIO::Config;

    /// The test configuration
    /** \note It will only be loaded if a configuration file path is given
      *       during construction of this base class. To specify a file,
      *       derive from this fixture and pass the path to the base class
      *       constructor.
      */
    const Config cfg;

    /// A logger to use during the test or to pass to entities that need it
    std::shared_ptr<spdlog::logger> log;

    /// The shared random number generator instance, randomly seeded
    std::shared_ptr<RNG> rng;

    /// Construct the BaseInfrastructure fixture
    BaseInfrastructure (const std::string& config_file_path = "")
    :
        // Load a configuration file, if a file path was given
        cfg([&config_file_path](){
            if (config_file_path.size()) {
                return YAML::LoadFile(config_file_path);
            }
            return Config{};
        }()),

        // Set up a test logger
        log([](){
            auto logger = spdlog::get("test");

            // Create it only if it does not already exist
            if (not logger) {
                logger = spdlog::stdout_color_mt("test");
            }

            // Set level and global logging pattern
            logger->set_level(spdlog::level::trace);
            spdlog::set_pattern("[%T.%e] [%^%l%$] [%n]  %v");
            // "[HH:MM:SS.mmm] [level(colored)] [logger]  <message>"

            return logger;
        }()),

        // Set up random number generator (with random seed)
        rng(std::make_shared<RNG>(std::random_device()()))
    {
        log->info("BaseInfrastructure fixture set up.");
        if (config_file_path.size()) {
            log->info("Test configuration loaded from:  {}", config_file_path);
        }
        else {
            log->info("No test configuration file loaded.");
        }
    }

    /// Destruct the BaseInfrastructure fixture, tearing down the test logger
    ~BaseInfrastructure () {
        spdlog::drop("test");
    }
};


} // namespace Utopia::TestTools

// end group TestTools
/**
 *  \}
 */

#endif // UTOPIA_CORE_TESTTOOLS_FIXTURES_HH
