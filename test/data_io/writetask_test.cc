#define BOOST_TEST_MODULE datamanager_test
#include <boost/mpl/list.hpp>       // type lists for testing
#include <boost/test/unit_test.hpp> // for unit tests
// #include <boost/test/included/unit_test.hpp>

#include <sstream> // needed for testing throw messages from exceptions
#include <utopia/core/utils.hh>         // for output operators, get size...
#include <utopia/data_io/hdfdataset.hh> // for messing around with datasets
#include <utopia/data_io/hdffile.hh>    // for buiding model mock class
#include <utopia/data_io/write_task.hh>

namespace utf = boost::unit_test;
using namespace std::literals;
using namespace Utopia::DataIO;
using namespace Utopia::Utils;

/**
 * @brief Mock class for Model
 * 
 */
struct Model
{
    std::string name;
    Utopia::DataIO::HDFFile file;
    std::shared_ptr<spdlog::logger> logger;
    std::vector<int> x;
    auto get_logger()
    {
        return logger;
    }

    Model(std::string n)
        : name(n), file(n + ".h5", "w"), logger(spdlog::stdout_color_mt("logger." + n)),
        // mock data
        x([](){
            std::vector<int> v(100);
            std::iota(v.begin(), v.end(), 1);
            return v;
        }())
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

using Writer =
    std::function<void(Model&, std::shared_ptr<Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>>&)>;
using Builder =
    std::function<std::shared_ptr<Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>>(
        Model&, std::string path, std::string)>;

// test write functionality
BOOST_AUTO_TEST_CASE(writetask_write_functionality)
{
    Utopia::setup_loggers();
    spdlog::get("data_io")->set_level(spdlog::level::debug);

    Model model("writetask_testmodel");
    WriteTask wt(
        // base group to use, required
        model.file.open_group("/datagroup"),
        // path to new dataset, required
        "testgroup/initial_dataset",
        // writer function
        [](auto& dataset, Model& m) { dataset->write(m.x); },
        // builder function
        [](auto& group, std::string path, Model& m) {
            return group->open_dataset(path + "_" + m.name);
        },
        // attribute writer
        [](auto& hdfobject, std::string name, auto&& v) {
            hdfobject->add_attribute(name, v);
        },
        // builder args other than group and path
        model);

    // test writing of data
    BOOST_REQUIRE(wt.get_base_path() == "/datagroup");
    BOOST_REQUIRE(wt.get_active_path() ==
                  "testgroup/initial_dataset_writetask_testmodel");

    wt.write_data(wt.active_dataset, model);

    // check that the dataset is a valid path in the file
    BOOST_REQUIRE(path_exists(wt.base_group->get_id(),
                              "testgroup/initial_dataset_writetask_testmodel") > 0);

    wt.write_attribute(
        wt.active_dataset, "a", std::vector<int>{1, 2, 3, 4, 5, 6});

    wt.write_attribute(
        wt.active_dataset,"b", std::vector<double>{3.14, 4.14, 5.14, 6.14});

    wt.write_attribute(
        wt.active_dataset, "dimensions", std::vector<std::string>{"a", "b"});

    wt.write_attribute(
        wt.active_dataset, 
        "test_attribute", "This is the active dataset, it is exchangeable");

    wt.write_attribute(
        wt.base_group,
        "test_attribute", "This is the base_group, it is not exchangeable");
}

// test what has been written to hdf5 
BOOST_AUTO_TEST_CASE(writetask_read_written_data)
{
    HDFFile file("writetask_testmodel.h5", "r");
    auto group = file.open_group("/datagroup/testgroup");

    auto dset = group->open_dataset("initial_dataset_writetask_testmodel");

    // read written data
    auto [shape, data] = dset->read<std::vector<int>>();

    std::vector<hsize_t> expected_shape{100};
    std::vector<int> expected_data(100);
    std::iota(expected_data.begin(), expected_data.end(), 1);

    BOOST_TEST(shape == expected_shape);
    BOOST_TEST(data == expected_data);

    // read default attributes
    auto expected_dimension_labels = std::vector<std::string>{"a", "b"};
    auto expected_coordinates_a = std::vector<int>{1, 2, 3, 4, 5, 6};
    auto expected_coordinates_b = std::vector<double>{3.14, 4.14, 5.14, 6.14};

    HDFAttribute dim_attr(*dset, "dimensions");
    auto [shape_dim, labels] = dim_attr.read<std::vector<std::string>>();
    BOOST_TEST(shape_dim == (std::vector<hsize_t>{2}));
    BOOST_TEST(labels == (std::vector<std::string>{"a", "b"}));

    HDFAttribute coord_attr_a(*dset, "a");
    HDFAttribute coord_attr_b(*dset, "b");

    auto [shape_a, coords_a] = coord_attr_a.read<std::vector<int>>();
    auto [shape_b, coords_b] = coord_attr_b.read<std::vector<double>>();

    BOOST_TEST(shape_a == (std::vector<hsize_t>{6}));
    BOOST_TEST(coords_a == expected_coordinates_a);

    BOOST_TEST(shape_b == (std::vector<hsize_t>{4}));
    BOOST_TEST(coords_b == expected_coordinates_b);

    // read custom attributes
    auto custom_attr1 = HDFAttribute(*dset, "test_attribute");
    auto [shape_custom1, cutom_data1] = custom_attr1.read<std::string>();
    BOOST_TEST(shape_custom1 == (std::vector<hsize_t>{1}));
    BOOST_TEST(cutom_data1 == "This is the active dataset, it is exchangeable"s);

    auto custom_attr2 = HDFAttribute(*file.open_group("/datagroup"), "test_attribute");
    auto [shape_custom2, cutom_data2] = custom_attr2.read<std::string>();
    BOOST_TEST(shape_custom2 == (std::vector<hsize_t>{1}));
    BOOST_TEST(cutom_data2 == "This is the base_group, it is not exchangeable"s);
}

// // test swap function 
BOOST_AUTO_TEST_CASE(writetask_swap){
    Model model("writetask_testmodel_2");
    // example
    using Writer = std::function<void(std::shared_ptr<HDFDataset<HDFGroup>>&, Model&)>;
    using Builder = std::function<std::shared_ptr<HDFDataset<HDFGroup>>(std::shared_ptr<HDFGroup>&, std::string, Model&)>;
    using AttributeWriter = std::function<void(std::shared_ptr<HDFGroup>&, std::string& , std::string&)>;

    WriteTask wt1(
        // base group to use, required
        model.file.open_group("/datagroup1"),

        // path to new dataset, required
        "testgroup/initial_dataset1",
        // writer function
        Writer([](std::shared_ptr<HDFDataset<HDFGroup>>& dataset, Model& m) {
            dataset->write(m.x);
        }),
        // builder function
        Builder([](std::shared_ptr<HDFGroup>& group, std::string path, Model& m) {
            return group->open_dataset(path + "_" + m.name);
        }),
        // attribute writer
        AttributeWriter([](auto& hdfobject, std::string name, auto&& v) {
            hdfobject->add_attribute(name, v);
        }),
        // arguments to the build function
        model);

    WriteTask wt2(
        // base group to use, required
        model.file.open_group("/datagroup2"),

        // path to new dataset, required
        "testgroup/initial_dataset2",
        // writer function
        Writer([](std::shared_ptr<HDFDataset<HDFGroup>>& dataset, Model& m) {
            dataset->write(m.x);
        }),
        // builder function
        Builder([](std::shared_ptr<HDFGroup>& group, std::string path, Model& m) {
            return group->open_dataset(path + "_" + m.name);
        }),
        // attribute writer
        AttributeWriter([](auto& hdfobject, std::string name, auto&& v) {
            hdfobject->add_attribute(name, v);}),
        // arguments to the build function
        model);

    auto wt1_cpy = wt1;
    auto wt2_cpy = wt2; 

    wt1.swap(wt2);

    BOOST_TEST(wt1.base_group == wt2_cpy.base_group);
    BOOST_TEST(wt1.active_dataset == wt2_cpy.active_dataset);

    BOOST_TEST(wt2.base_group == wt1_cpy.base_group);
    BOOST_TEST(wt2.active_dataset == wt1_cpy.active_dataset);
}