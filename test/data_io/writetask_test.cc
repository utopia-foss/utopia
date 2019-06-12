#define BOOST_TEST_MODULE writetask_test

#include <sstream> // needed for testing throw messages from exceptions

#include <boost/mpl/list.hpp>       // type lists for testing
#include <boost/test/included/unit_test.hpp> // for unit tests

#include <utopia/core/utils.hh> // for output operators, get size...

#include <utopia/data_io/hdfdataset.hh> // for messing around with datasets
#include <utopia/data_io/hdffile.hh>    // for buiding model mock class

#include <utopia/data_io/data_manager/defaults.hh>
#include <utopia/data_io/data_manager/write_task.hh>

#include "dataio_test.hh"

namespace utf = boost::unit_test;
using namespace std::literals;
using namespace Utopia::DataIO;
using namespace Utopia::Utils;

// test write functionality
BOOST_AUTO_TEST_CASE(writetask_write_functionality)
{
    Utopia::setup_loggers();
    spdlog::get("data_io")->set_level(spdlog::level::debug);

    Model model("writetask_testmodel");
    Default::DefaultWriteTask<Model> wt(
      // basegroup builder
      [](std::shared_ptr<HDFGroup>&& grp) {
          return grp->open_group("/datagroup");
      },
      // writer function
      [](auto& dataset, Model& m) { dataset->write(m.x); },
      // builder function
      [](auto& group, Model& m) {
          return group->open_dataset("testgroup/initial_dataset_" + m.name);
      },
      [](std::shared_ptr<HDFGroup>& group, Model& m) {
          group->add_attribute("some name group",
                               "some data in group"s + " "s + m.name);
      },
      // attribute writer
      [](std::shared_ptr<HDFDataset<HDFGroup>>& dataset, Model& m) {
          dataset->add_attribute("some name dataset",
                                 "some data in dataset"s + " "s + m.name);
      });

    // build base group
    wt.base_group = wt.build_basegroup(model.get_hdfgrp());

    // test writing of data
    BOOST_REQUIRE(wt.get_base_path() == "/datagroup");

    wt.active_dataset = wt.build_dataset(wt.base_group, model);
    BOOST_REQUIRE(wt.get_active_path() ==
                  "testgroup/initial_dataset_writetask_testmodel");

    wt.write_data(wt.active_dataset, model);

    // check that the dataset is a valid path in the file
    BOOST_REQUIRE(path_exists(wt.base_group->get_id(),
                              "testgroup/initial_dataset_writetask_testmodel") >
                  0);

    wt.write_attribute_active_dataset(wt.active_dataset, model);

    wt.write_attribute_basegroup(wt.base_group, model);
}

// test what has been written to hdf5
BOOST_AUTO_TEST_CASE(writetask_read_written_data)
{
    HDFFile file("writetask_testmodel.h5", "r");
    auto group = file.open_group("/datagroup/testgroup");

    auto dset = group->open_dataset("initial_dataset_writetask_testmodel");

    // read written data
    auto [shape, data] = dset->read<std::vector<int>>();

    std::vector<hsize_t> expected_shape{ 100 };
    std::vector<int> expected_data(100);
    std::iota(expected_data.begin(), expected_data.end(), 1);

    BOOST_TEST(shape == expected_shape);
    BOOST_TEST(data == expected_data);

    // read written attributes

    HDFAttribute group_attr(*file.open_group("/datagroup"), "some name group");
    auto [group_shape, group_data] = group_attr.read<std::string>();
    BOOST_TEST(group_shape == std::vector<hsize_t>{ 1 });
    BOOST_TEST(group_data == "some data in group writetask_testmodel");

    HDFAttribute dataset_attr(*dset, "some name dataset");
    auto [dset_shape, dset_data] = dataset_attr.read<std::string>();
    BOOST_TEST(dset_shape == std::vector<hsize_t>{ 1 });
    BOOST_TEST(dset_data == "some data in dataset writetask_testmodel");
}

// test swap function
BOOST_AUTO_TEST_CASE(writetask_lifecycle)
{
    Model model("writetask_testmodel_2");

    

    // swap FIXME: 

    using BasegroupBuilder = std::function<std::shared_ptr<HDFGroup>(Model&)>;
    using Writer =
      std::function<void(std::shared_ptr<HDFDataset<HDFGroup>>&, Model&)>;
    using Builder = std::function<std::shared_ptr<HDFDataset<HDFGroup>>(
      std::shared_ptr<HDFGroup>&, Model&)>;
    using AttributeWriterGroup = std::function<void(
      std::shared_ptr<HDFGroup>&, std::string&, std::string&)>;
    using AttributeWriterDataset = std::function<void(
      std::shared_ptr<HDFDataset<HDFGroup>>&, std::string, std::string)>;

    using WT = WriteTask<BasegroupBuilder,
                         Writer,
                         Builder,
                         AttributeWriterGroup,
                         AttributeWriterDataset>;

    WT wt1(
      // basegroup builder
      [](Model& m) { return m.file.open_group("/datagroup1"); },
      // writer function
      [](auto& dataset, Model& m) { dataset->write(m.x); },
      // builder function
      [](auto& group, Model& m) {
          return group->open_dataset("testgroup1/initial_dataset_" + m.name);
      },
      [](std::shared_ptr<HDFGroup>& group, std::string a, std::string b) {
          group->add_attribute("some name group1",
                               "some data in group1"s + " "s + a + b);
      },
      // attribute writer
      [](std::shared_ptr<HDFDataset<HDFGroup>>& dataset,
         std::string a,
         std::string b) {
          dataset->add_attribute("some name dataset1",
                                 "some data in dataset1"s + " "s + a + b);
      });

    wt1.base_group = wt1.build_basegroup(model);
    wt1.active_dataset = wt1.build_dataset(wt1.base_group, model);

    WT wt2(
      // basegroup builder
      [](Model& m) { return m.file.open_group("/datagroup2"); },
      // writer function
      [](auto& dataset, Model& m) { dataset->write(m.x); },
      // builder function
      [](auto& group, Model& m) {
          return group->open_dataset("testgroup1/initial_dataset_" + m.name);
      },
      [](std::shared_ptr<HDFGroup>& group, std::string a, std::string b) {
          group->add_attribute("some name group2",
                               "some data in group1"s + " "s + a + b);
      },
      // attribute writer
      [](std::shared_ptr<HDFDataset<HDFGroup>>& dataset,
         std::string a,
         std::string b) {
          dataset->add_attribute("some name dataset2",
                                 "some data in dataset1"s + " "s + a + b);
      });


    wt2.base_group = wt2.build_basegroup(model);
    wt2.active_dataset = wt2.build_dataset(wt2.base_group, model);


    auto wt1_cpy = wt1;
    auto wt2_cpy = wt2;

    wt1.swap(wt2);

    BOOST_TEST(wt1.base_group == wt2_cpy.base_group);
    BOOST_TEST(wt1.active_dataset == wt2_cpy.active_dataset);

    BOOST_TEST(wt2.base_group == wt1_cpy.base_group);
    BOOST_TEST(wt2.active_dataset == wt1_cpy.active_dataset);


    // test fucking shared_ptr making which segfaulted before

}
