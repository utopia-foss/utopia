#define BOOST_TEST_MODULE writetaskfactory_test

#include <numeric>
#include <sstream> // needed for testing throw messages from exceptions

#include <boost/mpl/list.hpp>                // type lists for testing
#include <boost/test/included/unit_test.hpp> // for unit tests

#include <utopia/core/type_traits.hh> // for output operators, get size...
#include <utopia/data_io/data_manager/data_manager.hh>
#include <utopia/data_io/data_manager/defaults.hh>
#include <utopia/data_io/data_manager/factory.hh>
#include <utopia/data_io/hdfdataset.hh> // for messing around with datasets
#include <utopia/data_io/hdffile.hh>    // for buiding model mock class

#include "testtools.hh"

namespace utf = boost::unit_test;
using namespace std::literals;
using namespace Utopia::DataIO;
using namespace Utopia::Utils;

// helper for array output
namespace boost
{
namespace test_tools
{
namespace tt_detail
{

template < typename T, std::size_t N >
struct print_log_value< std::array< T, N > >
{
    void
    operator()(std::ostream& os, std::array< T, N > const& pr)
    {
        os << "[";
        for (std::size_t i = 0; i < N - 1; ++i)
        {
            os << pr[i] << ", ";
        }
        os << pr[N - 1];
        os << "]";
    }
};
} // namespace tt_detail
} // namespace test_tools
} // namespace boost



// custom comparision operator for std::array, because of reasons... thx stl ...
template < typename T, std::size_t N >
bool
operator==(const std::array< T, N >& a, const std::array< T, N >& b)
{
    bool equal = true;
    for (std::size_t i = 0; i < N; ++i)
    {
        equal = (a[i] == b[i]);
    }
    return equal;
}

struct Fixture
{
    Model model   = Model("writetaskfactory_testmodel",
                        "datamanager_test_factory.yml",
                        1024,
                        256,
                        { 3, 1, 3.14 },
                        { 12, 24.314 });
    using G       = Graph_vertvecS_edgevecS_undir;
    using GModel  = GraphModel< G >;
    GModel gmodel = GModel("graphmodel", 1024, 4096);
};


struct Fix
{
    void
    setup()
    {
        Utopia::setup_loggers();
    }
};

BOOST_AUTO_TEST_SUITE(Suite, *boost::unit_test::fixture< Fix >())

