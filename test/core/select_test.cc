#define BOOST_TEST_MODULE select test

#include <boost/test/unit_test.hpp>

#include <utopia/core/select.hh>
#include <utopia/data_io/cfg_utils.hh>

#include "agent_manager_test.hh"
#include "cell_manager_test.hh"

// -- Types -------------------------------------------------------------------

using namespace Utopia;
using Utopia::DataIO::Config;
using Utopia::SelectionMode;
using Utopia::Update;

template<class T>
using CMMockModel = Utopia::Test::CellManager::MockModel<T>;

template<class T>
using AMMockModel = Utopia::Test::AgentManager::MockModel<T>;

using TestCellTraits = Utopia::CellTraits<int, Utopia::Update::manual, true>;
using TestAgentTraits = Utopia::AgentTraits<int, Update::sync, true>;

// -- Fixtures ----------------------------------------------------------------

struct ModelFixture {
    Config cfg;
    CMMockModel<TestCellTraits> mm_cm; // A model with a cell manager
    AMMockModel<TestCellTraits> mm_am; // A model with an agent manager

    ModelFixture ()
    :
        cfg(YAML::LoadFile("select_test.yml")),
        mm_cm("mm_cm", cfg["models"]["with_cm"]),
        mm_am("mm_am", cfg["models"]["with_am"])
    {}
};

// -- Basic tests -------------------------------------------------------------

// Standalone functions should work with both AgentManager and CellManager
BOOST_FIXTURE_TEST_CASE(interface, ModelFixture)
{
    auto& cm = mm_cm._cm;
    auto& am = mm_am._am;

    // Now, just try out some selection calls via the free functions ...

    auto c1 = select_entities<SelectionMode::sample>(cm, 23);
    auto a1 = select_entities<SelectionMode::sample>(am, 23);
    BOOST_TEST(c1.size() == a1.size());

    select_entities<SelectionMode::probability>(cm, .1);
    select_entities<SelectionMode::probability>(am, .1);

    auto always_true = [](const auto&){ return true; };
    
    auto c3 = select_entities<SelectionMode::condition>(cm, always_true);
    BOOST_TEST(c3.size() == cm.entities().size());
    
    auto a3 = select_entities<SelectionMode::condition>(am, always_true);
    BOOST_TEST(a3.size() == am.entities().size());
}

// -- Selection Mode Tests (on AgentManager) ----------------------------------
// -- Selection mode: sample
BOOST_FIXTURE_TEST_CASE(am_sample, ModelFixture)
{
    auto& am = mm_am._am;

    auto a1 = am.select_agents<SelectionMode::sample>(42);
    auto a2 = am.select_agents(cfg["sample"]);
    BOOST_TEST(a1.size() == 42);
    BOOST_TEST(a2.size() == 42);
    BOOST_TEST(a1 == a2);  // because there are exactly 42 agents

    BOOST_TEST(am.select_agents<SelectionMode::sample>(1).size() == 1);
}


// -- Selection Mode Tests (on CellManager) -----------------------------------

// -- Selection mode: sample
BOOST_FIXTURE_TEST_CASE(cm_sample, ModelFixture)
{
    auto& cm = mm_cm._cm;

    auto c1 = cm.select_cells<SelectionMode::sample>(42);
    auto c2 = cm.select_cells(cfg["sample"]);
    BOOST_TEST(c1.size() == 42);
    BOOST_TEST(c2.size() == 42);
    BOOST_TEST(c1 != c2);  // very unlikely with 42*42 agents

    BOOST_TEST(cm.select_cells<SelectionMode::sample>(1).size() == 1);
}

// -- Selection mode: probability
BOOST_FIXTURE_TEST_CASE(cm_probability, ModelFixture)
{
    auto& cm = mm_cm._cm;

    cm.select_cells<SelectionMode::probability>(.5);
    cm.select_cells(cfg["probability"]);
    BOOST_TEST(   cm.select_cells<SelectionMode::probability>(0.).size()
               == 0);
    BOOST_TEST(   cm.select_cells<SelectionMode::probability>(1.).size()
               == cm.cells().size());
}

// -- Selection mode: boundary
BOOST_FIXTURE_TEST_CASE(cm_boundary, ModelFixture)
{
    auto& cm = mm_cm._cm;

    auto c1 = cm.select_cells<SelectionMode::boundary>("bottom");
    auto c2 = cm.select_cells(cfg["boundary"]);
    auto c3 = cm.boundary_cells("bottom");

    BOOST_TEST(c3.size() == cm.grid()->shape()[0]);
    BOOST_TEST(c1 == c2);
    BOOST_TEST(c1 == c3);
    BOOST_TEST(c2 == c3);
}

// -- Selection mode: clustered_simple
BOOST_FIXTURE_TEST_CASE(cm_clustered_simple, ModelFixture)
{
    auto& cm = mm_cm._cm;

    // FIXME Need a neighborhood for this but should be independent!
    cm.select_neighborhood("vonNeumann", true);

    auto c1 = cm.select_cells<SelectionMode::clustered_simple>(.01, .2, 1);
    auto c2 = cm.select_cells(cfg["clustered"]);

    BOOST_TEST(c1.size() > 0);
    BOOST_TEST(c2.size() > 0);
    BOOST_TEST(c1 != c2);
    BOOST_TEST(c2.size() > c1.size()); // many more passes in 2

    // TODO ... check cluster sizes
}
