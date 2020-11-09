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
#include <utopia/data_io/data_manager/defaults.hh>

#include <utopia/data_io/hdfattribute.hh>
#include <utopia/data_io/hdffile.hh>

// test
#include "../data_io/testtools.hh"
#include "model_test.hh"

#include <utopia/core/types.hh>


// build some custom decider which is later put into the decidermap. This is the
// first part that has to be done
template < typename Model >
struct CustomDecider : Utopia::DataIO::Default::Decider< Model >
{
    double mean = 0.; // this is not necessary strictly, but members are allowed

    virtual bool
    operator()(Model& m) override
    {
        mean = std::accumulate(
                   m.state().begin(),
                   m.state().end(),
                   0.,
                   [](const double& a, const double& b) { return a + b; }) /
               m.state().size();

        return mean < 5.1; // write when mean of state vector is below 5.1
    }

    virtual void set_from_cfg(const Utopia::DataIO::Config&) override 
    {
        // dummy which does nothing
    }

};


BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::vector< double >);
BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::vector< int >);
BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::vector< std::size_t >);
BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::vector< std::string >);
BOOST_TEST_SPECIALIZED_COLLECTION_COMPARE(std::vector< unsigned long long >);

BOOST_AUTO_TEST_CASE(model_datamanager_integration)
{

    Utopia::setup_loggers();

    // build pseudoparent
    Utopia::PseudoParent< Utopia::DefaultRNG > pp(
        // yaml config
        "model_datamanager_test.yml",
        // outpath
        "model_test_datamanager_defaults.h5",
        // seed
        42,
        // writemode: append if exists else create
        "w");

    // build model

    Utopia::TestModel< Utopia::WriteMode::managed > model(
        "test",
        pp,
        std::vector< double >(1000, 5),
        {},
        // task tuple definition
        std::make_tuple(
            // first task
            std::make_tuple(
                "state_writer", // this is for finding name
                [](auto&& model) -> decltype(auto) { return model.state(); },
                [](auto&& thing) -> decltype(auto) { return thing; },
                std::make_tuple("Content_group", "state is contained here"),
                std::make_tuple("Content_dset",
                                "state is contained here once more"))
            // second task
            ,
            std::make_tuple(
                "state_writer_x2", // this is for finding name
                [](auto&& model) -> decltype(auto) { return model.state(); },
                [](auto&& thing) -> decltype(auto) { return thing * 2; },
                Utopia::Utils::Nothing{},
                std::make_tuple("Content_x2",
                                "state times two is contained"))));

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
               std::vector< std::string >{ "state_writer" });
    //    boost::test_tools::per_element());
    BOOST_TEST(datamanager.get_decider_task_map()["write_interval"] ==
               std::vector< std::string >{ "state_writer_x2" });
    //    boost::test_tools::per_element());

    BOOST_TEST(datamanager.get_trigger_task_map()["build_interval_step"] ==
               std::vector< std::string >{ "state_writer" });
    BOOST_TEST(datamanager.get_trigger_task_map()["build_once"] ==
               std::vector< std::string >{ "state_writer_x2" });

    /// run the model -> writes data
    model.run();

}