// test write functionality without any additional stuff
BOOST_FIXTURE_TEST_CASE(writetaskfactory_basic,
                        Fixture,
                        *boost::unit_test::tolerance(1e-16))
{
    // needed for later
    using G      = Graph_vertvecS_edgevecS_undir;
    using GModel = GraphModel< G >;

    

    model.get_logger()->info("writetaskfactory_basic");

    BOOST_TEST(model.get_agentmanager().agents().size() == 256);
    for (auto& a : model.get_agentmanager().agents())
    {
        BOOST_TEST(a->state()._age == 12);
        BOOST_TEST(a->state()._adaption == 24.314,
                   boost::test_tools::tolerance(1e-16));
    }

    BOOST_TEST(model.get_cellmanager().cells().size() == 1024);
    for (auto& c : model.get_cellmanager().cells())
    {
        BOOST_TEST(c->state()._x == 3);
        BOOST_TEST(c->state()._y == 1);
        BOOST_TEST(c->state()._res == 3.14,
                   boost::test_tools::tolerance(1e-16));
    }

    // use TaskFactory for agents
    auto [name, task] = TaskFactory< Model >()(
        "adaption",
        "/basic",
        { "agent_dset", {}, {}, 1 },

        // keep this decltype(auto) here!! Will not return ref otherwise!
        // auto is insufficient
        // https://isocpp.org/wiki/faq/cpp14-language#decltype-auto
        // This is not specific to these factories!
        [](Model& model) -> decltype(auto) {
            return model.get_agentmanager().agents();
        },
        [](auto&& agent) -> decltype(auto) { return agent->state()._adaption; },
        Nothing{},
        Nothing{});

    BOOST_TEST(name == "adaption");

    // execute functions by hand and check what they
    // do in the next thing then
    task->base_group     = task->build_basegroup(model.get_hdfgrp());
    task->active_dataset = task->build_dataset(task->base_group, model);

    BOOST_TEST(check_validity(H5Iis_valid(task->base_group->get_C_id()),
                              "/basic") == true);
    // write out data
    task->write_data(task->active_dataset, model);
    BOOST_TEST(check_validity(H5Iis_valid(task->active_dataset->get_C_id()),
                              "agent_dset") == true); // FIXME: this does not

    // these are default constructed functions which should not
    // be usable. This is tested here
    bool active_dset_attr_writer = true;
    if (task->write_attribute_active_dataset)
    {
        task->write_attribute_active_dataset(task->active_dataset, model);
    }
    else
    {
        active_dset_attr_writer = false;
    }
    BOOST_TEST(active_dset_attr_writer == false);

    bool active_grp_attr_writer = true;
    if (task->write_attribute_basegroup)
    {
        task->write_attribute_basegroup(task->base_group, model);
    }
    else
    {
        active_grp_attr_writer = false;
    }
    BOOST_TEST(active_grp_attr_writer == false);

    // use TaskFactory for cells
    auto [name2, task2] = TaskFactory< Model >()(
        "x",
        "/basic",
        { "cell_dset" },

        [](auto& model) -> decltype(auto) {
            return model.get_cellmanager().cells();
        },
        [](auto&& cell) -> decltype(auto) { return cell->state()._x; },
        std::tuple{ "group_attribute"s, "this contains celldata"s },
        std::tuple{ "dataset_attribute"s,
                    "this saves the cell's x coordinate"s });

    BOOST_TEST(name2 == "x");
    // execute functions by hand and check what they do
    task2->base_group     = task2->build_basegroup(model.get_hdfgrp());
    task2->active_dataset = task2->build_dataset(task2->base_group, model);

    BOOST_TEST(check_validity(H5Iis_valid(task2->base_group->get_C_id()),
                              "/basic") == true);
    // write out data and stuff
    task2->write_data(task2->active_dataset, model);
    task2->write_attribute_active_dataset(task2->active_dataset, model);
    task2->write_attribute_basegroup(task2->base_group, model);
    BOOST_TEST(check_validity(H5Iis_valid(task2->active_dataset->get_C_id()),
                              "cell_dset") == true);

    auto [name_vertex, task_vertex] =
        TaskFactory< GModel, TypeTag::vertex_descriptor >()(
            "vertex_property_task",
            "/graph",
            { "vertex_property_dataset", { 1024 } },

            // mind the decltype(auto)
            [](auto& model) -> decltype(auto) { return model.get_graph(); },
            [](auto& g, auto& v) -> decltype(auto) { return g[v].id; },
            std::make_tuple("graph_group_attribute", "this contains graphdata"),
            std::make_tuple("dataset_attribute",
                            "this saves vertex indices or so"));

    BOOST_TEST(name_vertex == "vertex_property_task");
    // execute functions by hand and check what they do
    task_vertex->base_group = task_vertex->build_basegroup(gmodel.get_hdfgrp());
    task_vertex->active_dataset =
        task_vertex->build_dataset(task_vertex->base_group, gmodel);

    BOOST_TEST(check_validity(H5Iis_valid(task_vertex->base_group->get_C_id()),
                              "/graph") == true);

    // write out data and stuff
    task_vertex->write_data(task_vertex->active_dataset, gmodel);
    task_vertex->write_attribute_active_dataset(task_vertex->active_dataset,
                                                gmodel);
    task_vertex->write_attribute_basegroup(task_vertex->base_group, gmodel);
    BOOST_TEST(
        check_validity(H5Iis_valid(task_vertex->active_dataset->get_C_id()),
                       "vertex_property_dataset") == true);

    auto [name_edge_source, task_edge_source] =
        TaskFactory< GModel, TypeTag::edge_descriptor >()(
            "edge_property_task",
            "/graph",
            { "edge_property_dataset", { 2, 4096 } },

            [](auto& model) -> decltype(auto) { return model.get_graph(); },
            [](auto& g, auto& ed) -> decltype(auto) {
                return boost::get(
                    boost::vertex_index_t(), g, boost::source(ed, g));
            },
            std::make_tuple("graph_group_attribute", "this contains graphdata"),
            std::make_tuple("dataset_attribute", "this saves edges or so"));

    BOOST_TEST(name_edge_source == "edge_property_task");
    // execute functions by hand and check what they do
    task_edge_source->base_group =
        task_edge_source->build_basegroup(gmodel.get_hdfgrp());
    task_edge_source->active_dataset =
        task_edge_source->build_dataset(task_edge_source->base_group, gmodel);

    BOOST_TEST(
        check_validity(H5Iis_valid(task_edge_source->base_group->get_C_id()),
                       "/graph") == true);

    // // write out data and stuff
    task_edge_source->write_data(task_edge_source->active_dataset, gmodel);
    task_edge_source->write_attribute_active_dataset(
        task_edge_source->active_dataset, gmodel);
    task_edge_source->write_attribute_basegroup(task_edge_source->base_group,
                                                gmodel);
    BOOST_TEST(
        check_validity(H5Iis_valid(task_edge_source->active_dataset->get_C_id()),
                       "edge_property_dataset") == true);

    auto [name_edge_target, task_edge_target] =
        TaskFactory< GModel, TypeTag::edge_descriptor >()(
            "edge_property_task",
            "/graph",
            { "edge_property_dataset", { 2, 4096 } },

            [](auto& model) -> decltype(auto) { return model.get_graph(); },
            [](auto& g, auto& ed) -> decltype(auto) {
                return boost::get(
                    boost::vertex_index_t(), g, boost::target(ed, g));
            },
            Default::DefaultAttributeWriterGroup< GModel >{},
            Default::DefaultAttributeWriterDataset< GModel >{});

    BOOST_TEST(name_edge_target == "edge_property_task");
    // execute functions by hand and check what they do
    task_edge_target->base_group =
        task_edge_target->build_basegroup(gmodel.get_hdfgrp());
    task_edge_target->active_dataset =
        task_edge_target->build_dataset(task_edge_target->base_group, gmodel);

    BOOST_TEST(
        check_validity(H5Iis_valid(task_edge_target->base_group->get_C_id()),
                       "/graph") == true);

    // // write out data and stuff
    task_edge_target->write_data(task_edge_target->active_dataset, gmodel);
    BOOST_TEST(
        check_validity(H5Iis_valid(task_edge_target->active_dataset->get_C_id()),
                       "edge_property_dataset") == true);

    // test graph-structure writing shortcuts
    auto [name_vertices, task_vertices] =
        TaskFactory< GModel, TypeTag::vertex_descriptor >()(
            "vertices",
            "/graph_structure",
            { "vertices", { 1024 } },
            [](auto& model) -> decltype(auto) { return model.get_graph(); },
            std::function< void() >{},
            std::make_tuple("structure_attr",
                            "this group contains graph structure"),
            Default::DefaultAttributeWriterDataset< GModel >{});

    BOOST_TEST(name_vertices == "vertices");

    // execute functions by hand and check what they do
    task_vertices->base_group =
        task_vertices->build_basegroup(gmodel.get_hdfgrp());
    task_vertices->active_dataset =
        task_vertices->build_dataset(task_vertices->base_group, gmodel);

    task_vertices->write_attribute_basegroup(task_vertices->base_group, gmodel);

    BOOST_TEST(check_validity(H5Iis_valid(task_vertices->base_group->get_C_id()),
                              "/graph_structure") == true);

    // // write out data and stuff
    task_vertices->write_data(task_vertices->active_dataset, gmodel);
    BOOST_TEST(
        check_validity(H5Iis_valid(task_vertices->active_dataset->get_C_id()),
                       "vertices") == true);

    auto [name_edges, task_edges] =
        TaskFactory< GModel, TypeTag::edge_descriptor >()(
            "edges",
            "/graph_structure",
            { "edges", { 2, 4096 } },
            [](auto& model) -> decltype(auto) { return model.get_graph(); },
            std::function< void() >{},
            Default::DefaultAttributeWriterGroup< GModel >{},
            Default::DefaultAttributeWriterDataset< GModel >{});

    BOOST_TEST(name_edges == "edges");

    // execute functions by hand and check what they do
    task_edges->base_group = task_edges->build_basegroup(gmodel.get_hdfgrp());
    task_edges->active_dataset =
        task_edges->build_dataset(task_edges->base_group, gmodel);

    BOOST_TEST(check_validity(H5Iis_valid(task_edges->base_group->get_C_id()),
                              "/graph_structure") == true);

    // write out data and stuff
    task_edges->write_data(task_edges->active_dataset, gmodel);
    BOOST_TEST(check_validity(H5Iis_valid(task_edges->active_dataset->get_C_id()),
                              "edges") == true);

    // close the file again
    model.file.close();
}

