#include "../AmeeMulti.hh"
#include "../adaptionfunctions.hh"
#include "../agentstates/agentstate.hh"
#include "../agentstates/agentstate_policy_complex.hh"
#include "../agentstates/agentstate_policy_highlevel.hh"
#include "../agentstates/agentstate_policy_simple.hh"
#include "../cellstate.hh"
#include "../utils/generators.hh"
#include "../utils/test_utils.hh"
#include "../utils/utils.hh"

using namespace Utopia;
using namespace Utopia::Models::AmeeMulti;
using namespace Utopia::Models::AmeeMulti::Utils;

using RNG = std::mt19937;
using Celltraits = std::vector<double>;
using CS = Cellstate<Celltraits>;

template <typename Model>
void test_model_construction(Model& model)
{
    auto& agents = model.agents();
    auto& cells = model.cells();
    auto cfg = model.get_cfg();

    // get parameters from the config

    // cell parameters
    std::size_t init_cell_traitlen =
        Utopia::as_<std::size_t>(cfg["init_cell_traitlen"]);
    std::string init_cellresourceinflux_kind =
        as_str(cfg["init_cellresourceinflux_kind"]);
    std::vector<double> init_cell_resourceinflux_values =
        as_vector<double>(cfg["init_cell_influxvalues"]);
    std::vector<double> resourceinflux_limits =
        as_vector<double>(cfg["resourceinflux_limits"]);
    std::vector<double> init_celltrait_values =
        as_vector<double>(cfg["init_celltrait_values"]);
    double removethreshold = as_double(cfg["removethreshold"]);
    double decayintensity = as_double(cfg["decayintensity"]);

    // agent parameters
    std::size_t init_genotypelen =
        Utopia::as_<std::size_t>(cfg["init_genotypelen"]);
    double init_resources = as_double(cfg["init_resources"]);
    std::vector<double> init_genotype_values =
        as_vector<double>(cfg["init_genotype_values"]);
    double livingcost = as_double(cfg["livingcost"]);
    double reproductioncost = as_double(cfg["reproductioncost"]);
    double offspringresources = as_double(cfg["offspringresources"]);
    double deaththreshold = as_double(cfg["deathprobability"]);
    double substitutionrate = as_double(cfg["substitutionrate"]);
    double insertionrate = as_double(cfg["insertionrate"]);
    double substitution_std = as_double(cfg["substitution_std"]);
    double modifiercost = as_double(cfg["modifiercost"]);
    double upper_resourcelimit = as_double(cfg["upper_resourcelimit"]);

    // model parameters
    auto gridsize = Utopia::as_<std::vector<unsigned>>(cfg["grid_size"]);
    std::string adaptionfunction = as_str(cfg["adaptionfunction"]);
    std::string Agenttype = as_str(cfg["Agenttype"]);
    bool construction = as_bool(cfg["construction"]);
    bool decay = as_bool(cfg["decay"]);
    std::array<unsigned, 2> highresinterval =
        as_array<unsigned, 2>(cfg["highresinterval"]);
    bool highresoutput = as_bool(cfg["highresoutput"]);

    // check that the initialization of model,  cells and agent was correct
    // check model parameters
    ASSERT_EQ(model.get_highresoutput(), highresoutput);
    ASSERT_EQ(model.get_highresinterval(), highresinterval);
    ASSERT_EQ(model.get_decay(), decay);
    ASSERT_EQ(model.get_construction(), construction);

    // check agent parameters
    ASSERT_EQ(int(agents.size()), 1);
    auto inhabited_cell = agents[0]->state().habitat;
    for (auto& agent : agents)
    {
        ASSERT_EQ(std::abs(agent->state().resources - init_resources), 0.);
        ASSERT_EQ(agent->state().genotype.size(), init_genotypelen);
        ASSERT_LEQ(double(*std::max_element(agent->state().genotype.begin(),
                                            agent->state().genotype.end())),
                   init_genotype_values[1]);
        ASSERT_GEQ(double(*std::min_element(agent->state().genotype.begin(),
                                            agent->state().genotype.end())),
                   init_genotype_values[0]);
        ASSERT_GEQ(std::accumulate(agent->state().adaption.begin(),
                                   agent->state().adaption.end(), 0.),
                   livingcost);
    }

    ASSERT_EQ(model.get_upperresourcelimit(), upper_resourcelimit);
    ASSERT_EQ(model.get_livingcost(), livingcost);
    ASSERT_EQ(model.get_reproductioncost(), reproductioncost);
    ASSERT_EQ(model.get_offspringresources(), offspringresources);
    ASSERT_EQ(model.get_deathprobability(), deaththreshold);
    auto mutationrates = model.get_mutationrates();
    ASSERT_EQ(mutationrates[0], substitutionrate);
    ASSERT_EQ(mutationrates[1], insertionrate);
    ASSERT_EQ(mutationrates[2], substitution_std);
    ASSERT_EQ(model.get_modifiercost(), modifiercost);

    // check cell parameters
    for (auto& cell : cells)
    {
        if (cell == inhabited_cell)
        {
            continue;
        }
        else
        {
            auto& state = cell->state();
            ASSERT_EQ(state.celltrait.size(), init_cell_traitlen);
            ASSERT_LEQ(
                *std::max_element(state.celltrait.begin(), state.celltrait.end()),
                init_celltrait_values[1]);
            ASSERT_GEQ(
                *std::min_element(state.celltrait.begin(), state.celltrait.end()),
                init_celltrait_values[0]);
            ASSERT_EQ(state.resourceinfluxes, init_cell_resourceinflux_values);
            ASSERT_EQ(state.celltrait, state.original);
            ASSERT_EQ(state.modtimes, std::vector<double>(state.celltrait.size(), 0.));
        }
    }
    ASSERT_EQ(model.get_removethreshold(), removethreshold);
    ASSERT_EQ(model.get_decayintensity(), decayintensity);
}

