#define BOOST_TEST_MODULE model_test_writemode

// stl
#include <chrono>
#include <numeric>
#include <thread>
#include <yaml-cpp/yaml.h>

// boost test
#include <boost/test/included/unit_test.hpp>

// utopia
#include <utopia/core/type_traits.hh>
#include <utopia/data_io/cfg_utils.hh>
#include <utopia/data_io/data_manager/factory.hh>
#include <utopia/data_io/hdfattribute.hh>
#include <utopia/data_io/hdffile.hh>

// test
#include "../data_io/testtools.hh"
#include "model_test.hh"

template <Utopia::WriteMode write_mode>
struct Fixture
{
    // default the pseudoparent
    Utopia::PseudoParent<Utopia::DefaultRNG> pp =
        Utopia::PseudoParent<Utopia::DefaultRNG>(
            // yaml config
            "model_datamanager_test.yml",
            // outpath
            "model_test_datamanager_" +
                (write_mode == Utopia::WriteMode::basic
                     ? std::string("basic")
                     : std::string("managed")) +
                ".h5",
            // seed
            42,
            // writemode: append if exists else create
            "w");

    Utopia::TestModel<write_mode> model = Utopia::TestModel<write_mode>(
        "test", pp, std::vector<double>(1000, 5), {},
        // task tuple definition
        std::make_tuple(
        // first task
        std::make_tuple(
            "state_writer", // this is for finding name
            [](auto&& model) -> decltype(auto) { return model.state(); },
            [](auto&& thing) -> decltype(auto) { return thing; },
            std::make_tuple("Content_group", "state is contained here"),
            std::make_tuple("Content_dset", "state is contained here once more"))
        // second task
        ,
        std::make_tuple(
            "state_writer_x2", // this is for finding name
            [](auto&& model) -> decltype(auto) { return model.state(); },
            [](auto&& thing) -> decltype(auto) { return thing * 2; },
            Utopia::Utils::Nothing{},
            std::make_tuple("Content_x2", "state times two is contained"))
        )
    );
};

BOOST_FIXTURE_TEST_CASE(model_datamanager_integration,
                        Fixture<Utopia::WriteMode::managed>)
{
    Utopia::setup_loggers();

    auto datamanager = model.get_datamanager();

    // check task names
    auto taskmap = datamanager.get_tasks();

    auto state_writer_found = taskmap.find("state_writer") != taskmap.end();
    auto state_writer_x2_found =
        taskmap.find("state_writer_x2") != taskmap.end();
    BOOST_TEST(state_writer_found);
    BOOST_TEST(state_writer_x2_found);

    // check decider-task-associations
    BOOST_TEST(datamanager.get_decider_task_map()["write_interval_step"] ==
               std::vector<std::string>{"state_writer"});
    //    boost::test_tools::per_element());
    BOOST_TEST(datamanager.get_decider_task_map()["write_interval"] ==
               std::vector<std::string>{"state_writer_x2"});
    //    boost::test_tools::per_element());


    BOOST_TEST(datamanager.get_trigger_task_map()["build_interval_step"] ==
                   std::vector<std::string>{"state_writer"},
               boost::test_tools::per_element());
    BOOST_TEST(datamanager.get_trigger_task_map()["build_once"] ==
                   std::vector<std::string>{"state_writer_x2"},
               boost::test_tools::per_element());

    /// run the model -> writes data
    model.run();

    spdlog::drop("test");

    pp.get_hdffile()->close();
}

// read file in again and check that it is correct
BOOST_AUTO_TEST_CASE(model_datamanager_integration_read)
{
    std::vector<double> expected_data(1000, 5);
    std::vector<double> expected_data_x2;
    expected_data_x2.reserve(1000 * 25);

    Utopia::DataIO::HDFFile file("model_test_datamanager_managed.h5", "r");

    auto group = file.open_group("/test/state_group");

    Utopia::DataIO::HDFAttribute group_attr(*group, "Content_group");
    auto [a_shape, a_data] = group_attr.read<std::string>();
    BOOST_TEST(a_data == "state is contained here");
    BOOST_TEST(a_shape.size() == 1);

    for (std::size_t i = 0; i < 100; i += 10)
    {
        auto dset = group->open_dataset("state_" + std::to_string(i));

        Utopia::DataIO::HDFAttribute dset_attr(*dset, "Content_dset");
        auto [a_shape, a_data] = dset_attr.read<std::string>();
        BOOST_TEST(a_data == "state is contained here once more");
        BOOST_TEST(a_shape.size() == 1);

        auto [shape, data] = dset->read<std::vector<double>>();

        BOOST_TEST(shape == std::vector<hsize_t>{1000},
                   boost::test_tools::per_element());
        BOOST_TEST(data == expected_data, boost::test_tools::per_element());

        std::transform(expected_data.begin(), expected_data.end(),
                       std::back_inserter(expected_data_x2),
                       [](auto&& i) { return i * 2; });

        std::for_each(expected_data.begin(), expected_data.end(),
                      [](auto&& i) { i += 10; });
    }

    auto x2_dset = group->open_dataset("state_x2_50");
    Utopia::DataIO::HDFAttribute dset_attr_x2(*x2_dset, "Content_x2");
    auto [ax2_shape, ax2_data] = dset_attr_x2.read<std::string>();
    BOOST_TEST(ax2_data == "state times two is contained");
    BOOST_TEST(ax2_shape.size() == 1);

}