//     this test case checks the content of the file written in the last test
BOOST_AUTO_TEST_CASE(writetaskfactory_basic_read,
                     *boost::unit_test::tolerance(1e-16))
{

    
    HDFFile file("writetaskfactory_testmodel.h5", "r");
    auto    basic_group = file.open_group("/basic");
    auto    task2_dset  = basic_group->open_dataset("cell_dset");

    HDFAttribute basic_group_attr(*basic_group, "group_attribute");

    auto [basic_group_attrshape, basic_group_attr_data] =
        basic_group_attr.read< std::string >();

    BOOST_TEST(basic_group_attrshape.size() == 1); // this is to remove
    // unused variable warning while retaing structured bindings, also in the
    // following
    BOOST_TEST(basic_group_attr_data == "this contains celldata");

    auto task_dset = basic_group->open_dataset("agent_dset");

    // check dataset attributes
    // basic group contained datasets have no attributes
    HDFAttribute basic_dataset_attr(*task2_dset, "dataset_attribute");
    auto [basic_dset_attrshape, basic_dset_attrdata] =
        basic_dataset_attr.read< std::string >();

    BOOST_TEST(basic_dset_attrdata == "this saves the cell's x coordinate");
    BOOST_TEST(basic_dset_attrshape.size() == 1); // this is to remove

    // check data content
    auto [task_shape, task_data] = task_dset->read< std::vector< double > >();
    BOOST_TEST(task_shape == std::vector< hsize_t >{ 256 },
               boost::test_tools::per_element());
    BOOST_TEST(task_data == std::vector< double >(256, 24.314),
               boost::test_tools::per_element());

    auto [task2_shape, task2_data] = task2_dset->read< std::vector< int > >();
    BOOST_TEST(task2_shape == std::vector< hsize_t >{ 1024 },
               boost::test_tools::per_element());
    BOOST_TEST(task2_data == std::vector< int >(1024, 3),
               boost::test_tools::per_element());

    // test the graph output
    HDFFile graphfile("graphmodel.h5", "r");
    auto    graphgroup = graphfile.open_group("/graph");

    // make attribute and read
    HDFAttribute graphgroup_attr(*graphgroup, "graph_group_attribute");
    auto [ag_shape, ag_data] = graphgroup_attr.read< std::string >();

    BOOST_TEST(ag_shape == std::vector< hsize_t >{ 1 });
    BOOST_TEST(ag_data == "this contains graphdata");

    auto vertex_dset = graphgroup->open_dataset("vertex_property_dataset");
    HDFAttribute vertex_attr(*vertex_dset, "dataset_attribute");

    auto [vertex_attr_shape, vertex_attr_data] =
        vertex_attr.read< std::string >();
    BOOST_TEST(vertex_attr_shape == std::vector< hsize_t >{ 1 });
    BOOST_TEST(vertex_attr_data == "this saves vertex indices or so");

    auto [vertex_shape, vertex_data] =
        vertex_dset->read< std::vector< std::size_t > >();
    std::vector< std::size_t > vertex_data_expected(1024);
    std::iota(vertex_data_expected.begin(), vertex_data_expected.end(), 0);

    BOOST_TEST(vertex_shape == std::vector< hsize_t >{ 1024 });
    BOOST_TEST(vertex_data_expected == vertex_data,
               boost::test_tools::per_element());

    auto edge_property_dset = graphgroup->open_dataset("edge_property_dataset");
    HDFAttribute edge_property_dset_attr(*edge_property_dset,
                                         "dataset_attribute");

    auto [edge_source_attr_shape, edge_source_attr_data] =
        edge_property_dset_attr.read< std::string >();
    BOOST_TEST(edge_source_attr_shape == std::vector< hsize_t >{ 1 });
    BOOST_TEST(edge_source_attr_data == "this saves edges or so");

    auto [edge_source_shape, edge_source_data] =
        edge_property_dset->read< std::vector< std::size_t > >();

    std::vector< std::size_t > edge_data;
    edge_data.reserve(2 * 4096);

    // re-build the graph from before
    Graph_vertvecS_edgevecS_undir g =
        create_and_initialize_test_graph< Graph_vertvecS_edgevecS_undir >(1024,
                                                                          4096);

    typename Graph_vertvecS_edgevecS_undir::edge_iterator e, ed;
    boost::tie(e, ed) = boost::edges(g);

    std::transform(e, ed, std::back_inserter(edge_data), [&g](auto&& edg) {
        return boost::get(boost::vertex_index_t(), g, boost::source(edg, g));
    });
    std::transform(e, ed, std::back_inserter(edge_data), [&g](auto&& edg) {
        return boost::get(boost::vertex_index_t(), g, boost::target(edg, g));
    });

    BOOST_TEST(edge_source_shape == (std::vector< hsize_t >{ 2, 4096 }));
    BOOST_TEST(edge_source_data == edge_data, boost::test_tools::per_element());

    // pure vertex writer
    auto         struct_group        = graphfile.open_group("/graph_structure");
    auto         pure_vertex_dataset = struct_group->open_dataset("vertices");
    HDFAttribute struct_group_attr(*struct_group, "structure_attr");
    auto [structure_attr_shape, structure_attr_data] =
        struct_group_attr.read< std::string >();
    BOOST_TEST(structure_attr_shape == (std::vector< hsize_t >{ 1 }));
    BOOST_TEST(structure_attr_data == "this group contains graph structure");

    auto struct_vertex_dset = struct_group->open_dataset("vertices");
    auto [struct_vertex_shape, struct_vertex_data] =
        struct_vertex_dset->read< std::vector< std::size_t > >();

    BOOST_TEST(struct_vertex_shape == (std::vector< hsize_t >{ 1024 }));
    BOOST_TEST(struct_vertex_data == vertex_data_expected,
               boost::test_tools::per_element());

    auto struct_edges_dset = struct_group->open_dataset("edges");
    auto [struct_edges_shape, struct_edges_data] =
        struct_edges_dset->read< std::vector< std::size_t > >();

    BOOST_TEST(struct_edges_shape == (std::vector< hsize_t >{ 2, 4096 }));
    BOOST_TEST(struct_edges_data == edge_data,
               boost::test_tools::per_element());
}

