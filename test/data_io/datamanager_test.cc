#define BOOST_TEST_MODULE datamanager_test

#include <sstream> // needed for testing throw messages from exceptions
#include <thread>

#include <boost/mpl/list.hpp>                // type lists for testing
#include <boost/test/included/unit_test.hpp> // for unit tests

#include <utopia/core/logging.hh>
#include <utopia/core/utils.hh> // for output operators, get size...

#include <utopia/data_io/hdfdataset.hh> // for messing around with datasets
#include <utopia/data_io/hdffile.hh>    // for buiding model mock class

#include <utopia/data_io/data_manager/data_manager.hh>
#include <utopia/data_io/data_manager/defaults.hh>
#include <utopia/data_io/data_manager/write_task.hh>

#include "testtools.hh"

namespace utf = boost::unit_test;
using namespace std::literals;
using namespace Utopia::DataIO;

// less messy type aliases
using Writer = std::function< void(
    Model&, Utopia::DataIO::HDFDataset&) >;
using Builder =
    std::function< Utopia::DataIO::HDFDataset(
        Model&, Utopia::DataIO::HDFGroup&) >;

using Simpletask = Task< Builder, Writer >;


using Decider = std::function<bool(Model&)>;
using Trigger = Decider;

struct Fix
{
    void
    setup()
    {
        Utopia::setup_loggers();
        spdlog::get("data_mngr")->set_level(spdlog::level::debug);
    }
};

BOOST_AUTO_TEST_SUITE(Suite,
                      *boost::unit_test::fixture<Fix>())