template <typename Model>
void test_model_functions(Model& model)
{
    auto& agents = model.agents();

    auto adam = agents[0];
    auto& adamstate = adam->state();
    auto eden = adamstate.habitat;
    auto& edenstate = eden->state();

    // backup
    auto adamphenotype = adamstate.phenotype;

    auto edentrait = edenstate.celltrait;

    // update adaption
    adamstate.phenotype = std::vector<double>{1., 2., -1., 2., 4.};
    edenstate.celltrait = std::vector<double>{-1., 1., 2., 1., 3.};
    edenstate.resources = std::vector<double>{1., 1., 1., 1., 1.};
    edenstate.resourceinfluxes = std::vector<double>{1., 1., 1., 1., 1.};
    adamstate.start = 0;
    adamstate.end = 5;
    adamstate.adaption = {0., 0., 0., 0., 0.};
    model.update_adaption(adam);
    ASSERT_EQ(adamstate.adaption, (std::vector<double>{0., 1., 0., 1., 6.}));

    // metabolism
    adamstate.resources = 0.;
    edenstate.resources = std::vector<double>(5, 10.);
    model.metabolism(adam);
    ASSERT_EQ(adamstate.resources, 3.);
    ASSERT_EQ(edenstate.resources, (std::vector<double>{10., 9., 10., 9., 4.}));
    ASSERT_EQ(int(adamstate.age), 1);

    // metabolism values odd: too few resources on cell
    edenstate.resources = std::vector<double>{2., 2., 2., 2., 2.};
    adamstate.adaption = std::vector<double>{8., 8., 8., 8., 8.};
    adamstate.resources = 5.;
    model.metabolism(adam);
    ASSERT_EQ(adamstate.resources, 10.);
    ASSERT_EQ(edenstate.resources, (std::vector<double>{0., 0., 0, 0, 0}));
    ASSERT_EQ(int(adamstate.age), 2);

    // metabolism values odd: too much adaption -> limited by upper influx
    edenstate.resources = std::vector<double>{20., 20., 20., 20., 20.};
    adamstate.adaption = std::vector<double>{20., 20., 20., 20., 20.};
    adamstate.resources = 5.;
    model.metabolism(adam);
    ASSERT_EQ(adamstate.resources, 75.);
    ASSERT_EQ(edenstate.resources, (std::vector<double>{5., 5., 5, 5, 5}));
    ASSERT_EQ(int(adamstate.age), 3);

    // move
    auto neighbors =
        Utopia::Neighborhoods::MooreNeighbor::neighbors(eden, model.cellmanager());
    edenstate.celltrait = std::vector<double>(8, 0.);
    edenstate.resourceinfluxes = std::vector<double>(8, 10.);
    edenstate.resources = std::vector<double>(8, 10.);
    adamstate.phenotype = std::vector<double>(8, 1);
    adamstate.start = 1;
    adamstate.end = 5;
    adamstate.resources = 0.5;
    adamstate.adaption = std::vector<double>(4, 0.);
    model.update_adaption(adam);

    // directed movement
    for (auto& neighbor : neighbors)
    {
        neighbor->state().celltrait = std::vector<double>(8, 0);
        neighbor->state().resources = std::vector<double>(8, 10.);
        neighbor->state().resourceinfluxes = std::vector<double>(8, 10.);
    }

    neighbors[2]->state().celltrait = adamstate.phenotype;

    model.move(adam);

    assert(neighbors[2].get() == adamstate.habitat.get());
    ASSERT_EQ(adamstate.adaption, (std::vector<double>{1, 1, 1, 1}));

    eden = adamstate.habitat;
    edenstate = eden->state();

    // random movement
    adamstate.resources = 0.5; // has to move
    edenstate.celltrait = std::vector<double>(8, 1.);
    edenstate.resourceinfluxes = std::vector<double>(8, 10.);
    edenstate.resources = std::vector<double>(8, 10.);
    adamstate.phenotype = std::vector<double>(8, 1.);
    adamstate.start = 1;
    adamstate.end = 5;
    adamstate.resources = 0.5;
    adamstate.adaption = std::vector<double>(4, 0.);
    model.update_adaption(adam);

    for (auto& neighbor : neighbors)
    {
        neighbor->state().celltrait = std::vector<double>(8, 1.);
    }
    model.update_adaption(adam);
    model.move(adam);
    assert(adamstate.habitat != eden);
    ASSERT_EQ(adamstate.adaption, (std::vector<double>{1., 1., 1., 1.}))

    eden = adamstate.habitat;
    edenstate = eden->state();

    // modify
    adamstate.intensity = 0.5;
    adamstate.start = 2;
    adamstate.end = 5;
    adam->state().habitat->state().celltrait = std::vector<double>(6, 6.);
    adam->state().habitat->state().resources = std::vector<double>(6, 1.);
    adam->state().habitat->state().resourceinfluxes = std::vector<double>(6, 1.);
    adamstate.phenotype = std::vector<double>(6, 4.);
    adamstate.resources = 10.;
    model.set_modifiercost(0.1);

    model.modify(adam);

    ASSERT_EQ(adam->state().habitat->state().celltrait,
              (std::vector<double>{6., 6., 5., 5., 5., 6.}));

    ASSERT_EQ(adamstate.resources, 9.7);
    ASSERT_EQ(edenstate.modtimes, (std::vector<double>(6, 0.)));

    adamstate.end = 8;
    adamstate.phenotype = std::vector<double>(8, 4.);
    adam->state().habitat->state().celltrait = std::vector<double>(6, 6.);
    adam->state().habitat->state().resources = std::vector<double>(6, 1.);
    adam->state().habitat->state().resourceinfluxes = std::vector<double>(6, 1.);
    adam->state().habitat->state().modtimes = std::vector<double>(6, 0.);
    adam->state().resources = 10.;
    model.set_modifiercost(0.1);
    model.increment_time();

    model.modify(adam);

    ASSERT_EQ(adam->state().habitat->state().celltrait,
              (std::vector<double>{6., 6., 5., 5., 5., 5., 2., 2.}));
    ASSERT_EQ(adam->state().habitat->state().modtimes,
              (std::vector<double>{0., 0., 1., 1., 1., 1., 1., 1.}));
    ASSERT_EQ(adamstate.resources, 9.2);

    // test bad values -> should do nothing and hence everything is as it was
    adam->state().end = adam->state().start;
    model.modify(adam);
    ASSERT_EQ(adam->state().habitat->state().celltrait,
              (std::vector<double>{6., 6., 5., 5., 5., 5., 2., 2.}));
    ASSERT_EQ(adamstate.resources, 9.2);
    ASSERT_EQ(adam->state().habitat->state().modtimes,
              (std::vector<double>{0., 0., 1., 1., 1., 1., 1., 1.}));

    // cannot afford modification - internally
    adamstate.intensity = 2;
    adamstate.start = 2;
    adamstate.end = 5;
    adamstate.phenotype = std::vector<double>(6, 4.);
    adamstate.habitat->state().celltrait = std::vector<double>(6, 6.);
    adam->state().habitat->state().resources = std::vector<double>(6, 1.);
    adam->state().habitat->state().resourceinfluxes = std::vector<double>(6, 1.);
    adam->state().habitat->state().modtimes = std::vector<double>(6, 0.);
    adamstate.resources = 10.;
    model.set_modifiercost(1.);
    model.modify(adam);
    ASSERT_EQ(adam->state().habitat->state().celltrait,
              (std::vector<double>{6., 6., 2., 2., 6., 6.}));

    ASSERT_EQ(adamstate.resources, 2.);
    ASSERT_EQ(adam->state().habitat->state().modtimes,
              (std::vector<double>{0., 0., 1., 1., 0., 0.}));

    // cannot afford modification - beyond celltraitlength

    adamstate.start = 2;
    adamstate.end = 8;
    adamstate.intensity = 2;
    adamstate.phenotype = std::vector<double>(8, 4.);

    adam->state().habitat->state().celltrait = std::vector<double>(6, 6.);
    adam->state().habitat->state().resources = std::vector<double>(6, 1.);
    adam->state().habitat->state().resourceinfluxes = std::vector<double>(6, 1.);
    adam->state().resources = 15.;
    model.set_modifiercost(0.5);

    model.modify(adam);

    ASSERT_EQ(adam->state().habitat->state().celltrait,
              (std::vector<double>{6., 6., 2., 2., 2., 2., 8.}));
    ASSERT_EQ(adam->state().habitat->state().modtimes,
              (std::vector<double>{0., 0., 1., 1., 1., 1., 1.}));
    ASSERT_EQ(adamstate.resources, 3.);

    // reset time again
    model.set_time(0);

    // reproduce
    adam->state().resources = 10;
    model.reproduce(adam);
    ASSERT_EQ(adam->state().fitness, 4.);
    ASSERT_EQ(model.agents().size(), std::size_t(5));
    ASSERT_EQ(adam->state().resources, 2.);
    for (std::size_t i = 1; i < model.agents().size(); ++i)
    {
        ASSERT_EQ(model.agents()[i]->state().resources, 1.);
        assert(model.agents()[i]->state().habitat == adam->state().habitat);
        ASSERT_EQ(model.agents()[i]->state().fitness, 0.);
        ASSERT_EQ(model.agents()[i]->state().age, std::size_t(0));
    }

    // kill
    auto deadmanwalking = model.agents().back();
    deadmanwalking->state().resources = 0.;
    ASSERT_EQ(deadmanwalking->state().deathflag, false);
    model.kill(deadmanwalking);
    ASSERT_EQ(deadmanwalking->state().deathflag, true);

    // update cell
    auto cell = model.cells().front();
    cell->state().celltrait = std::vector<double>(10, 1.);
    cell->state().original = std::vector<double>(10, 1.);
    cell->state().resourceinfluxes =
        std::vector<double>(cell->state().celltrait.size(), 10);
    cell->state().resources = std::vector<double>(cell->state().celltrait.size(), 1);

    model.update_cell(cell);
    ASSERT_EQ(cell->state().resources, std::vector<double>(10, 11.));

    // decay_celltrait
    cell->state().celltrait = std::vector<double>(7, 5.);
    cell->state().original = std::vector<double>(5, 1.);
    cell->state().modtimes = std::vector<double>(7, 2);
    cell->state().resources = std::vector<double>(7, 2);
    cell->state().resourceinfluxes = std::vector<double>(7, 5);

    model.increment_time(5);

    model.set_decayintensity(0.5);
    model.celltrait_decay(cell);
    ASSERT_EQ(cell->state().celltrait,
              (std::vector<double>{1.8925206405937192, 1.8925206405937192,
                                   1.8925206405937192, 1.8925206405937192, 1.8925206405937192,
                                   1.115650800742149, 1.115650800742149}));

    // decay until the first added locus is removed
    model.increment_time(2);
    model.set_decayintensity(2.5);

    cell->state().celltrait = std::vector<double>(7, 5.);
    cell->state().original = std::vector<double>(5, 1.);
    cell->state().modtimes = std::vector<double>{4, 4, 4, 4, 4, 1, 4};
    cell->state().resources = std::vector<double>(7, 2);
    cell->state().resourceinfluxes = std::vector<double>(7, 5);
    model.celltrait_decay(cell);

    for (std::size_t i = 0; i < 5; ++i)
    {
        ASSERT_EQ(cell->state().celltrait[i], 1.002212337480591);
        ASSERT_EQ(cell->state().modtimes[i], 4.);
    }

    assert(std::isnan(cell->state().celltrait[5]));
    assert(std::isnan(cell->state().modtimes[5]));

    ASSERT_EQ(cell->state().celltrait[6], 0.002765421850739168);
    ASSERT_EQ(cell->state().modtimes[6], 4.);

    ASSERT_EQ(cell->state().resourceinfluxes, (std::vector<double>{5, 5, 5, 5, 5, 0, 5}));
    ASSERT_EQ(cell->state().resources, std::vector<double>(7, 2));
}