// test datamanager integration
BOOST_AUTO_TEST_CASE(writetaskfactory_datamanager_integration)
{
    // std::this_thread::sleep_for(15s);
    
    Model model("writetaskfactory_testmodel_integration",
                "datamanager_test_factory.yml",
                128,
                32,
                { 4, 6, 6.28 },
                { 15, 34.314 });

    TaskFactory< Model > factory;

    // make a map of tasks which are build with the taskfactory
    auto taskmap = std::unordered_map<
        std::string,
        std::shared_ptr< Default::DefaultWriteTask< Model > > >{

        factory(
            "adaption"s,
            "/Agents",
            { "adaption$time" },

            [](auto& model) -> decltype(auto) {
                return model.get_agentmanager().agents();
            },
            [](auto&& agent) -> decltype(auto) {
                return agent->state()._adaption;
            },
            std::tuple{ "Description"s, "This contains agent highresdata"s }),
        factory(
            "age"s,
            "/Agents",
            { "age$time" },
            [](auto& model) -> decltype(auto) {
                return model.get_agentmanager().agents();
            },
            [](auto&& agent) -> decltype(auto) { return agent->state()._age; }),
        factory(
            "coords"s,
            "/Cells",
            { "coordinates$time" },

            [](auto& model) -> decltype(auto) {
                return model.get_cellmanager().cells();
            },
            [](auto&& cell) -> decltype(auto) {
                return std::array< int, 2 >{ cell->state()._x,
                                             cell->state()._y };
            },
            std::tuple{ "Description"s, "This contains cell highresdata"s }),
        factory(
            "resources"s,
            "/Cells",
            { "resources$time" },

            [](auto& model) -> decltype(auto) {
                return model.get_cellmanager().cells();
            },
            [](auto&& cell) -> decltype(auto) { return cell->state()._res; })

    };

    // make a datamanager which uses a config file

    auto cfg = YAML::LoadFile("datamanager_test_factory.yml");

    using DMT = DataManagerTraits< Default::DefaultWriteTask< Model >,
                                   Default::DefaultDecider< Model >,
                                   Default::DefaultTrigger< Model >,
                                   Default::DefaultExecutionProcess >;

    Utopia::DataIO::DataManager< DMT > dm(
        cfg["data_manager"],
        // tasks
        taskmap,
        // rest
        Default::default_decidertypes< Model >,
        Default::default_triggertypes< Model >,
        Default::DefaultExecutionProcess());

    // execute the datamanager execution process
    for (model.time = 0; model.time < 200; ++model.time)
    {
        for (auto& agent : model.get_agentmanager().agents())
        {
            agent->state()._age = model.time;
        }

        dm(model);
    }
}