// read file in again and check that it is correct
BOOST_AUTO_TEST_CASE(model_datamanager_integration_read,
                     * boost::unit_test::depends_on(
                         "model_datamanager_integration"))
{
    std::vector< double > expected_data(1000, 5);
    std::vector< double > expected_data_x2;
    expected_data_x2.reserve(1000 * 25);

    Utopia::DataIO::HDFFile file("model_test_datamanager_defaults.h5", "r");

    auto group = file.open_group("/test/state_group");

    Utopia::DataIO::HDFAttribute group_attr(*group, "Content_group");
    auto [a_shape, a_data] = group_attr.read< std::string >();
    BOOST_TEST(a_data == "state is contained here");
    BOOST_TEST(a_shape.size() == 1);

    for (std::size_t i = 0; i < 100; i += 10)
    {
        auto dset = group->open_dataset("state_" + std::to_string(i));

        Utopia::DataIO::HDFAttribute dset_attr(*dset, "Content_dset");
        auto [a_shape, a_data] = dset_attr.read< std::string >();
        BOOST_TEST(a_data == "state is contained here once more");
        BOOST_TEST(a_shape.size() == 1);

        auto [shape, data] = dset->read< std::vector< double > >();

        BOOST_TEST(shape == std::vector< hsize_t >{ 1000 });
        BOOST_TEST(data == expected_data);

        std::for_each(expected_data.begin(), expected_data.end(), [](auto&& i) {
            i += 10;
        });
    }

    // create expected data for x2 writer
    for (std::size_t i = 0; i < 1000; ++i)
    {
        expected_data_x2.push_back(110);
    }

    for (std::size_t i = 51; i < 75; ++i)
    {
        for (std::size_t j = 0; j < 1000; ++j)
        {
            expected_data_x2.push_back(110 + (i - 50) * 2);
        }
    }

    auto                         x2_dset = group->open_dataset("state_x2_50");
    Utopia::DataIO::HDFAttribute dset_attr_x2(*x2_dset, "Content_x2");

    // read attribute
    auto [ax2_shape, ax2_data] = dset_attr_x2.read< std::string >();
    BOOST_TEST(ax2_data == "state times two is contained");
    BOOST_TEST(ax2_shape.size() == 1);

    // read actual dataset
    auto [shape, data] = x2_dset->read< std::vector< double > >();

    std::vector< unsigned long long > expected_shape{ 25000 };

    BOOST_TEST(shape == expected_shape, boost::test_tools::per_element());

    BOOST_TEST(data == expected_data_x2, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(model_datamanager_integration_custom_decidermap)
{
    spdlog::drop_all();

    Utopia::setup_loggers();

    // build pseudoparent
    Utopia::PseudoParent< Utopia::DefaultRNG > pp(
        // yaml config
        "model_datamanager_test_custom.yml",
        // outpath
        "model_test_datamanager_custom.h5",
        // seed
        42,
        // writemode: append if exists else create
        "w");

    // one can  build a completely new decidermap using the basetypes from
    // "Default"
    auto deciders = Utopia::DataIO::Default::DefaultDecidermap<
        Utopia::TestModel< Utopia::WriteMode::managed > >{

    };

    // or instantiate the default map and extent it
    deciders = Utopia::DataIO::Default::default_deciders<
        Utopia::TestModel< Utopia::WriteMode::managed > >;

    // insert a custom decider
    deciders["average"] = []() -> decltype(auto) {
        return std::make_shared< CustomDecider<
            Utopia::TestModel< Utopia::WriteMode::managed > > >();
    };

    // build model
    Utopia::TestModel< Utopia::WriteMode::managed > model(
        "test",
        pp,
        // for these values, the "average" decider will return true only once
        // at the first timestep
        std::vector< double >(1000, 5),
        {},
        // task tuple definition
        std::make_tuple(
            // first task
            std::make_tuple(
                "state_writer", // this is for finding name
                [](auto&& model) -> decltype(auto) { return model.state(); },
                [](auto&& thing) -> decltype(auto) { return thing; },
                std::make_tuple("Content_group", "state is contained here"),
                std::make_tuple("Content_dset",
                                "state is contained here once more"))
            // second task
            ,
            std::make_tuple(
                "state_writer_x2", // this is for finding name
                [](auto&& model) -> decltype(auto) { return model.state(); },
                [](auto&& thing) -> decltype(auto) { return thing * 2; },
                Utopia::Utils::Nothing{},
                std::make_tuple("Content_x2", "state times two is contained"))),
        deciders);

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
               std::vector< std::string >{ "state_writer" });
    //    boost::test_tools::per_element());
    BOOST_TEST(datamanager.get_decider_task_map()["write_mean"] ==
               std::vector< std::string >{ "state_writer_x2" });
    //    boost::test_tools::per_element());

    BOOST_TEST(datamanager.get_trigger_task_map()["build_interval_step"] ==
                   std::vector< std::string >{ "state_writer" },
               boost::test_tools::per_element());
    BOOST_TEST(datamanager.get_trigger_task_map()["build_once"] ==
                   std::vector< std::string >{ "state_writer_x2" },
               boost::test_tools::per_element());

    /// run the model -> writes data
    model.run();
}

// read file in again and check that it is correct
BOOST_AUTO_TEST_CASE(model_datamanager_integration_read_custom,
                     *boost::unit_test::depends_on(
                        "model_datamanager_integration_custom_decidermap"))
{
    Utopia::DataIO::HDFFile file("model_test_datamanager_custom.h5", "r");

    auto group = file.open_group("/test/state_group");

    Utopia::DataIO::HDFAttribute group_attr(*group, "Content_group");
    auto [a_shape, a_data] = group_attr.read< std::string >();
    BOOST_TEST(a_data == "state is contained here");
    BOOST_TEST(a_shape.size() == 1);

    // check that attribute is correct
    auto                         x2_dset = group->open_dataset("state_x2_0");
    Utopia::DataIO::HDFAttribute dset_attr_x2(*x2_dset, "Content_x2");
    auto [ax2_shape, ax2_data] = dset_attr_x2.read< std::string >();
    BOOST_TEST(ax2_data == "state times two is contained");
    BOOST_TEST(ax2_shape.size() == 1);

    // check that data is correct which has been written with custom decider
    std::vector< double > expected_data_x2(1000, 10.);

    auto [shape, data] = x2_dset->read< std::vector< double > >();
    BOOST_TEST(shape == (std::vector< std::size_t >{ 1000 }),
               boost::test_tools::per_element());
    BOOST_TEST(data == expected_data_x2, boost::test_tools::per_element());

}