void test_simple()
{
    Utopia::PseudoParent<RNG> parentmodel_simple(
        "multi_test_config_simple.yml");
    auto cellmanager_simple =
        Utopia::Setup::create_grid_manager_cells<CS, true, 2, true, false>(
            "AmeeMultiSimple", parentmodel_simple);

    auto grid_simple = cellmanager_simple.grid();
    using GridType = typename decltype(grid_simple)::element_type;
    using Cell = typename decltype(cellmanager_simple)::Cell;

    Utopia::GridWrapper<GridType> wrapper_simple{
        grid_simple, cellmanager_simple.extensions(), cellmanager_simple.grid_cells()};

    using GenotypeS = std::vector<double>;
    using PhenotypeS = std::vector<double>;
    using PolicyS = Agentstate_policy_simple<GenotypeS, PhenotypeS, RNG>;
    using ASS = AgentState<Cell, PolicyS>;

    parentmodel_simple.get_logger()->info("Using simple Agentstate");
    // make agents and agentmanager
    auto agents_simple = Utopia::Setup::create_agents_on_grid(wrapper_simple, 1, ASS());

    auto agentmanager_simple = Utopia::Setup::create_manager_agents<true, true>(
        wrapper_simple, agents_simple);

    // making model types
    using Modeltypes_Simple =
        Utopia::ModelTypes<std::pair<Cell, typename decltype(agentmanager_simple)::Agent>, Utopia::BCDummy, RNG>;

    // get AmeeMulti typedef
    using AmeeMulti_Simple =
        AmeeMulti<Cell, decltype(cellmanager_simple), decltype(agentmanager_simple), Modeltypes_Simple, true, true>;

    // make model
    AmeeMulti_Simple model_simple("AmeeMultiSimple", parentmodel_simple,
                                  cellmanager_simple, agentmanager_simple);

    // actual tests
    test_model_construction(model_simple);
    test_model_functions(model_simple);
}