BOOST_AUTO_TEST_CASE(writetaskfactory_datamanager_integration_read_result,
                     *boost::unit_test::tolerance(1e-16))
{
    
    HDFFile file("writetaskfactory_testmodel_integration.h5", "r");

    auto agentgroup = file.open_group("/Agents");
    auto cellgroup  = file.open_group("/Cells");

    for (std::size_t i = 0; i < 100; ++i)
    {
        // open agent adaption dataset and read in data
        auto [adaption_shape, adaption_data] =
            agentgroup->open_dataset("adaption_" + std::to_string(i))
                ->read< std::vector< double > >();

        BOOST_TEST(adaption_shape == (std::vector< hsize_t >{ 32 }),
                   boost::test_tools::per_element());

        BOOST_TEST(adaption_data == (std::vector< double >(32, 34.314)),
                   boost::test_tools::per_element());

        // open agent age dataset and read in data
        auto [age_shape, age_data] =
            agentgroup->open_dataset("age_" + std::to_string(i))
                ->read< std::vector< int > >();

        BOOST_TEST(age_shape == (std::vector< hsize_t >{ 32 }),
                   boost::test_tools::per_element());

        BOOST_TEST(age_data == (std::vector< int >(32, static_cast< int >(i))),
                   boost::test_tools::per_element());

        // open cell resouce dataset and read in data
        auto [cellresource_shape, cellresource_data] =
            cellgroup->open_dataset("resources_" + std::to_string(i))
                ->read< std::vector< double > >();

        BOOST_TEST(cellresource_shape == (std::vector< hsize_t >{ 128 }),
                   boost::test_tools::per_element());

        BOOST_TEST(cellresource_data == (std::vector< double >(128, 6.28)),
                   boost::test_tools::per_element());

        // open cell coordinates dataset and read in data
        auto [cellcoords_shape, cellcoords_data] =
            cellgroup->open_dataset("coordinates_" + std::to_string(i))
                ->read< std::vector< std::array< int, 2 > > >();

        BOOST_TEST(cellcoords_shape == (std::vector< hsize_t >{ 128 }),
                   boost::test_tools::per_element());

        BOOST_TEST(cellcoords_data == (std::vector< std::array< int, 2 > >(
                                          128, std::array< int, 2 >{ 4, 6 })),
                   boost::test_tools::per_element());
    }

    file.close();
}

