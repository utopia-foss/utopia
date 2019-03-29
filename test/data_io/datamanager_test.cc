#define BOOST_TEST_MODULE datamanager_test
#include <boost/mpl/list.hpp>       // type lists for testing
#include <boost/test/unit_test.hpp> // for unit tests

#include <sstream> // needed for testing throw messages from exceptions
#include <utopia/core/utils.hh> // for output operators, get size...
#include <utopia/data_io/data_manager.hh>
#include <utopia/data_io/hdfdataset.hh> // for messing around with datasets
#include <utopia/data_io/hdffile.hh>    // for buiding model mock class

namespace utf = boost::unit_test;
using namespace std::literals;
using namespace Utopia::DataIO;

/**
 * @brief Mocking class for the model
 *
 */
struct Model
{
    std::string name;
    Utopia::DataIO::HDFFile file;
    std::shared_ptr<spdlog::logger> logger;

    auto get_logger()
    {
        return logger;
    }

    Model(std::string n)
        : name(n), file(n + ".h5", "w"), logger(spdlog::stdout_color_mt("logger." + n))
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

// less messy type aliases
using Writer =
    std::function<void(Model&, Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>&)>;
using Builder =
    std::function<Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>(Model&, Utopia::DataIO::HDFGroup&)>;
using Simpletask = Task<Builder, Writer>;

using Decider = std::function<bool(Model&)>;
using Trigger = std::function<bool(Model&)>;

// this tests the constructor which builts a new
BOOST_AUTO_TEST_CASE(datamanager_tuplelike_constructor)
{
    using namespace Utopia::Utils; // enable output operators defined in Utils by default without qualifiers

    // needed for all the structors
    Model model("fixture_1");

    DataManager dm(
        model,
        // tasks
        std::array<std::pair<std::string, Simpletask>, 2>{
            std::make_pair(
                "t1"s,
                Simpletask(
                    [](Model& m, Utopia::DataIO::HDFGroup& g)
                        -> Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup> {
                        return *g.open_dataset("/" + m.name + "_1");
                    },
                    [](Model&, Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>& d) -> void {
                        d.write(std::vector<int>{1, 2, 3});
                    },
                    *model.file.open_group("/t1"))),
            std::make_pair(
                "t2"s,
                Simpletask(
                    [](Model& m, Utopia::DataIO::HDFGroup& g)
                        -> Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup> {
                        return *g.open_dataset("/" + m.name + "_2");
                    },
                    [](Model&, Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>& d) -> void {
                        d.write(std::vector<int>{4, 5, 6});
                    },
                    *model.file.open_group("/t2")))},
        // deciders
        std::array<std::pair<std::string, Decider>, 2>{
            std::make_pair("d1"s, [](Model&) -> bool { return true; }),
            std::make_pair("d2"s, [](Model&) -> bool { return false; })},
        // triggers
        std::array<std::pair<std::string, Trigger>, 2>{
            std::make_pair("b1"s, [](Model&) -> bool { return true; }),
            std::make_pair("b2"s, [](Model&) -> bool { return false; })}
        );

    // test correct associations
    BOOST_TEST(dm.get_decider_task_map() ==
               (std::unordered_map<std::string, std::vector<std::string>>{
                   {"d1", std::vector<std::string>{"t1"}},
                   {"d2", std::vector<std::string>{"t2"}}}));

    BOOST_TEST(dm.get_trigger_task_map() ==
               (std::unordered_map<std::string, std::vector<std::string>>{
                   {"b1", std::vector<std::string>{"t1"}},
                   {"b2", std::vector<std::string>{"t2"}}}));

    // try to build a datamanager without explicit associations but with unequal length arrays
    // fails with an exception complaining about unequal sizes and ambiguous associations

    // use ostream to get exception message
    std::ostringstream s;

    try
    {
        DataManager dm2(
            model,
            // tasks
            std::array<std::pair<std::string, Simpletask>, 1>{std::make_pair(
                "t1_2"s,
                Simpletask(
                    [](Model& m, Utopia::DataIO::HDFGroup& g)
                        -> Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup> {
                        return *g.open_dataset("/" + m.name + "_1_2");
                    },
                    [](Model&, Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>& d) -> void {
                        d.write(std::vector<int>{1, 2, 3});
                    },
                    *model.file.open_group("/t1_2")))},
            // deciders
            std::array<std::pair<std::string, Decider>, 1>{
                std::make_pair("d1_2"s, [](Model&) -> bool { return true; })},
            // triggers -> this will cause an error
            std::array<std::pair<std::string, Trigger>, 2>{
                std::make_pair("b1_2"s, [](Model&) -> bool { return true; }),
                std::make_pair("b2_2"s, [](Model&) -> bool { return false; })}
            );
    }
    catch (std::exception& e)
    {
        s << e.what();
    }

    // check that the error message is correct.
    BOOST_TEST(s.str() ==
        "triggers size != tasks size! You have to disambiguate "
        "the association of triggers and write tasks by "
        "supplying an explicit task_trigger_assocs argument if you want to "
        "have an unequal number of tasks and triggers.");

    // build a datamanager with explicit associations.
    // Additionally mix arrays and tuples
    DataManager dm3(
        model,
        // tasks
        std::make_tuple(
            std::make_pair(
                "t1_3"s,
                Simpletask(
                    [](Model& m, Utopia::DataIO::HDFGroup& g)
                        -> Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup> {
                        return *g.open_dataset("/" + m.name + "_1_3");
                    },
                    [](Model&, Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>& d) -> void {
                        d.write(std::vector<int>{1, 2, 3});
                    },
                    *model.file.open_group("/t1_3"))
            ),
            std::make_pair(
                "t2_3"s,
                Simpletask(
                    [](Model& m, Utopia::DataIO::HDFGroup& g)
                        -> Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup> {
                        return *g.open_dataset("/" + m.name + "_2_3");
                    },
                    [](Model&, Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>& d) -> void {
                        d.write(std::vector<int>{4, 5, 6});
                    },
                    *model.file.open_group("/t2_3"))
            ),
            std::make_pair(
                "t3_3"s,
                Simpletask(
                    [](Model& m, Utopia::DataIO::HDFGroup& g)
                        -> Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup> {
                        return *g.open_dataset("/" + m.name + "_2_3");
                    },
                    [](Model&, Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>& d) -> void {
                        d.write(std::vector<int>{4, 5, 6});
                    },
                    *model.file.open_group("/t3"))
            )
        ),
        // deciders
        std::array<std::pair<std::string, Decider>, 1>{
            std::make_pair("d1_3"s, [](Model&) -> bool { return true; })
        },
        // triggers
        std::array<std::pair<std::string, Trigger>, 2>{
            std::make_pair("b1_3"s, [](Model&) -> bool { return true; }),
            std::make_pair("b2_3"s, [](Model&) -> bool { return false; })
        },
        // task -> decider associations
        std::vector<std::pair<std::string, std::string>>{
            {"t1_3", "d1_3"}, {"t2_3", "d1_3"}, {"t3_3", "d1_3"}
        },
        // task -> trigger associations
        std::vector<std::pair<std::string, std::string>>{
            {"t1_3", "b1_3"}, {"t2_3", "b1_3"}, {"t3_3", "b2_3"}
        });

    // again check that the associations are correct
    BOOST_TEST(dm3.get_decider_task_map() ==
               (std::unordered_map<std::string, std::vector<std::string>>{
                   {"d1_3", {"t1_3", "t2_3", "t3_3"}}}));

    BOOST_TEST(dm3.get_trigger_task_map() ==
               (std::unordered_map<std::string, std::vector<std::string>>{
                   {"b1_3", {"t1_3", "t2_3"}}, {"b2_3", {"t3_3"}}}));
}

BOOST_AUTO_TEST_CASE(datamanager_lifecycle)
{
    // needed for all the structors
    Model model("fixture_3");

    // datamanager to use for testing copy, move etc
    DataManager dm(
        model,
        // tasks
        std::make_tuple(
            std::make_pair(
                "v1"s,
                Simpletask(
                    [](Model& m, Utopia::DataIO::HDFGroup& g)
                        -> Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup> {
                        return *g.open_dataset("/" + m.name + "_1");
                    },
                    [](Model&, Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>& d) -> void {
                        d.write(std::vector<int>{1, 2, 3});
                    },
                    *model.file.open_group("/t1"))),
            std::make_pair(
                "v2"s,
                Simpletask(
                    [](Model& m, Utopia::DataIO::HDFGroup& g)
                        -> Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup> {
                        return *g.open_dataset("/" + m.name + "_2");
                    },
                    [](Model&, Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>& d) -> void {
                        d.write(std::vector<int>{4, 5, 6});
                    },
                    *model.file.open_group("/t2")))
        ),
        // deciders
        std::make_tuple(
            std::make_pair("w1"s, [](Model&) -> bool { return true; }),
            std::make_pair("w2"s, [](Model&) -> bool { return false; })
        ),
        // triggers
        std::make_tuple(
            std::make_pair("k1"s, [](Model&) -> bool { return true; }),
            std::make_pair("k2"s, [](Model&) -> bool { return false; })
        )
        );

    // have copy to check against later
    auto dm_cpy(dm);

    // swap
    // datamanager to use for testing copy, move etc
    DataManager dm2(
        model,
        // tasks
        std::make_tuple(
            std::make_pair(
                "t1"s,
                Simpletask(
                    [](Model& m, Utopia::DataIO::HDFGroup& g) -> Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup> {
                        return *g.open_dataset("/" + m.name + "_1");
                    },
                    [](Model&, Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>& d) -> void {
                        d.write(std::vector<int>{1, 2, 3});
                    },
                    *model.file.open_group("/t1")))
        ),
        // deciders
        std::make_tuple(
            std::make_pair("d1"s, [](Model&) -> bool { return true; })
        ),
        // triggers
        std::make_tuple(
            std::make_pair("b1"s, [](Model&) -> bool { return true; })
        ));

    // have copy to check against
    auto dm2_cpy(dm2);

    // swap(dm2, dm);

    // check that the states are swapped
    // BOOST_TEST(dm.get_triggers() == dm2_cpy.get_triggers());
    // BOOST_TEST(dm.get_tasks() == dm2_cpy.get_tasks());
    // BOOST_TEST(dm.get_deciders() == dm2_cpy.get_deciders());
    // BOOST_TEST(dm.get_logger() == dm2_cpy.get_logger());
    // BOOST_TEST(dm.get_trigger_task_map() == dm2_cpy.get_trigger_task_map());
    // BOOST_TEST(dm.get_decider_task_map() == dm2_cpy.get_decider_task_map());

    // BOOST_TEST(dm2.get_triggers() == dm_cpy.get_triggers());
    // BOOST_TEST(dm2.get_tasks() == dm_cpy.get_tasks());
    // BOOST_TEST(dm2.get_deciders() == dm_cpy.get_deciders());
    // BOOST_TEST(dm2.get_logger() == dm_cpy.get_logger());
    // BOOST_TEST(dm2.get_trigger_task_map() == dm_cpy.get_trigger_task_map());
    // BOOST_TEST(dm2.get_decider_task_map() == dm_cpy.get_decider_task_map());
}

// test polymorphism for tasks in datamanager
// The principle is the same for deciders and triggers, and since they
// are stored in the same way in the same datastructures,
// there polymorphism capabilities are not additionally tested.
BOOST_AUTO_TEST_CASE(datamanager_polymorphism)
{
    Model model("fixture_4");

    // datamanager to use for testing copy, move etc
    DataManager dm(
        model,
        // tasks
        std::make_tuple(std::make_pair("basic"s, BasicTask()),
                        std::make_pair("derived"s, DerivedTask())),
        // deciders
        std::make_tuple(
            std::make_pair("w1"s, [](Model&) -> bool { return true; }),
            std::make_pair("w2"s, [](Model&) -> bool { return false; })),
        // triggers
        std::make_tuple(
            std::make_pair("k1"s, [](Model&) -> bool { return true; }),
            std::make_pair("k2"s, [](Model&) -> bool { return false; }))
        );

    // execute tasks
    auto tsks = dm.get_tasks();
    for (auto& pair : tsks)
    {
        pair.second->write();
    }

    // check that the respective 'write' operations where called correctly for
    // the registered tasks
    BOOST_TEST(tsks["basic"]->str == "base");
    BOOST_TEST(tsks["derived"]->str == "derived");
}

BOOST_AUTO_TEST_CASE(datamanager_customize_association)
{
    Model model("fixture_5");

    // datamanager to use for testing copy, move etc
    DataManager dm(
        model,
        // tasks
        std::make_tuple(
            std::make_pair(
                "v1"s,
                Simpletask(
                    [](Model& m, Utopia::DataIO::HDFGroup& g)
                        -> Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup> {
                        return *g.open_dataset("/" + m.name + "_1");
                    },
                    [](Model&, Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>& d) -> void {
                        d.write(std::vector<int>{1, 2, 3});
                    },
                    *model.file.open_group("/t1"))),
            std::make_pair(
                "v2"s,
                Simpletask(
                    [](Model& m, Utopia::DataIO::HDFGroup& g)
                        -> Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup> {
                        return *g.open_dataset("/" + m.name + "_2");
                    },
                    [](Model&, Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>& d) -> void {
                        d.write(std::vector<int>{4, 5, 6});
                    },
                    *model.file.open_group("/t2")))
        ),
        // deciders
        std::make_tuple(
            std::make_pair("w1"s, [](Model&) -> bool { return true; }),
            std::make_pair("w2"s, [](Model&) -> bool { return false; })
        ),
        // triggers
        std::make_tuple(
            std::make_pair("k1"s, [](Model&) -> bool { return true; }),
            std::make_pair("k2"s, [](Model&) -> bool { return false; })
        ));

    // try to register the new task
    dm.register_task( "v3", Simpletask(
        [](Model& m, Utopia::DataIO::HDFGroup& g)
            -> Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>
        {
            return *g.open_dataset("/" + m.name + "_3");
        },
        [](Model&, Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>& d) -> void
        {
            d.write(std::vector<int>{4, 5, 6, 8, 0, 10});
        },
        *model.file.open_group("/t3"))
    );

    BOOST_TEST(dm.get_tasks().size() == 3);

    // link the new task to a decider
    dm.link_task_to_decider("v3", "w1");
    BOOST_TEST(dm.get_decider_task_map().at("w1") == (std::vector<std::string>{"v1", "v3"}));

    // link the new task to a trigger
    dm.link_task_to_trigger("v3", "k2");
    BOOST_TEST(dm.get_trigger_task_map().at("k2") == (std::vector<std::string>{"v2", "v3"}));

    // register new decider
    dm.register_decider("w3", Decider([](Model&) -> bool { return true; }));
    BOOST_TEST(dm.get_deciders().size() == 3);

    // register new trigger
    dm.register_trigger("k3", Trigger([](Model&) -> bool { return true; }));
    BOOST_TEST(dm.get_triggers().size() == 3);

    // relink v2 to new decider w3 from w2
    
    dm.link_task_to_decider("v2", "w3", "w2");
    //-----------------------^ new taskname,
    //-----------------------------^ new decidername,
    // ------------------------------------^ old decidername

    BOOST_TEST(dm.get_decider_task_map().at("w2") == (std::vector<std::string>{}));
    BOOST_TEST(dm.get_decider_task_map().at("w3") ==
               (std::vector<std::string>{"v2"}));

    // relink v2 to new trigger k3 from k2
   
    dm.link_task_to_trigger("v2", "k3", "k2");

    BOOST_TEST(dm.get_trigger_task_map().at("k2") == (std::vector<std::string>{"v3"}));
    BOOST_TEST(dm.get_trigger_task_map().at("k3") ==
               (std::vector<std::string>{"v2"}));

    // register a new procedure
    dm.register_procedure(
        "new_task", Simpletask(
                  [](Model& m, Utopia::DataIO::HDFGroup& g)
                      -> Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup> {
                      return *g.open_dataset("/" + m.name + "_new");
                  },
                  [](Model&, Utopia::DataIO::HDFDataset<Utopia::DataIO::HDFGroup>& d) -> void {
                      d.write(std::vector<int>{4, 5, 6, 8, 0, 10});
                  },
                  *model.file.open_group("/t_new")),
        "new_decider", Decider([](Model& ){ return true;}), 
        "new_trigger", Trigger([](Model& ){ return true;})
    );

    BOOST_TEST(dm.get_deciders().size()== 4);
    BOOST_TEST(dm.get_tasks().size() == 4);
    BOOST_TEST(dm.get_triggers().size() == 4);
    BOOST_TEST(dm.get_decider_task_map().at("new_decider") == (std::vector<std::string>{"new_task"}));
    BOOST_TEST(dm.get_trigger_task_map().at("new_trigger") ==
               (std::vector<std::string>{"new_task"}));
}