void test_complex()
{
    Utopia::PseudoParent<RNG> parentmodel_complex(
        "multi_test_config_complex.yml");
    auto cellmanager_complex =
        Utopia::Setup::create_grid_manager_cells<CS, true, 2, true, false>(
            "AmeeMultiComplex", parentmodel_complex);

    auto grid_complex = cellmanager_complex.grid();
    using GridType = typename decltype(grid_complex)::element_type;
    using Cell = typename decltype(cellmanager_complex)::Cell;

    Utopia::GridWrapper<GridType> wrapper_complex{
        grid_complex, cellmanager_complex.extensions(), cellmanager_complex.grid_cells()};

    using GenotypeS = std::vector<int>;
    using PhenotypeS = std::vector<double>;
    using PolicyS = Agentstate_policy_complex<GenotypeS, PhenotypeS, RNG>;
    using ASS = AgentState<Cell, PolicyS>;

    parentmodel_complex.get_logger()->info("Using complex Agentstate");
    // make agents and agentmanager
    auto agents_complex =
        Utopia::Setup::create_agents_on_grid(wrapper_complex, 1, ASS());

    auto agentmanager_complex = Utopia::Setup::create_manager_agents<true, true>(
        wrapper_complex, agents_complex);

    // making model types
    using Modeltypes_Simple =
        Utopia::ModelTypes<std::pair<Cell, typename decltype(agentmanager_complex)::Agent>, Utopia::BCDummy, RNG>;

    // get AmeeMulti typedef
    using AmeeMulti_Complex =
        AmeeMulti<Cell, decltype(cellmanager_complex), decltype(agentmanager_complex), Modeltypes_Simple, true, true>;

    // make model
    AmeeMulti_Complex model_complex("AmeeMultiComplex", parentmodel_complex,
                                    cellmanager_complex, agentmanager_complex);

    // actual tests
    test_model_construction(model_complex);
    test_model_functions(model_complex);
}

