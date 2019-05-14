#ifndef UTOPIA_DATAIO_TEST_HH
#define UTOPIA_DATAIO_TEST_HH

#include <sstream> // needed for testing throw messages from exceptions

#include <utopia/data_io/hdfdataset.hh> // for messing around with datasets
#include <utopia/data_io/hdffile.hh>    // for buiding model mock class

namespace Utopia {
namespace DataIO {

/**
 * @brief Mocking class for the model
 *
 */
struct Model {
    std::string name;
    Utopia::DataIO::HDFFile file;
    std::shared_ptr<spdlog::logger> logger;
    std::vector<int> x;
    std::size_t time;
    
    auto get_logger() {
        return logger;
    }

    std::size_t get_time() {
        return time;
    }

    auto get_name() {
        return name;
    }

    auto get_hdfgrp() {
        return file.get_basegroup();
    }

    Model(std::string n)
        : name(n),
          file(n + ".h5", "w"),
          logger(spdlog::stdout_color_mt("logger." + n)),
          // mock data
          x([]() {
              std::vector<int> v(100);
              std::iota(v.begin(), v.end(), 1);
              return v;
          }()),
          time(0)
    {
    }

    Model() = default;
    Model(const Model&) = default;
    Model(Model&&) = default;
    Model& operator=(Model&&) = default;
    Model& operator=(const Model&) = default;
    ~Model()
    {
        file.close();
    }
};

/**
 * @brief Mocking class for tasks
 *
 */
template <typename B, typename W>
struct Task
{
    using Builder = B;
    using Writer = W;
    Builder build_dataset;
    Writer write;
    Utopia::DataIO::HDFGroup group;

    Task(Builder b, Writer w, Utopia::DataIO::HDFGroup g)
        : build_dataset(b), write(w), group(g)
    {
    }
    Task() = default;
    Task(const Task&) = default;
    Task(Task&&) = default;
    Task& operator=(Task&&) = default;
    Task& operator=(const Task&) = default;
    virtual ~Task() = default;
};

/**
 * @brief Mocking class for tasks ,basic. Needed for testing polymorphism
 *
 */
struct BasicTask
{
    std::string str;

    virtual void write()
    {
        str = "base";
    }

    BasicTask() = default;
    BasicTask(const BasicTask&) = default;
    BasicTask(BasicTask&&) = default;
    BasicTask& operator=(const BasicTask&) = default;
    BasicTask& operator=(BasicTask&&) = default;
    virtual ~BasicTask() = default;
};

/**
 * @brief Mocking class for a task which derives from BasicTask
 *
 */
struct DerivedTask : public BasicTask
{
    using Base = BasicTask;

    virtual void write() override
    {
        this->str = "derived";
    }

    DerivedTask() = default;
    DerivedTask(const DerivedTask&) = default;
    DerivedTask(DerivedTask&&) = default;
    DerivedTask& operator=(DerivedTask&&) = default;
    DerivedTask& operator=(const DerivedTask&) = default;
    virtual ~DerivedTask() = default;
};

}
} // namespace Utopia
#endif
