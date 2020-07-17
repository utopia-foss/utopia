#ifndef PARALLEL_FIXTURES_HH
#define PARALLEL_FIXTURES_HH

#include <vector>

#include <utopia/core/logging.hh>
#include <utopia/core/parallel.hh>

// +++ Fixtures +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ //

struct logger_setup
{
    logger_setup() { Utopia::setup_loggers(); }
};

struct parallel_setup : public logger_setup
{
    parallel_setup()
    {
        Utopia::ParallelExecution::set(
            Utopia::ParallelExecution::Setting::enabled);
    }
    ~parallel_setup()
    {
        Utopia::ParallelExecution::set(
            Utopia::ParallelExecution::Setting::disabled);
    }
};

struct vectors : public parallel_setup
{
    std::vector<double> from = std::vector<double>(1E6, 0.0);
    std::vector<double> to = std::vector<double>(1E6, 1.0);
};

#endif // PARALLEL_FIXTURES_HH