void test_highlevel()
{
    Utopia::PseudoParent<RNG> parentmodel_highlevel(
        "multi_test_config_highlevel.yml");
    auto cellmanager_highlevel =
        Utopia::Setup::create_grid_manager_cells<CS, true, 2, true, false>(
            "AmeeMultiHighlevel", parentmodel_highlevel);

    auto grid_highlevel = cellmanager_highlevel.grid();
    using GridType = typename decltype(grid_highlevel)::element_type;
    using Cell = typename decltype(cellmanager_highlevel)::Cell;

    Utopia::GridWrapper<GridType> wrapper_highlevel{
        grid_highlevel, cellmanager_highlevel.extensions(),
        cellmanager_highlevel.grid_cells()};

    using GenotypeS = std::vector<double>;
    using PhenotypeS = std::vector<double>;
    using PolicyS = Agentstate_policy_highlevel<GenotypeS, PhenotypeS, RNG>;
    using ASS = AgentState<Cell, PolicyS>;

    parentmodel_highlevel.get_logger()->info("Using highlevel Agentstate");
    // make agents and agentmanager
    auto agents_highlevel =
        Utopia::Setup::create_agents_on_grid(wrapper_highlevel, 1, ASS());

    auto agentmanager_highlevel = Utopia::Setup::create_manager_agents<true, true>(
        wrapper_highlevel, agents_highlevel);

    // making model types
    using Modeltypes_Simple =
        Utopia::ModelTypes<std::pair<Cell, typename decltype(agentmanager_highlevel)::Agent>, Utopia::BCDummy, RNG>;

    // get AmeeMulti typedef
    using AmeeMulti_Highlevel =
        AmeeMulti<Cell, decltype(cellmanager_highlevel), decltype(agentmanager_highlevel), Modeltypes_Simple, true, true>;

    // make model
    AmeeMulti_Highlevel model_highlevel("AmeeMultiHighlevel", parentmodel_highlevel,
                                        cellmanager_highlevel, agentmanager_highlevel);

    // actual tests
    test_model_construction(model_highlevel);
    test_model_functions(model_highlevel);
}

int main(int argc, char** argv)
{
    Dune::MPIHelper::instance(argc, argv);

    test_simple();
    test_complex();
    test_highlevel();

    return 0;
}