BOOST_AUTO_TEST_CASE(datamanager_factory_test)
{
    Model model("datamanagerfactory_testmodel_integration",
                "datamanager_test_factory.yml",
                128,
                32,
                { 4, 6, 6.28 },
                { 15, 34.314 });

    const auto args = std::make_tuple(
        // first way: simplified arguments
        std::make_tuple(
            "adaption",
            [](auto& model) -> decltype(auto) {
                return model.get_agentmanager().agents();
            },
            [](auto&& agent) -> decltype(auto) {
                std::cout << "adaption: " << agent->state()._adaption
                          << std::endl;
                return agent->state()._adaption;
            },
            std::make_tuple("Content", "This contains agent highres data"),
            std::make_tuple("Content", "This contains adaption data")),
        std::make_tuple(
            "age",
            [](auto& model) -> decltype(auto) {
                return model.get_agentmanager().agents();
            },
            [](auto& agent) -> decltype(auto) {
                std::cout << "age: " << agent->state()._age << std::endl;

                return agent->state()._age;
            },
            Nothing{},
            std::make_tuple("content", "This contains age data"))
        // // second way: direct lambdas
        ,
        std::make_tuple(
            "coords",
            // groupbuilder
            [](auto&& group) { return group->open_group("Cells"); },
            // writer
            [](auto& dataset, auto& model) {
                dataset->write(model.get_cellmanager().cells().begin(),
                               model.get_cellmanager().cells().end(),
                               [](auto&& cell) -> decltype(auto) {
                                   return std::array< int, 2 >{
                                       cell->state()._x, cell->state()._y
                                   };
                               });
            },
            // dataset builder
            [](auto& group, auto& model) -> decltype(auto) {
                return group->open_dataset("coordinates_" +
                                           std::to_string(model.get_time()));
            },
            // group attribute builder
            [](auto& group, auto&) {
                group->add_attribute("content", "This contains cell data");
            },
            // dataset attribute builder
            [](auto& dataset, auto&) {
                dataset->add_attribute("content",
                                       "This contains cell coordinates");
            }),
        std::make_tuple(
            "resources",
            // groupbuilder
            [](auto&& group) -> decltype(auto) {
                return group->open_group("Cells");
            },
            // writer
            [](auto& dataset, auto& model) {
                dataset->write(model.get_cellmanager().cells().begin(),
                               model.get_cellmanager().cells().end(),
                               [](auto&& cell) -> decltype(auto) {
                                   return cell->state()._res;
                               });
            },
            // dataset builder
            [](auto& group, auto& model) -> decltype(auto) {
                return group->open_dataset("resources_" +
                                           std::to_string(model.get_time()));
            },
            // group attribute builder
            [](auto& group, auto&) {
                group->add_attribute("content", "This contains cell data");
            },
            // dataset attribute builder
            [](auto& dataset, auto&) {
                dataset->add_attribute("content",
                                       "This contains cell resources");
            }));

    auto dm =
        DataManagerFactory< Model >()(model.get_cfg()["data_manager"], args);

    for (auto&& [name, decider] : dm.get_deciders())
    {
        std::cout << name << ", " << decider << std::endl;
    }
    std::cout << std::endl;

    for (auto&& [name, trigger] : dm.get_triggers())
    {
        std::cout << name << ", " << trigger << std::endl;
    }
    std::cout << std::endl;

    for (auto&& [name, task] : dm.get_tasks())
    {
        std::cout << name << ", " << task << std::endl;
    }
    std::cout << std::endl;

    for (auto&& [decidername, tasknames] : dm.get_decider_task_map())
    {
        std::cout << decidername << ", " << tasknames << std::endl;
    }
    std::cout << std::endl;

    for (auto&& [triggername, tasknames] : dm.get_trigger_task_map())
    {
        std::cout << triggername << ", " << tasknames << std::endl;
    }
    std::cout << std::endl;

    for (model.time = 0; model.time < 200; ++model.time)
    {
        for (auto& agent : model.get_agentmanager().agents())
        {
            agent->state()._age = model.time;
        }

        dm(model);
    }
}

