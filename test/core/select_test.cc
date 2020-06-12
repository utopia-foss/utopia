#define BOOST_TEST_MODULE select test

#include <boost/test/included/unit_test.hpp>

#include <utopia/core/select.hh>
#include <utopia/core/testtools.hh>
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
    auto cp2 = cm.select_cells(cfg["lanes"]["simple"]);
    auto cnp1 = cm_np.select_cells<SelectionMode::lanes>(2, 3);
    auto cnp2 = cm_np.select_cells(cfg["lanes"]["simple"]);
    BOOST_TEST(cp1 == cp2);
    BOOST_TEST(cnp1 == cnp2);
    BOOST_TEST(cp1 != cnp1);
    BOOST_TEST(cp2 != cnp2);

    // interface for permeable lanes
    auto cp_wp1 = cm.select_cells<SelectionMode::lanes>(2, 3,
                        std::pair<double, double>({0.2, 0.2}));
    auto cp_wp2 = cm.select_cells(cfg["lanes"]["w_permeability"]);
    auto cnp_wp1 = cm_np.select_cells<SelectionMode::lanes>(2, 3,
                        std::pair<double, double>({0.2, 0.3}));
    auto cnp_wp2 = cm_np.select_cells(cfg["lanes"]["w_permeability"]);
    BOOST_TEST(cp_wp1 != cp1);
    BOOST_TEST(cp_wp2 != cp2);
    BOOST_TEST(cnp_wp1 != cnp1);
    BOOST_TEST(cnp_wp2 != cnp2);

    // interface for gated lanes
    auto cp_wg1 = cm.select_cells<SelectionMode::lanes>(2, 3,
                        std::pair<double, double>({0., 0.}),
                        std::pair<unsigned int, unsigned int>({2, 3}));
    auto cp_wg2 = cm.select_cells(cfg["lanes"]["w_gates"]);
    auto cnp_wg1 = cm_np.select_cells<SelectionMode::lanes>(2, 3,
                        std::pair<double, double>({0., 0.}),
                        std::pair<unsigned int, unsigned int>({2, 3}));
    auto cnp_wg2 = cm_np.select_cells(cfg["lanes"]["w_gates"]);
    BOOST_TEST(cp_wg1 == cp_wg2);
    BOOST_TEST(cnp_wg1 == cnp_wg2);
    BOOST_TEST(cp_wg1 != cp1);
    BOOST_TEST(cp_wg2 != cp2);
    BOOST_TEST(cnp_wg1 != cnp1);
    BOOST_TEST(cnp_wg2 != cnp_wp2);
    BOOST_TEST(cp_wg1 != cp_wp1);
    BOOST_TEST(cp_wg2 != cp_wp2);
    BOOST_TEST(cnp_wg1 != cnp_wp1);
    BOOST_TEST(cnp_wg2 != cnp_wp2);

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


    // expected positions of gates in periodic space (2x2 extent, resolution 42)
    for (auto& cell : cp_wg1) {
        MultiIndex midx = cm.midx_of(cell);
        BOOST_TEST_CONTEXT("Cell ID: " << cell->id() << "\nmidx:\n" << midx) {
            // vertical:   either at x-index 0 or 42
            //             gates at y-index 13, 14, 15 / 41, 42, 43 / 69, 70, 71
            // horizontal: either at y-index 0, 28 or 56
            //             gates at x-index 20, 21, and 62, 63
            BOOST_TEST(  (midx[0] == 0) | (midx[0] == 42)
                       | (midx[1] == 0) | (midx[1] == 28) | (midx[1] == 56));

            // not selecting gate cells in vertical lanes
            BOOST_TEST(midx[1] != 13); BOOST_TEST(midx[1] != 14);
            BOOST_TEST(midx[1] != 15);
            BOOST_TEST(midx[1] != 41); BOOST_TEST(midx[1] != 42);
            BOOST_TEST(midx[1] != 43);
            BOOST_TEST(midx[1] != 69); BOOST_TEST(midx[1] != 70);
            BOOST_TEST(midx[1] != 71);


            // not selecting gate cells in horizontal lanes
            BOOST_TEST(midx[0] != 20); BOOST_TEST(midx[0] != 21);
            BOOST_TEST(midx[0] != 62); BOOST_TEST(midx[0] != 63);
        }
    }

    // expected positions of gates in non-periodic space
    // (2x2 extent, resolution 42)
    for (auto& cell : cnp_wg1) {
        MultiIndex midx = cm_np.midx_of(cell);
        BOOST_TEST_CONTEXT("Cell ID: " << cell->id() << "\nmidx:\n" << midx) {
            // vertical:   either at x-index 28 or 56
            //             gates at y-index 10.5, 31.5, 52.5, 73.5
            // horizontal: either at y-index 21, 42 or 63
            //             gates at x-index  14, 42, 70
            BOOST_TEST(  (midx[0] == 28) | (midx[0] == 56)
                       | (midx[1] == 21) | (midx[1] == 42) | (midx[1] == 63));

            // not selecting gate cells in vertical lanes
            BOOST_TEST(midx[1] != 9); BOOST_TEST(midx[1] != 10);
            BOOST_TEST(midx[1] != 11);
            BOOST_TEST(midx[1] != 30); BOOST_TEST(midx[1] != 31);
            BOOST_TEST(midx[1] != 32);
            BOOST_TEST(midx[1] != 51); BOOST_TEST(midx[1] != 52);
            BOOST_TEST(midx[1] != 53);
            BOOST_TEST(midx[1] != 72); BOOST_TEST(midx[1] != 73);
            BOOST_TEST(midx[1] != 74);

            // not selecting gate cells in horizontal lanes
            BOOST_TEST(midx[0] != 13); BOOST_TEST(midx[0] != 14);
            BOOST_TEST(midx[0] != 41); BOOST_TEST(midx[0] != 42);
            BOOST_TEST(midx[0] != 69); BOOST_TEST(midx[0] != 70);
        }
    }

    // check error message on too large gate width
    TestTools::check_exception<std::invalid_argument>(
        [&](){
            cm_np.select_cells<SelectionMode::lanes>(2, 3,
                                                     std::make_pair(0., 0.),
                                                     std::make_pair(1234u,0u));
        },
        "Failed to determine gate cells for lane selection",
        {__LINE__, __FILE__}
    );
    // _Not_ an issue for periodic grid
    cm.select_cells<SelectionMode::lanes>(2, 3, std::make_pair(0., 0.),
                                          std::make_pair(1234u, 0u));

    // finally, check a bunch of different configurations to ensure that the
    // config interface works as expected
    TestTools::test_config_callable(
        [&](const DataIO::Config& params){
            cm.select_cells(params);
            cm_np.select_cells(params);
        },
        cfg["lanes"]["batch_test"],
        "CellManager::select_cells batch test",
        {__LINE__, __FILE__}
    );
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
