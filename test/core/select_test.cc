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
using SpaceVec = Utopia::SpaceVecType<2>;
using MultiIndex = Utopia::MultiIndexType<2>;

template<class T>
using CMMockModel = Utopia::Test::CellManager::MockModel<T>;

template<class T>
using AMMockModel = Utopia::Test::AgentManager::MockModel<T>;

using TestCellTraits = Utopia::CellTraits<int, Utopia::Update::manual, true>;
using TestAgentTraits = Utopia::AgentTraits<int, Update::sync, true>;

// -- Fixtures ----------------------------------------------------------------

struct ModelFixture {
    Config cfg;
    CMMockModel<TestCellTraits> mm_cm;    // A model with a cell manager
    CMMockModel<TestCellTraits> mm_cm_np; // CellManager, nonperiodic space
    AMMockModel<TestCellTraits> mm_am;    // A model with an agent manager

    ModelFixture ()
    :
        cfg(YAML::LoadFile("select_test.yml")),
        mm_cm(   "mm_cm",    cfg["models"]["with_cm"]),
        mm_cm_np("mm_cm_np", cfg["models"]["with_cm_np"]),
        mm_am(   "mm_am",    cfg["models"]["with_am"])
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

// -- Selection mode: position
BOOST_FIXTURE_TEST_CASE(cm_position, ModelFixture)
{
    auto& cm = mm_cm._cm;

    auto c1 = cm.select_cells(cfg["position"]);
    BOOST_TEST(c1.size() == 3);

    std::vector<SpaceVecType<2>> pos{{0.,0.}, {.5, .5}, {1., 1.}};
    auto c2 = cm.select_cells<SelectionMode::position>(pos);
    BOOST_TEST(c2.size() == 3);
    
    BOOST_TEST(c1 == c2);

    BOOST_TEST(c1[0] == cm.cell_at({0., 0.}));
    BOOST_TEST(c1[1] == cm.cell_at({.5, .5}));
    BOOST_TEST(c1[2] == cm.cell_at({1., 1.}));
}

// -- Selection mode: boundary
BOOST_FIXTURE_TEST_CASE(cm_boundary, ModelFixture)
{
    auto& cm = mm_cm._cm;        // periodic
    auto& cm_np = mm_cm_np._cm;  // non-periodic

    BOOST_TEST(cm.select_cells(cfg["boundary"]).size() == 0);

    auto c1 = cm_np.select_cells<SelectionMode::boundary>("bottom");
    auto c2 = cm_np.select_cells(cfg["boundary"]);
    auto c3 = cm_np.boundary_cells("bottom");

    BOOST_TEST(c3.size() == cm.grid()->shape()[0]);
    BOOST_TEST(c1 == c2);
    BOOST_TEST(c1 == c3);
    BOOST_TEST(c2 == c3);
}

// -- Selection mode: lanes
BOOST_FIXTURE_TEST_CASE(cm_lanes, ModelFixture)
{
    auto& cm = mm_cm._cm;        // periodic
    auto& cm_np = mm_cm_np._cm;  // non-periodic

    // interface
    auto cp1 = cm.select_cells<SelectionMode::lanes>(2, 3);
    auto cp2 = cm.select_cells(cfg["lanes"]);
    auto cnp1 = cm_np.select_cells<SelectionMode::lanes>(2, 3);
    auto cnp2 = cm_np.select_cells(cfg["lanes"]);
    BOOST_TEST(cp1 == cp2);
    BOOST_TEST(cnp1 == cnp2);
    BOOST_TEST(cp1 != cnp1);
    BOOST_TEST(cp2 != cnp2);

    // expected positions in periodic space (2x2 extent, resolution 42)
    for (auto& cell : cp1) {
        MultiIndex midx = cm.midx_of(cell);
        BOOST_TEST_CONTEXT("Cell ID: " << cell->id() << "\nmidx:\n" << midx) {
            // vertical:   either at x-index 0 or 42
            // horizontal: either at y-index 0, 28 or 56
            BOOST_TEST(  (midx[0] == 0) | (midx[0] == 42)
                       | (midx[1] == 0) | (midx[1] == 28) | (midx[1] == 56));
        }
    }

    // expected positions in non-periodic space (2x2 extent, resolution 42)
    for (auto& cell : cnp1) {
        MultiIndex midx = cm_np.midx_of(cell);
        BOOST_TEST_CONTEXT("Cell ID: " << cell->id() << "\nmidx:\n" << midx) {
            // vertical:   either at x-index 28 or 56
            // horizontal: either at y-index 21, 42 or 63
            BOOST_TEST(  (midx[0] == 28) | (midx[0] == 56)
                       | (midx[1] == 21) | (midx[1] == 42) | (midx[1] == 63));
        }
    }
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