BOOST_AUTO_TEST_CASE(datamanager_factory_test_read)
{
    
    HDFFile file("datamanagerfactory_testmodel_integration.h5", "r");

    auto agentgroup = file.open_group("/Agents");
    auto cellgroup  = file.open_group("/Cells");

    for (std::size_t i = 0; i < 100; ++i)
    {
        // open agent adaption dataset and read in data
        auto [adaption_shape, adaption_data] =
            agentgroup->open_dataset("adaption_" + std::to_string(i))
                ->read< std::vector< double > >();

        BOOST_TEST(adaption_shape == (std::vector< hsize_t >{ 32 }),
                   boost::test_tools::per_element());

        BOOST_TEST(adaption_data == (std::vector< double >(32, 34.314)),
                   boost::test_tools::per_element());

        // open agent age dataset and read in data
        auto [age_shape, age_data] =
            agentgroup->open_dataset("age_" + std::to_string(i))
                ->read< std::vector< int > >();

        BOOST_TEST(age_shape == (std::vector< hsize_t >{ 32 }),
                   boost::test_tools::per_element());

        BOOST_TEST(age_data == (std::vector< int >(32, static_cast< int >(i))),
                   boost::test_tools::per_element());

        // open cell resouce dataset and read in data
        auto [cellresource_shape, cellresource_data] =
            cellgroup->open_dataset("resources_" + std::to_string(i))
                ->read< std::vector< double > >();

        BOOST_TEST(cellresource_shape == (std::vector< hsize_t >{ 128 }),
                   boost::test_tools::per_element());

        BOOST_TEST(cellresource_data == (std::vector< double >(128, 6.28)),
                   boost::test_tools::per_element());

        // open cell coordinates dataset and read in data
        auto [cellcoords_shape, cellcoords_data] =
            cellgroup->open_dataset("coordinates_" + std::to_string(i))
                ->read< std::vector< std::array< int, 2 > > >();

        BOOST_TEST(cellcoords_shape == (std::vector< hsize_t >{ 128 }),
                   boost::test_tools::per_element());

        BOOST_TEST(cellcoords_data == (std::vector< std::array< int, 2 > >(
                                          128, std::array< int, 2 >{ 4, 6 })),
                   boost::test_tools::per_element());
    }
}
BOOST_AUTO_TEST_SUITE_END()