BOOST_AUTO_TEST_CASE(datamanager_basic_constructor)
{

    using namespace Utopia::Utils; // enable output operators defined in



    // needed for all the structors
    Model model("fixture_1");

    // build Traits
    using DMT = DataManagerTraits< Simpletask,
                                   std::function< bool(Model&) >, // decider
                                   std::function< bool(Model&) >, // trigger,
                                   Default::DefaultExecutionProcess >;

    DataManager< DMT > dm(
        // tasks
        { { "t1"s,
            std::make_shared< Simpletask >(
                [](Model& m, Utopia::DataIO::HDFGroup& g)
                    -> Utopia::DataIO::HDFDataset {
                    return *g.open_dataset("/" + m.name + "_1");
                },
                [](Model&,
                   Utopia::DataIO::HDFDataset& d)
                    -> void {
                    d.write(std::vector< int >{ 1, 2, 3 });
                },
                *model.file.open_group("/t1")) },
          { "t2"s,
            std::make_shared< Simpletask >(
                [](Model& m, Utopia::DataIO::HDFGroup& g)
                    -> Utopia::DataIO::HDFDataset {
                    return *g.open_dataset("/" + m.name + "_2");
                },
                [](Model&,
                   Utopia::DataIO::HDFDataset& d)
                    -> void {
                    d.write(std::vector< int >{ 4, 5, 6 });
                },
                *model.file.open_group("/t2")) } },
        // deciders
        { { "d1"s,
            std::make_shared< Decider >([](Model&) -> bool { return true; }) },
          { "d2"s, std::make_shared< Decider >([](Model&) -> bool {
                return false;
            }) } },
        // triggers
        { { "b1"s,
            std::make_shared< Trigger >([](Model&) -> bool { return true; }) },
          { "b2"s, std::make_shared< Trigger >([](Model&) -> bool {
                return false;
            }) } },
        Default::DefaultExecutionProcess());

    // test correct associations
    BOOST_TEST(dm.get_decider_task_map() ==
               (std::unordered_map< std::string, std::vector< std::string > >{
                   { "d1", std::vector< std::string >{ "t1" } },
                   { "d2", std::vector< std::string >{ "t2" } } }));

    BOOST_TEST(dm.get_trigger_task_map() ==
               (std::unordered_map< std::string, std::vector< std::string > >{
                   { "b1", std::vector< std::string >{ "t1" } },
                   { "b2", std::vector< std::string >{ "t2" } } }));

    // try to build a datamanager without explicit associations but with
    // unequal length arrays
    // fails with an exception complaining about unequal sizes and
    // ambiguous associations

    // use ostream to get exception message
    std::ostringstream s;

    try
    {
        DataManager< DMT > dm2(
            // tasks
            { { "t1_2"s,
                std::make_shared< Simpletask >(
                    [](Model& m, Utopia::DataIO::HDFGroup& g)
                        -> Utopia::DataIO::HDFDataset {
                        return *g.open_dataset("/" + m.name + "_1_2");
                    },
                    [](Model&,
                       Utopia::DataIO::HDFDataset&
                           d) -> void {
                        d.write(std::vector< int >{ 1, 2, 3 });
                    },
                    *model.file.open_group("/t1_2")) } },
            // deciders
            { { "d1_2"s, std::make_shared< Decider >([](Model&) -> bool {
                    return true;
                }) } },
            // triggers -> this will cause an error
            { { "b1_2"s, std::make_shared< Trigger >([](Model&) -> bool {
                    return true;
                }) },
              { "b2_2"s, std::make_shared< Trigger >([](Model&) -> bool {
                    return false;
                }) } },
            Default::DefaultExecutionProcess());
    }
    catch (std::exception& e)
    {
        s << e.what();
    }

    // check that the error message is correct.
    BOOST_TEST(
        s.str() ==
        std::string(
            "Error, explicit associations have to be given when mapping "
            "unequal numbers of decider or trigger functions and tasks."));

    // build a datamanager with explicit associations.
    // Additionally mix arrays and tuples
    DataManager< DMT > dm3(
        // tasks
        { { "t1_2"s,
            std::make_shared< Simpletask >(
                [](Model& m, Utopia::DataIO::HDFGroup& g)
                    -> Utopia::DataIO::HDFDataset {
                    return *g.open_dataset("/" + m.name + "_1_2");
                },
                [](Model&,
                   Utopia::DataIO::HDFDataset& d)
                    -> void {
                    d.write(std::vector< int >{ 1, 2, 3 });
                },
                *model.file.open_group("/t1_3")) },
          { "t2_2"s,
            std::make_shared< Simpletask >(
                [](Model& m, Utopia::DataIO::HDFGroup& g)
                    -> Utopia::DataIO::HDFDataset {
                    return *g.open_dataset("/" + m.name + "_2_2");
                },
                [](Model&,
                   Utopia::DataIO::HDFDataset& d)
                    -> void {
                    d.write(std::vector< int >{ 4, 5, 6 });
                },
                *model.file.open_group("/t2_3")) },
          { "t3_2"s,
            std::make_shared< Simpletask >(
                [](Model& m, Utopia::DataIO::HDFGroup& g)
                    -> Utopia::DataIO::HDFDataset {
                    return *g.open_dataset("/" + m.name + "_2_2");
                },
                [](Model&,
                   Utopia::DataIO::HDFDataset& d)
                    -> void {
                    d.write(std::vector< int >{ 4, 5, 6 });
                },
                *model.file.open_group("/t3")) } },
        // deciders
        { { "d1_3"s, std::make_shared< Decider >([](Model&) -> bool {
                return true;
            }) } },
        // triggers
        { { "b1_3"s,
            std::make_shared< Trigger >([](Model&) -> bool { return true; }) },
          { "b2_3"s, std::make_shared< Trigger >([](Model&) -> bool {
                return false;
            }) } },
        // execution process
        Default::DefaultExecutionProcess(),
        // task -> decider associations
        { { "t1_3", "d1_3" }, { "t2_3", "d1_3" }, { "t3_3", "d1_3" } },
        // task -> trigger associations
        { { "t1_3", "b1_3" }, { "t2_3", "b1_3" }, { "t3_3", "b2_3" } });

    BOOST_TEST(dm3.get_decider_task_map()["d1_3"] ==
                   (std::vector< std::string >{ "t1_3", "t2_3", "t3_3" }),
               boost::test_tools::per_element());
    BOOST_TEST(dm3.get_trigger_task_map()["b1_3"] ==
                   (std::vector< std::string >{ "t1_3", "t2_3" }),
               boost::test_tools::per_element());

    BOOST_TEST(dm3.get_trigger_task_map()["b2_3"] ==
                   (std::vector< std::string >{ "t3_3" }),
               boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(datamanager_lifecycle)
{

    // needed for all the structors
    Model model("fixture_3");

    using Decider = std::function< bool(Model&) >;
    using Trigger = std::function< bool(Model&) >;
    using DMT     = DataManagerTraits< Simpletask,
                                   Decider,
                                   Trigger,
                                   Default::DefaultExecutionProcess >;
    using DM      = DataManager< DMT >;

    // datamanager to use for testing copy, move etc
    DM dm(
        // tasks
        { { "v1"s,
            std::make_shared< Simpletask >(
                [](Model& m, Utopia::DataIO::HDFGroup& g)
                    -> Utopia::DataIO::HDFDataset {
                    return *g.open_dataset("/" + m.name + "_1");
                },
                [](Model&,
                   Utopia::DataIO::HDFDataset& d)
                    -> void {
                    d.write(std::vector< int >{ 1, 2, 3 });
                },
                *model.file.open_group("/t1")) },
          { "v2"s,
            std::make_shared< Simpletask >(
                [](Model& m, Utopia::DataIO::HDFGroup& g)
                    -> Utopia::DataIO::HDFDataset {
                    return *g.open_dataset("/" + m.name + "_2");
                },
                [](Model&,
                   Utopia::DataIO::HDFDataset& d)
                    -> void {
                    d.write(std::vector< int >{ 4, 5, 6 });
                },
                *model.file.open_group("/t2")) } },
        // deciders
        { { "w1"s,
            std::make_shared< Decider >([](Model&) -> bool { return true; }) },
          { "w2"s, std::make_shared< Decider >([](Model&) -> bool {
                return false;
            }) } },
        // triggers
        { { "k1"s,
            std::make_shared< Trigger >([](Model&) -> bool { return true; }) },
          { "k2"s, std::make_shared< Trigger >([](Model&) -> bool {
                return false;
            }) } },
        Default::DefaultExecutionProcess());

    // have copy to check against later
    auto dm_cpy(dm);

    // datamanager to use for testing copy, move etc
    DM dm2(
        // tasks
        { { "t1"s,
            std::make_shared< Simpletask >(
                [](Model& m, Utopia::DataIO::HDFGroup& g)
                    -> Utopia::DataIO::HDFDataset {
                    return *g.open_dataset("/" + m.name + "_1");
                },
                [](Model&,
                   Utopia::DataIO::HDFDataset& d)
                    -> void {
                    d.write(std::vector< int >{ 1, 2, 3 });
                },
                *model.file.open_group("/t1")) } },
        // deciders
        { { "d1"s, std::make_shared< Decider >([](Model&) -> bool {
                return true;
            }) } },
        // triggers
        { { "b1"s, std::make_shared< Trigger >([](Model&) -> bool {
                return true;
            }) } },
        Default::DefaultExecutionProcess());

    // have copy to check against
    auto dm2_cpy(dm2);

    swap(dm2, dm);

    // check that the states are swapped
    BOOST_TEST(dm.get_triggers() == dm2_cpy.get_triggers());
    BOOST_TEST(dm.get_tasks() == dm2_cpy.get_tasks());
    BOOST_TEST(dm.get_deciders() == dm2_cpy.get_deciders());
    BOOST_TEST(dm.get_logger() == dm2_cpy.get_logger());
    BOOST_TEST(dm.get_trigger_task_map() == dm2_cpy.get_trigger_task_map());
    BOOST_TEST(dm.get_decider_task_map() == dm2_cpy.get_decider_task_map());

    BOOST_TEST(dm2.get_triggers() == dm_cpy.get_triggers());
    BOOST_TEST(dm2.get_tasks() == dm_cpy.get_tasks());
    BOOST_TEST(dm2.get_deciders() == dm_cpy.get_deciders());
    BOOST_TEST(dm2.get_logger() == dm_cpy.get_logger());
    BOOST_TEST(dm2.get_trigger_task_map() == dm_cpy.get_trigger_task_map());
    BOOST_TEST(dm2.get_decider_task_map() == dm_cpy.get_decider_task_map());
}

// test polymorphism for tasks in datamanager
// The principle is the same for deciders and triggers, and since they
// are stored in the same way in the same datastructures,
// there polymorphism capabilities are not additionally tested.
BOOST_AUTO_TEST_CASE(datamanager_polymorphism)
{
    Model model("fixture_4");
    using Decider = std::function< bool(Model&) >;
    using Trigger = std::function< bool(Model&) >;
    using DMT     = DataManagerTraits< BasicTask,
                                   Decider,
                                   Trigger,
                                   Default::DefaultExecutionProcess >;
    using DM      = DataManager< DMT >;
    // datamanager to use for testing copy, move etc
    DM dm(
        // tasks
        { { "basic"s, std::make_shared< BasicTask >(BasicTask()) },
          { "derived"s, std::make_shared< DerivedTask >(DerivedTask()) } },
        // deciders
        { { "w1"s,
            std::make_shared< Decider >([](Model&) -> bool { return true; }) },
          { "w2"s, std::make_shared< Decider >([](Model&) -> bool {
                return false;
            }) } },
        // triggers
        { { "k1"s,
            std::make_shared< Trigger >([](Model&) -> bool { return true; }) },
          { "k2"s, std::make_shared< Trigger >([](Model&) -> bool {
                return false;
            }) } },
        Default::DefaultExecutionProcess());

    // execute tasks
    auto tsks = dm.get_tasks();
    for (auto& pair : tsks)
    {
        pair.second->write();
    }

    // check that the respective 'write' operations where called
    // correctly for
    // the registered tasks
    BOOST_TEST(tsks["basic"]->str == "base");
    BOOST_TEST(tsks["derived"]->str == "derived");
}

BOOST_AUTO_TEST_CASE(datamanager_call_operator_default)
{


    using BGB = std::function< std::shared_ptr< HDFGroup >(
        std::shared_ptr< HDFGroup > &&) >;
    using W   = std::function< void(std::shared_ptr< HDFDataset >&,
                                  Model&) >;
    using B   = std::function< std::shared_ptr< HDFDataset >(
        std::shared_ptr< HDFGroup >&, Model&) >;
    using AWG = std::function< void(std::shared_ptr< HDFGroup >&, Model&) >;
    using AWD = std::function< void(std::shared_ptr< HDFDataset >&,
                                    Model&) >;

    using Task = Utopia::DataIO::WriteTask< BGB, W, B, AWG, AWD >;

    using DMT = DataManagerTraits< Task,
                                   Decider,
                                   Trigger,
                                   Default::DefaultExecutionProcess >;

    Model model("fixture_6");

    // build tasks here to check if they work
    Task t1(
        // basegroup builder
        [](std::shared_ptr< HDFGroup >&& bgrp) -> std::shared_ptr< HDFGroup > {
            return bgrp->open_group("datagroup/1");
        },
        // writer function
        [](auto& dataset, Model& m) { dataset->write(m.x); },
        // builder function
        [](auto& group, Model& m) {
            return group->open_dataset("testgroup/initial_dataset1_" + m.name);
        },
        // attribute writer
        [](auto& hdfgroup, Model& m) {
            hdfgroup->add_attribute(
                "dimension names for " + m.name,
                std::vector< std::string >{ "X", "Y", "Z" });
        },
        [](auto& hdfdataset, Model& m) {
            hdfdataset->add_attribute("cell_data",
                                      std::vector< std::string >{
                                          "resources", "traitlength", m.name });
        });

    Task t2(
        // basegroup builder
        [](std::shared_ptr< HDFGroup >&& bgrp) -> std::shared_ptr< HDFGroup > {
            return bgrp->open_group("datagroup2/2");
        },
        // writer function
        [](auto& dataset, Model& m) {
            std::vector< int > v;
            std::transform(m.x.begin(),
                           m.x.end(),
                           std::back_inserter(v),
                           [](int i) { return 2 * i + 1; });
            dataset->write(v);
        },
        // builder function
        [](auto& group, Model& m) {
            return group->open_dataset("testgroup/initial_dataset2_" + m.name);
        },
        // attribute writer
        [](auto& hdfgroup, Model& m) {
            hdfgroup->add_attribute(
                "dimension names for " + m.name,
                std::vector< std::string >{ "X", "Y", "Z" });
        },
        [](auto& hdfdataset, Model& m) {
            hdfdataset->add_attribute("cell_data",
                                      std::vector< std::string >{ "resources"s,
                                                                  "traitlength",
                                                                  m.name });
        });

    // build fucking datamanager from it
    Utopia::DataIO::DataManager< DMT > dm(
        // tasks
        { { "wt1"s, std::make_shared< Task >(t1) },
          { "wt2"s, std::make_shared< Task >(t2) } },
        // deciders
        { { "w1"s,
            std::make_shared< Decider >([](Model&) -> bool { return true; }) },
          { "w2"s, std::make_shared< Decider >([](Model&) -> bool {
                return false;
            }) } },
        // triggers
        { { "k1"s,
            std::make_shared< Trigger >([](Model&) -> bool { return true; }) },
          { "k2"s, std::make_shared< Trigger >([](Model&) -> bool {
                return false;
            }) } },
        Default::DefaultExecutionProcess());

    // call datamanager
    dm(model);

    for (auto& taskpair : dm.get_tasks())
    {
        BOOST_TEST(taskpair.second->base_group != nullptr);
    }

    // check that the dataset is a valid path in the file
    BOOST_REQUIRE(
        path_exists(model.file.get_basegroup()->get_C_id(), "/datagroup/1"));

    BOOST_REQUIRE(path_exists(dm.get_tasks()["wt1"]->base_group->get_C_id(),
                              "testgroup/initial_dataset1_fixture_6") > 0);

    BOOST_REQUIRE(
        path_exists(model.file.get_basegroup()->get_C_id(), "/datagroup2/2"));

    // second writer/builder is never active here, hence not present
}

// test that the datamanager has written the data correctly
// this shows that the default writer works correctly
BOOST_AUTO_TEST_CASE(default_datamananger_written_data_check)
{
    HDFFile file("fixture_6.h5", "r");
    auto    group    = file.open_group("/datagroup/1/testgroup");
    auto    dataset1 = group->open_dataset("initial_dataset1_fixture_6");

    // expected dataset data
    std::vector< int > expected(100);
    std::iota(expected.begin(), expected.end(), 1);

    auto [shape, data] = dataset1->read< std::vector< int > >();
    BOOST_TEST(shape == std::vector< hsize_t >{ 100 });
    BOOST_TEST(data == expected, boost::test_tools::per_element());

    HDFAttribute attr_dset_1(*dataset1, "cell_data");

    HDFAttribute attr_group_1(*file.open_group("/datagroup/1"),
                              "dimension names for fixture_6");
    HDFAttribute attr_group_2(*file.open_group("/datagroup2/2"),
                              "dimension names for fixture_6");

    // expected attribute data
    auto attr_dset_expected =
        std::vector< std::string >{ "resources", "traitlength", "fixture_6" };

    auto attr_group_expected = std::vector< std::string >{ "X", "Y", "Z" };

    auto [attrshape, attrdata] =
        attr_dset_1.read< std::vector< std::string > >();
    BOOST_TEST(attrshape == std::vector< hsize_t >{ 3 });
    BOOST_TEST(attrdata == attr_dset_expected,
               boost::test_tools::per_element());

    auto [attrshape_group1, attrdata_group1] =
        attr_group_1.read< std::vector< std::string > >();
    BOOST_TEST(attrshape_group1 == std::vector< hsize_t >{ 3 });
    BOOST_TEST(attrdata_group1 == attr_group_expected,
               boost::test_tools::per_element());

    auto [attrshape_group2, attrdata_group2] =
        attr_group_1.read< std::vector< std::string > >();
    BOOST_TEST(attrshape_group2 == std::vector< hsize_t >{ 3 });
    BOOST_TEST(attrdata_group2 == attr_group_expected,
               boost::test_tools::per_element());
}

// Test for custom execution process and decider, trigger, task types
// Mind the small differences in the names of the groups and datasets
// which are used here when changing this!
BOOST_AUTO_TEST_CASE(datamanager_call_operator_custom)
{


    using BGB = std::function< std::shared_ptr< HDFGroup >(Model&) >;
    using W   = std::function< void(std::shared_ptr< HDFDataset >&,
                                  Model&,
                                  double additionvalue) >;
    using B   = std::function< std::shared_ptr< HDFDataset >(
        std::shared_ptr< HDFGroup >&, Model&) >;
    using AWG = std::function< void(
        std::shared_ptr< HDFGroup >&, Model&, std::string, std::string) >;
    using AWD = std::function< void(std::shared_ptr< HDFDataset >&,
                                    Model&,
                                    std::string,
                                    std::string) >;

    using Task = Utopia::DataIO::WriteTask< BGB, W, B, AWG, AWD >;

    Model model("fixture_7");

    // build tasks here to check if they work
    Task t1(
        // make group builder
        [](Model& m) -> std::shared_ptr< HDFGroup > {
            return m.file.open_group("datagroup/task_1");
        },
        // writer function
        [](auto& dataset, Model& m, double add) {
            std::vector< int > d;
            std::transform(
                m.x.begin(), m.x.end(), std::back_inserter(d), [&add](int v) {
                    return static_cast< double >(v) + add;
                });

            dataset->write(d);
        },
        // builder function
        [](auto& group, Model& m) {
            return group->open_dataset("testgroup/initial_dataset_of_task_1_" +
                                       m.name + "_" + std::to_string(m.time));
        },
        // attribute writer group
        [](auto& hdfgroup, Model& m, std::string name, std::string postfix) {
            hdfgroup->add_attribute(
                name + " " + m.name,
                std::vector< std::string >{
                    "X_" + postfix, "Y_" + postfix, "Z_" + postfix });
        },
        // attribute writer dataset
        [](auto& hdfdataset, Model& m, std::string name, std::string postfix) {
            hdfdataset->add_attribute(
                name + " " + m.name,
                std::vector< std::string >{
                    "resources_" + postfix, "traitlength_" + postfix, m.name });
        });

    Task t2(
        // basegroup builder
        [](Model& m) -> std::shared_ptr< HDFGroup > {
            return m.file.open_group("datagroup2/2");
        },
        // writer function
        [](auto& dataset, Model& m, double add) {
            std::vector< double > v;
            std::transform(m.x.begin(),
                           m.x.end(),
                           std::back_inserter(v),
                           [&add](int i) { return 2. * i + 1 + add; });
            dataset->write(v);
        },
        // builder function
        [](auto& group, Model& m) {
            return group->open_dataset("testgroup/initial_dataset2_" + m.name +
                                       "_" + std::to_string(m.time));
        },
        // attribute writer dataset
        [](auto& hdfgroup, Model& m, std::string name, std::string postfix) {
            hdfgroup->add_attribute(
                name + " " + m.name,
                std::vector< std::string >{
                    "X2_"s + postfix, "Y2_"s + postfix, "Z2_"s + postfix });
        },
        // attribute writer group
        [](auto& hdfdataset, Model& m, std::string name, std::string postfix) {
            hdfdataset->add_attribute(
                name,
                std::vector< std::string >{ "resources_"s + postfix,
                                            "traitlength_"s + postfix,
                                            m.name });
        });

    // make custom execution process here, honoring the types defined
    // above.
    auto custom_exec = [](auto& datamanager, auto& model) {
        // trigger and dataset builder phase
        auto& tasks = datamanager.get_tasks();

        for (auto& taskpair : tasks)
        {
            if (taskpair.second->base_group == nullptr)
            {
                taskpair.second->base_group =
                    taskpair.second->build_basegroup(model);
            }
        }

        for (auto& [name, trigger] : datamanager.get_triggers())
        {
            if ((*trigger)(model))
            {
                for (auto& tname : datamanager.get_trigger_task_map().at(name))
                {

                    tasks.at(tname)->active_dataset =
                        tasks.at(tname)->build_dataset(
                            tasks.at(tname)->base_group, model);

                    tasks.at(tname)->write_attribute_basegroup(
                        tasks.at(tname)->base_group,
                        model,
                        "stored_pseudo_variables",
                        "_[-]");
                }
            }
        }

        for (auto& [name, decider] : datamanager.get_deciders())
        {
            if ((*decider)(model))
            {
                for (auto& tname : datamanager.get_decider_task_map().at(name))
                {
                    tasks.at(tname)->write_data(
                        tasks.at(tname)->active_dataset, model, 4);
                    tasks.at(tname)->write_attribute_active_dataset(
                        tasks.at(tname)->active_dataset,
                        model,
                        "pseudo_attribute",
                        "something");
                }
            }
        };
    };

    using DMT =
        DataManagerTraits< Task, Decider, Trigger, decltype(custom_exec) >;

    // define custom exec process
    Utopia::DataIO::DataManager< DMT > dm(
        // tasks
        { { "wt1"s, std::make_shared< Task >(t1) },
          { "wt2"s, std::make_shared< Task >(t2) } },
        // deciders
        { { "w1"s,
            std::make_shared< Decider >([](Model&) -> bool { return true; }) },
          { "w2"s, std::make_shared< Decider >([](Model&) -> bool {
                return true;
            }) } },
        // triggers
        { { "k1"s,
            std::make_shared< Trigger >([](Model&) -> bool { return true; }) },
          { "k2"s, std::make_shared< Trigger >([](Model& m) -> bool {
                return m.time % 2 == 0;
            }) } },
        // custom execution process
        custom_exec);

    // write data
    dm(model);

    // increment and write a secondtime
    ++model.time;
    dm(model);

    // check that the dataset is a valid path in the file
    BOOST_REQUIRE(
        path_exists(model.file.get_basegroup()->get_C_id(), "/datagroup/task_1"));
    BOOST_REQUIRE(
        path_exists(model.file.get_basegroup()->get_C_id(), "/datagroup2/2"));

    BOOST_REQUIRE(
        path_exists(dm.get_tasks()["wt1"]->base_group->get_C_id(),
                    "testgroup/initial_dataset_of_task_1_fixture_7_0") > 0);
    BOOST_REQUIRE(
        path_exists(dm.get_tasks()["wt1"]->base_group->get_C_id(),
                    "testgroup/initial_dataset_of_task_1_fixture_7_1") > 0);
    BOOST_REQUIRE(path_exists(dm.get_tasks()["wt2"]->base_group->get_C_id(),
                              "testgroup/initial_dataset2_fixture_7_0") > 0);
}

// check that the custom datamanager has written its data correctly
BOOST_AUTO_TEST_CASE(custom_datamananger_written_data_check)
{
    HDFFile file("fixture_7.h5", "r");
    // groups
    auto group1 = file.open_group("datagroup/task_1/testgroup/");
    auto group2 = file.open_group("datagroup2/2/testgroup");

    auto basegroupgroup1 = file.open_group("datagroup/task_1");
    auto basegroupgroup2 = file.open_group("datagroup2/2");

    // datasets

    auto dataset1_0 =
        group1->open_dataset("initial_dataset_of_task_1_fixture_7_0");
    auto dataset1_1 =
        group1->open_dataset("initial_dataset_of_task_1_fixture_7_1");

    auto dataset2_0 = group2->open_dataset("initial_dataset2_fixture_7_0");

    // attributes
    HDFAttribute attr_dset_1_0(*dataset1_0, "pseudo_attribute fixture_7");
    HDFAttribute attr_dset_1_1(*dataset1_1, "pseudo_attribute fixture_7");
    HDFAttribute attr_dset_2_0(*dataset2_0, "pseudo_attribute");

    HDFAttribute attr_group_1(*basegroupgroup1,
                              "stored_pseudo_variables fixture_7");
    HDFAttribute attr_group_2(*basegroupgroup2,
                              "stored_pseudo_variables fixture_7");

    // expected group attribute data
    std::vector< std::string > expected_attr_group_data_1{ "X__[-]",
                                                           "Y__[-]",
                                                           "Z__[-]" };
    std::vector< std::string > expected_attr_group_data_2{ "X2__[-]",
                                                           "Y2__[-]",
                                                           "Z2__[-]" };

    // expected dataset attribute data
    std::vector< std::string > attr_dataset_data{ "resources_something",
                                                  "traitlength_something",
                                                  "fixture_7" };

    // expected dataset data
    std::vector< int > expected_base(100);
    std::iota(expected_base.begin(), expected_base.end(), 1);

    // use given adder '4' here for computing the result
    std::vector< int > expected_1;
    std::transform(expected_base.begin(),
                   expected_base.end(),
                   std::back_inserter(expected_1),
                   [](int v) { return v + 4; });

    std::vector< double > expected_2;
    std::transform(expected_base.begin(),
                   expected_base.end(),
                   std::back_inserter(expected_2),
                   [](int i) { return 2. * i + 1 + 4; });

    expected_2.insert(expected_2.end(), expected_2.begin(), expected_2.end());

    std::vector< hsize_t > expected_shape_1{ 100 };
    std::vector< hsize_t > expected_shape_2{ 200 };

    // read datasets and compare
    auto [shape1_0, data1_0] = dataset1_0->read< std::vector< int > >();
    auto [shape1_1, data1_1] = dataset1_1->read< std::vector< int > >();
    auto [shape2_0, data2_0] = dataset2_0->read< std::vector< double > >();

    BOOST_TEST(shape1_0 == expected_shape_1, boost::test_tools::per_element());
    BOOST_TEST(shape1_1 == expected_shape_1, boost::test_tools::per_element());
    BOOST_TEST(shape2_0 == expected_shape_2, boost::test_tools::per_element());

    BOOST_TEST(data1_0 == expected_1, boost::test_tools::per_element());
    BOOST_TEST(data1_1 == expected_1, boost::test_tools::per_element());
    BOOST_TEST(data2_0 == expected_2, boost::test_tools::per_element());

    // read attributes

    auto [attr_shape1_0, attr_data1_0] =
        attr_dset_1_0.read< std::vector< std::string > >();
    auto [attr_shape1_1, attr_data1_1] =
        attr_dset_1_1.read< std::vector< std::string > >();
    auto [attr_shape2_0, attr_data2_0] =
        attr_dset_2_0.read< std::vector< std::string > >();

    BOOST_TEST(attr_shape1_0 == (std::vector< hsize_t >{ 3 }),
               boost::test_tools::per_element());
    BOOST_TEST(attr_shape1_1 == (std::vector< hsize_t >{ 3 }),
               boost::test_tools::per_element());
    BOOST_TEST(attr_shape2_0 == (std::vector< hsize_t >{ 3 }),
               boost::test_tools::per_element());

    BOOST_TEST(attr_data1_0 == attr_dataset_data,
               boost::test_tools::per_element());
    BOOST_TEST(attr_data1_1 == attr_dataset_data,
               boost::test_tools::per_element());
    BOOST_TEST(attr_data2_0 == attr_dataset_data,
               boost::test_tools::per_element());

    auto [attr_shape_group_1, attr_data_group_1] =
        attr_group_1.read< std::vector< std::string > >();
    auto [attr_shape_group_2, attr_data_group_2] =
        attr_group_2.read< std::vector< std::string > >();

    BOOST_TEST(attr_shape_group_1 == (std::vector< hsize_t >{ 3 }),
               boost::test_tools::per_element());
    BOOST_TEST(attr_shape_group_2 == (std::vector< hsize_t >{ 3 }),
               boost::test_tools::per_element());

    BOOST_TEST(attr_data_group_1 == expected_attr_group_data_1,
               boost::test_tools::per_element());
    BOOST_TEST(attr_data_group_2 == expected_attr_group_data_2,
               boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(datamanager_default_config_check)
{


    using Task = Default::DefaultWriteTask< Model >;

    using DMT = DataManagerTraits< Task,
                                   Default::DefaultDecider< Model >,
                                   Default::DefaultTrigger< Model >,
                                   Default::DefaultExecutionProcess >;

    std::size_t file_postfix = 1;
    for (auto& filename :
         { "datamanager_test.yml", "datamanager_test_anchors.yml" })
    {

        Model model("fixture_9_" + std::to_string(file_postfix));
        ++file_postfix;
        auto cfg = YAML::LoadFile(filename);

        // build tasks here to check if they work
        Task t1( // model
                 // basegroup builder
            [](std::shared_ptr< HDFGroup >&& grp)
                -> std::shared_ptr< HDFGroup > {
                return grp->open_group("datagroup/1");
            },
            // writer function
            [](auto& dataset, Model& m) { dataset->write(m.x); },
            // builder function
            [](auto& group, Model& m) {
                return group->open_dataset("testgroup/initial_dataset1_" +
                                           m.name);
            },
            // attribute writer
            [](auto& hdfgroup, Model& m) {
                hdfgroup->add_attribute(
                    "dimension names for " + m.name,
                    std::vector< std::string >{ "X", "Y", "Z" });
            },
            [](auto& hdfdataset, Model& m) {
                hdfdataset->add_attribute(
                    "cell_data",
                    std::vector< std::string >{
                        "resources", "traitlength", m.name });
            });

        Task t2(
            // basegroup builder
            [](std::shared_ptr< HDFGroup >&& grp)
                -> std::shared_ptr< HDFGroup > {
                return grp->open_group("datagroup2/2");
            },
            // writer function
            [](auto& dataset, Model& m) {
                std::vector< int > v;
                std::transform(m.x.begin(),
                               m.x.end(),
                               std::back_inserter(v),
                               [](int i) { return 2 * i + 1; });
                dataset->write(v);
            },
            // builder function
            [](auto& group, Model& m) {
                return group->open_dataset("testgroup/initial_dataset2_" +
                                           m.name);
            },
            // attribute writer
            [](auto& hdfgroup, Model& m) {
                hdfgroup->add_attribute(
                    "dimension names for " + m.name,
                    std::vector< std::string >{ "X", "Y", "Z" });
            },
            [](auto& hdfdataset, Model& m) {
                hdfdataset->add_attribute(
                    "cell_data",
                    std::vector< std::string >{
                        "resources"s, "traitlength", m.name });
            });

        Task t3( // model
                 // basegroup builder
            [](std::shared_ptr< HDFGroup >&& grp)
                -> std::shared_ptr< HDFGroup > {
                return grp->open_group("datagroup3/3");
            },
            // writer function
            [](auto& dataset, Model& m) {
                std::vector< int > v;
                std::transform(m.x.begin(),
                               m.x.end(),
                               std::back_inserter(v),
                               [](int i) { return 5 * i + 2; });
                dataset->write(v);
            },
            // builder function
            [](auto& group, Model& m) {
                return group->open_dataset("testgroup/initial_dataset3_" +
                                           m.name);
            },
            // attribute writer
            [](auto& hdfgroup, Model& m) {
                hdfgroup->add_attribute(
                    "dimension names for " + m.name,
                    std::vector< std::string >{ "X", "Y", "Z" });
            },
            [](auto& hdfdataset, Model& m) {
                hdfdataset->add_attribute(
                    "cell_data",
                    std::vector< std::string >{
                        "resources"s, "traitlength", m.name });
            });

        Utopia::DataIO::DataManager< DMT > dm(
            cfg["data_manager"],
            // tasks
            { { "write_x"s, std::make_shared< Task >(t1) },
              { "write_y"s, std::make_shared< Task >(t2) },
              { "write_z"s, std::make_shared< Task >(t3) } },
            // decidertypes
            Default::default_decidertypes< Model >,
            // triggertypes
            Default::default_triggertypes< Model >,
            // executionprocess
            Default::DefaultExecutionProcess());

        // check that the deciders are present with the correct names
        bool found = (dm.get_deciders().find("write_intervals") !=
                 dm.get_deciders().end());
        BOOST_TEST(found == true);

        // check that the triggers are present with the correct names
        found = (dm.get_triggers().find("build_intervals") !=
                 dm.get_triggers().end());
        BOOST_TEST(found == true);

        found =
            (dm.get_triggers().find("build_once") != dm.get_triggers().end());
        BOOST_TEST(found == true);

        // check that deciders work correctly according to config file
        model.time = 0;
        for (; model.time < 150; ++model.time)
        {
            if (model.time < 10)
            {
                BOOST_TEST((*dm.get_deciders()["write_intervals"])(model) ==
                           true);
            }
            else if (model.time >= 25 and model.time < 30)
            {
                BOOST_TEST((*dm.get_deciders()["write_intervals"])(model) ==
                           true);
            }
            else if (model.time >= 96 and model.time < 100 and
                     (model.time - 96) % 2 == 0)
            {
                BOOST_TEST((*dm.get_deciders()["write_intervals"])(model) ==
                           true);
            }
            else if (model.time >= 100 and model.time < 115 and
                     (model.time - 100) % 5 == 0)
            {
                BOOST_TEST((*dm.get_deciders()["write_intervals"])(model) ==
                           true);
            }
            else
            {
                BOOST_TEST((*dm.get_deciders()["write_intervals"])(model) ==
                           false);
            }
        }

        // check that triggers work correctly according to config
        model.time = 0;
        for (; model.time < 100; ++model.time)
        {
            if (model.time == 0)
            {
                BOOST_TEST((*dm.get_triggers()["build_once"])(model) == true);
            }
            else
            {
                BOOST_TEST((*dm.get_triggers()["build_once"])(model) == false);
            }
        }

        model.time = 0;
        for (; model.time < 150; ++model.time)
        {
            if (model.time < 10)
            {
                BOOST_TEST((*dm.get_triggers()["build_intervals"])(model) ==
                           true);
            }
            else if (model.time >= 25 and model.time < 30)
            {
                BOOST_TEST((*dm.get_triggers()["build_intervals"])(model) ==
                           true);
            }
            else if (model.time >= 96 and model.time < 100 and
                     (model.time - 96) % 2 == 0)
            {
                BOOST_TEST((*dm.get_triggers()["build_intervals"])(model) ==
                           true);
            }
            else if (model.time >= 100 and model.time < 115 and
                     (model.time - 100) % 5 == 0)
            {
                BOOST_TEST((*dm.get_triggers()["build_intervals"])(model) ==
                           true);
            }
            else
            {
                BOOST_TEST((*dm.get_triggers()["build_intervals"])(model) ==
                           false);
            }
        }

        // check associations of deciders
        BOOST_TEST(dm.get_decider_task_map()["write_intervals"] ==
                       (std::vector< std::string >{ "write_x",
                                                    "write_y", 
                                                    "write_z" }),
                   boost::test_tools::per_element());

        // check associations of triggers
        BOOST_TEST(dm.get_trigger_task_map()["build_once"] ==
                       (std::vector< std::string >{ "write_x",
                                                    "write_y" }),
                   boost::test_tools::per_element());
        BOOST_TEST(dm.get_trigger_task_map()["build_intervals"] ==
                       (std::vector< std::string >{ "write_z" }),
                   boost::test_tools::per_element());



        // test datamanager with one task being marked inactive
        Utopia::DataIO::DataManager< DMT > dm0(
            cfg["data_manager0"],
            // tasks
            { { "write_x"s, std::make_shared< Task >(t1) },
              { "write_y"s, std::make_shared< Task >(t2) },
              { "write_z"s, std::make_shared< Task >(t3) } },
            // decidertypes
            Default::default_decidertypes< Model >,
            // triggertypes
            Default::default_triggertypes< Model >,
            // executionprocess
            Default::DefaultExecutionProcess());

        // check that everything is registered correctly
        found = dm0.get_tasks().find("write_x") != dm0.get_tasks().end();
        BOOST_TEST(not found);

        found = dm0.get_tasks().find("write_y") != dm0.get_tasks().end();
        BOOST_TEST(found);

        found = dm0.get_tasks().find("write_z") != dm0.get_tasks().end();
        BOOST_TEST(found);

        found = (dm0.get_deciders().find("write_intervals") !=
                 dm0.get_deciders().end());
        BOOST_TEST(found == true);
        found = (dm0.get_deciders().find("write_intervals") !=
                 dm0.get_deciders().end());
        BOOST_TEST(found == true);
        found = (dm0.get_deciders().find("write_intervals") !=
                 dm0.get_deciders().end());
        BOOST_TEST(found == true);

        // check that the triggers are present with the correct names
        found =
            (dm0.get_triggers().find("build_once") != dm0.get_triggers().end());
        BOOST_TEST(found == true);

        found = (dm0.get_triggers().find("build_intervals") !=
                 dm0.get_triggers().end());
        BOOST_TEST(found == true);
    }
}

BOOST_AUTO_TEST_CASE(datamanager_test_execprocess_with_config)
{
    // prepare model and data
    Model model("fixture_10");
    model.x = std::vector< int >(100);
    std::iota(model.x.begin(), model.x.end(), 0);

    // load yaml file
    auto cfg = YAML::LoadFile("datamanager_test.yml");

    // build tasks
    using Task = Default::DefaultWriteTask< Model >;

    using DMT = DataManagerTraits< Task,
                                   Default::DefaultDecider< Model >,
                                   Default::DefaultTrigger< Model >,
                                   Default::DefaultExecutionProcess >;

    Task t1(
        [](std::shared_ptr< HDFGroup >&& grp) -> std::shared_ptr< HDFGroup > {
            return grp->open_group("datagroup/1");
        },
        // writer function
        [](auto& dataset, Model& m) { dataset->write(m.x); },
        // builder function
        [](auto& group, Model& m) {
            return group->open_dataset("testgroup/initial_dataset1_" +
                                       std::to_string(m.time));
        },
        // attribute writer
        [](auto& hdfgroup, Model& m) {
            hdfgroup->add_attribute(
                "dimension names for " + m.name + " " + std::to_string(m.time),
                std::vector< std::string >{ "X", "Y", "Z" });
        },
        [](auto& hdfdataset, Model& m) {
            hdfdataset->add_attribute("cell_data " + std::to_string(m.time),
                                      std::vector< std::string >{
                                          "resources", "traitlength", m.name });
        });

    Task t2(
        [](std::shared_ptr< HDFGroup >&& grp) -> std::shared_ptr< HDFGroup > {
            return grp->open_group("datagroup2/2");
        },
        // writer function
        [](auto& dataset, Model& m) {
            std::vector< int > v;
            std::transform(m.x.begin(),
                           m.x.end(),
                           std::back_inserter(v),
                           [](int i) { return 2 * i; });
            dataset->write(v);
        },
        // builder function
        [](auto& group, Model& m) {
            return group->open_dataset("testgroup/initial_dataset2_" +
                                       std::to_string(m.time));
        },
        // attribute writer
        [](auto& hdfgroup, Model& m) {
            hdfgroup->add_attribute(
                "dimension names for " + m.name + " " + std::to_string(m.time),
                std::vector< std::string >{ "X", "Y", "Z" });
        },
        [](auto& hdfdataset, Model& m) {
            hdfdataset->add_attribute("cell_data " + std::to_string(m.time),
                                      std::vector< std::string >{ "resources"s,
                                                                  "traitlength",
                                                                  m.name });
        });

    Task t3(
        [](std::shared_ptr< HDFGroup >&& grp) -> std::shared_ptr< HDFGroup > {
            return grp->open_group("datagroup3/3");
        },
        // writer function
        [](auto& dataset, Model& m) {
            std::vector< int > v;
            std::transform(m.x.begin(),
                           m.x.end(),
                           std::back_inserter(v),
                           [](int i) { return 3 * i; });
            dataset->write(v);
        },
        // builder function
        [](auto& group, Model& m) {
            return group->open_dataset("testgroup/initial_dataset3_" +
                                       std::to_string(m.time));
        },
        // attribute writer
        [](auto& hdfgroup, Model& m) {
            hdfgroup->add_attribute(
                "dimension names for " + m.name + " " + std::to_string(m.time),
                std::vector< std::string >{ "X", "Y", "Z" });
        },
        [](auto& hdfdataset, Model& m) {
            hdfdataset->add_attribute("cell_data " + std::to_string(m.time),
                                      std::vector< std::string >{ "resources"s,
                                                                  "traitlength",
                                                                  m.name });
        });

    Utopia::DataIO::DataManager< DMT > dm(
        cfg["data_manager"],
        // tasks
        { { "write_x"s, std::make_shared< Task >(t1) },
          { "write_y"s, std::make_shared< Task >(t2) },
          { "write_z"s, std::make_shared< Task >(t3) } },
        Default::default_decidertypes< Model >,
        Default::default_triggertypes< Model >,
        Default::DefaultExecutionProcess());

    // call the data writer
    for (model.time = 0; model.time < 200; ++model.time)
    {
        dm(model);
    }

    // check that there are objects in the file which have the correct number
    // and place

    // task1
    BOOST_TEST(
        path_exists(model.file.get_basegroup()->get_C_id(), "datagroup/1"));
    BOOST_TEST(path_exists(model.file.get_basegroup()->get_C_id(),
                           "datagroup/1/testgroup/initial_dataset1_0"));

    // task2
    BOOST_TEST(
        path_exists(model.file.get_basegroup()->get_C_id(), "datagroup2/2"));
    BOOST_TEST(path_exists(model.file.get_basegroup()->get_C_id(),
                           "datagroup2/2/testgroup/initial_dataset2_0"));

    //     // Task3
    BOOST_TEST(
        path_exists(model.file.get_basegroup()->get_C_id(), "datagroup3/3"));

    for (model.time = 0; model.time < 200; ++model.time)
    {
        if (model.time < 10)
        {
            BOOST_TEST(path_exists(model.file.get_basegroup()->get_C_id(),
                                   "datagroup3/3/testgroup/initial_dataset3_" +
                                       std::to_string(model.time)));
        }
        else if (model.time >= 25 and model.time < 30)
        {
            BOOST_TEST(path_exists(model.file.get_basegroup()->get_C_id(),
                                   "datagroup3/3/testgroup/initial_dataset3_" +
                                       std::to_string(model.time)));
        }
        else if (model.time >= 96 and model.time < 100 and
                 (model.time - 96) % 2 == 0)
        {
            BOOST_TEST(path_exists(model.file.get_basegroup()->get_C_id(),
                                   "datagroup3/3/testgroup/initial_dataset3_" +
                                       std::to_string(model.time)));
        }
        else if (model.time >= 100 and model.time < 115 and
                 (model.time - 100) % 5 == 0)
        {
            BOOST_TEST(path_exists(model.file.get_basegroup()->get_C_id(),
                                   "datagroup3/3/testgroup/initial_dataset3_" +
                                       std::to_string(model.time)));
        }
        else
        {
            // do nothing
        }
    }
}

BOOST_AUTO_TEST_CASE(datamanager_config_written_data_check)
{

    HDFFile file("fixture_10.h5", "r");

    std::vector< int > testdata(100);
    std::iota(testdata.begin(), testdata.end(), 0);

    // task1
    auto group = file.open_group("/datagroup/1/testgroup");
    auto dset = group->open_dataset("initial_dataset1_0");
    auto [shape, data] = dset->read< std::vector< int > >();

    std::iota(testdata.begin(), testdata.end(), 0);
    std::transform(testdata.begin(),
                   testdata.end(),
                   testdata.begin(),
                   [](int i) { return i; });

    auto testdata2 = testdata;
    // 20 writes on single build
    for (std::size_t i = 0; i < 19; i++) {
        testdata2.insert(testdata2.end(), testdata.begin(), testdata.end());
    }
    BOOST_TEST(shape == (std::vector< hsize_t >{ 2000 }),
               boost::test_tools::per_element());
    BOOST_TEST(data == testdata2, boost::test_tools::per_element());

    // task2
    group = file.open_group("/datagroup2/2/testgroup");
    dset = group->open_dataset("initial_dataset2_0");
    std::tie(shape, data) = dset->read< std::vector< int > >();

    std::iota(testdata.begin(), testdata.end(), 0);
    std::transform(testdata.begin(),
                   testdata.end(),
                   testdata.begin(),
                   [](int i) { return i * 2; });

    testdata2 = testdata;
    // 20 writes on single build
    for (std::size_t i = 0; i < 19; i++) {
        testdata2.insert(testdata2.end(), testdata.begin(), testdata.end());
    }

    BOOST_TEST(shape == (std::vector< hsize_t >{ 2000 }),
               boost::test_tools::per_element());
    BOOST_TEST(data == testdata2, boost::test_tools::per_element());

    // task3
    group = file.open_group("/datagroup3/3/testgroup");

    testdata.resize(100, 0);
    std::iota(testdata.begin(), testdata.end(), 0);
    std::transform(testdata.begin(),
                   testdata.end(),
                   testdata.begin(),
                   [](int i) { return i * 3; });

    for (std::size_t t = 0; t < 120; ++t)
    {
        if (t < 10)
        {
            auto dset =
                group->open_dataset("initial_dataset3_" + std::to_string(t));

            auto [shape, data] = dset->read< std::vector< int > >();

            BOOST_TEST(shape == (std::vector< hsize_t >{ 100 }),
                       boost::test_tools::per_element());
            BOOST_TEST(data == testdata, boost::test_tools::per_element());
        }
        else if (t >= 25 and t < 30)
        {
            auto dset =
                group->open_dataset("initial_dataset3_" + std::to_string(t));
            auto [shape, data] = dset->read< std::vector< int > >();
            BOOST_TEST(shape == (std::vector< hsize_t >{ 100 }),
                       boost::test_tools::per_element());
            BOOST_TEST(data == testdata, boost::test_tools::per_element());
        }
        else if (t >= 96 and t < 100 and (t - 96) % 2 == 0)
        {
            auto dset =
                group->open_dataset("initial_dataset3_" + std::to_string(t));
            auto [shape, data] = dset->read< std::vector< int > >();
            BOOST_TEST(shape == (std::vector< hsize_t >{ 100 }),
                       boost::test_tools::per_element());
            BOOST_TEST(data == testdata, boost::test_tools::per_element());
        }
        else if (t >= 100 and t < 115 and (t - 100) % 5 == 0)
        {
            auto dset =
                group->open_dataset("initial_dataset3_" + std::to_string(t));
            auto [shape, data] = dset->read< std::vector< int > >();
            BOOST_TEST(shape == (std::vector< hsize_t >{ 100 }),
                       boost::test_tools::per_element());
            BOOST_TEST(data == testdata, boost::test_tools::per_element());
        }
        else
        {
            // do nothing
        }
    }
}

BOOST_AUTO_TEST_CASE(datamanager_exception_check)
{
    // prepare model and data
    Model model("fixture_11");
    model.x = std::vector< int >(100);
    std::iota(model.x.begin(), model.x.end(), 0);

    // load yaml file
    auto cfg = YAML::LoadFile("datamanager_test.yml");

    // build tasks
    using Task = Default::DefaultWriteTask< Model >;

    using DMT = DataManagerTraits< Task,
                                   Default::DefaultDecider< Model >,
                                   Default::DefaultTrigger< Model >,
                                   Default::DefaultExecutionProcess >;

    Task t1(
        [](std::shared_ptr< HDFGroup >&& grp) -> std::shared_ptr< HDFGroup > {
            return grp->open_group("datagroup/1");
        },
        // writer function
        [](auto& dataset, Model& m) { dataset->write(m.x); },
        // builder function
        [](auto& group, Model& m) {
            return group->open_dataset("testgroup/initial_dataset1_" +
                                       std::to_string(m.time));
        },
        // attribute writer
        [](auto& hdfgroup, Model& m) {
            hdfgroup->add_attribute(
                "dimension names for " + m.name + " " + std::to_string(m.time),
                std::vector< std::string >{ "X", "Y", "Z" });
        },
        [](auto& hdfdataset, Model& m) {
            hdfdataset->add_attribute("cell_data " + std::to_string(m.time),
                                      std::vector< std::string >{
                                          "resources", "traitlength", m.name });
        });

    Task t2(
        [](std::shared_ptr< HDFGroup >&& grp) -> std::shared_ptr< HDFGroup > {
            return grp->open_group("datagroup2/2");
        },
        // writer function
        [](auto& dataset, Model& m) {
            std::vector< int > v;
            std::transform(m.x.begin(),
                           m.x.end(),
                           std::back_inserter(v),
                           [](int i) { return 2 * i; });
            dataset->write(v);
        },
        // builder function
        [](auto& group, Model& m) {
            return group->open_dataset("testgroup/initial_dataset2_" +
                                       std::to_string(m.time));
        },
        // attribute writer
        [](auto& hdfgroup, Model& m) {
            hdfgroup->add_attribute(
                "dimension names for " + m.name + " " + std::to_string(m.time),
                std::vector< std::string >{ "X", "Y", "Z" });
        },
        [](auto& hdfdataset, Model& m) {
            hdfdataset->add_attribute("cell_data " + std::to_string(m.time),
                                      std::vector< std::string >{ "resources"s,
                                                                  "traitlength",
                                                                  m.name });
        });

    Task t3(
        [](std::shared_ptr< HDFGroup >&& grp) -> std::shared_ptr< HDFGroup > {
            return grp->open_group("datagroup3/3");
        },
        // writer function
        [](auto& dataset, Model& m) {
            std::vector< int > v;
            std::transform(m.x.begin(),
                           m.x.end(),
                           std::back_inserter(v),
                           [](int i) { return 3 * i; });
            dataset->write(v);
        },
        // builder function
        [](auto& group, Model& m) {
            return group->open_dataset("testgroup/initial_dataset3_" +
                                       std::to_string(m.time));
        },
        // attribute writer
        [](auto& hdfgroup, Model& m) {
            hdfgroup->add_attribute(
                "dimension names for " + m.name + " " + std::to_string(m.time),
                std::vector< std::string >{ "X", "Y", "Z" });
        },
        [](auto& hdfdataset, Model& m) {
            hdfdataset->add_attribute("cell_data " + std::to_string(m.time),
                                      std::vector< std::string >{ "resources"s,
                                                                  "traitlength",
                                                                  m.name });
        });

    try
    {
        Utopia::DataIO::DataManager< DMT > dm(
            cfg["data_manager1"],
            // tasks
            { { "write_x"s, std::make_shared< Task >(t1) },
              { "write_y"s, std::make_shared< Task >(t2) },
              { "write_z"s, std::make_shared< Task >(t3) } },
            Default::default_decidertypes< Model >,
            Default::default_triggertypes< Model >,
            Default::DefaultExecutionProcess());
    }
    catch (std::exception& e)
    {
        std::stringstream s;
        s << e.what();

        BOOST_TEST(e.what() ==
                   "Error when trying to associate tasks to deciders or "
                   "triggers: Name in config does not match the name of a "
                   "decider/trigger known to the datamanager");
    }

    try
    {
        Utopia::DataIO::DataManager< DMT > dm(
            cfg["data_manager3"], // wrong name of node given -> zombie
            // tasks
            { { "write_x"s, std::make_shared< Task >(t1) },
              { "write_y"s, std::make_shared< Task >(t2) },
              { "write_z"s, std::make_shared< Task >(t3) } },
            Default::default_decidertypes< Model >,
            Default::default_triggertypes< Model >,
            Default::DefaultExecutionProcess());
    }
    catch (std::exception& e)
    {
        std::stringstream s;
        s << e.what();

        BOOST_TEST(
            s.str() ==
            "Received a zombie node for the setup of DataManager objects!");
    }

    std::cout << (cfg["data_manager2"] ? "active" : "zombie") << std::endl;
    std::cout << cfg["data_manager2"].IsMap() << std::endl;

    try
    {
        Utopia::DataIO::DataManager< DMT > dm(
            cfg["data_manager2"],
            // tasks
            { { "write_x"s, std::make_shared< Task >(t1) },
              { "write_y"s, std::make_shared< Task >(t2) },
              { "write_z"s, std::make_shared< Task >(t3) } },
            Default::default_decidertypes< Model >,
            Default::default_triggertypes< Model >,
            Default::DefaultExecutionProcess());
    }
    catch (std::exception& e)
    {
        std::stringstream s;
        s << e.what();

        BOOST_TEST(s.str() ==
                   "Expected a mapping for DataManager object setup, got:\n" +
                       to_string(cfg["data_manager2"]["deciders"]));
    }

    try
    {
        Utopia::DataIO::DataManager< DMT > dm(
            cfg["data_manager4"],
            // tasks
            { { "write_x"s, std::make_shared< Task >(t1) },
              { "write_y"s, std::make_shared< Task >(t2) },
              { "write_z"s, std::make_shared< Task >(t3) } },
            Default::default_decidertypes< Model >,
            Default::default_triggertypes< Model >,
            Default::DefaultExecutionProcess());
    }
    catch (std::exception& e)
    {
        std::stringstream s;
        s << e.what();

        BOOST_TEST(s.str() ==
                   "Error for node write_intervals: No 'type' node given");
    }

    try
    {
        Utopia::DataIO::DataManager< DMT > dm(
            cfg["data_manager5"],
            // tasks
            { { "write_x"s, std::make_shared< Task >(t1) },
              { "write_y"s, std::make_shared< Task >(t2) },
              { "write_z"s, std::make_shared< Task >(t3) } },
            Default::default_decidertypes< Model >,
            Default::default_triggertypes< Model >,
            Default::DefaultExecutionProcess());
    }
    catch (std::exception& e)
    {
        std::stringstream s;
        s << e.what();

        BOOST_TEST(s.str() == "Expected a mapping for DataManager task "
                              "filtering, got:\n[a, b, c, d]");
    }
}

BOOST_AUTO_TEST_SUITE_END()