#include "../AmeeMulti.hh"
#include "../adaptionfunctions.hh"
#include "../agentstates/agentstate.hh"
#include "../agentstates/agentstate_gauss.hh"
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
    std::vector<double> init_cell_resourceinflux =
        as_vector<double>(cfg["init_cell_resourceinflux"]);
    std::vector<double> init_cell_resourceinflux_values =
        as_vector<double>(cfg["cell_influxvalues"]);
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
        std::cout << " glen: " << agent->state().genotype.size() << std::endl;
        std::cout << " plen: " << agent->state().phenotype.size() << std::endl;
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
            ASSERT_EQ(state.resourceinfluxes, init_cell_resourceinflux);
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
    adamstate.start = 0;
    adamstate.end = 4;
    adamstate.adaption = {0., 0., 0., 0.};
    model.update_adaption(adam);
    ASSERT_EQ(adamstate.adaption, (std::vector<double>{0., 1., 0., 1., 6.}));

    // metabolism
    adamstate.resources = 0.;
    edenstate.resources = std::vector<double>(5, 10.);
    model.metabolism(adam);
    ASSERT_EQ(adamstate.resources, 7.);
    ASSERT_EQ(edenstate.resources, (std::vector<double>{10., 9., 10., 9., 4.}));
    ASSERT_EQ(int(adamstate.age), 1);

    // move
    auto neighbors =
        Utopia::Neighborhoods::MooreNeighbor::neighbors(eden, model.cellmanager());
    edenstate.celltrait = std::vector<double>(8, 1.);
    model.update_adaption(adam);

    // directed movement
    for (auto& neighbor : neighbors)
    {
        neighbor->state().celltrait = std::vector<double>(8, 1.);
    }

    neighbors[2]->state().celltrait = adamstate.phenotype;

    model.move(adam);

    ASSERT_EQ(neighbors[2], adamstate.habitat);
    ASSERT_EQ(adamstate.adaption, (std::vector<double>{1, 4, 1, 4, 16}));

    // random movement
    adamstate.resources = 0.5; // has to move
    for (auto& neighbor : neighbors)
    {
        neighbor->state().celltrait = std::vector<double>(8, 1.);
    }
    model.update_adaption(adam);
    model.move(adam);
    ASSERT_NEQ(adamstate.habitat, neighbors[2]);
    ASSERT_NEQ(adamstate.habitat, eden);
    ASSERT_EQ(adamstate.adaption, (std::vector<double>{1., 1., 0., 1., 1.}))

    eden = adamstate.habitat;
    edenstate = eden->state();
    // modify
    adamstate.intensity = 1.2;
    adamstate.start = 2;
    adamstate.end = 5;
    edenstate.celltrait = std::vector<double>(6, 1.);
    adamstate.phenotype = std::vector<double>(6, 4.8);
    adamstate.resources = 1.;
    model.modify(adam);
    ASSERT_EQ(edenstate.celltrait,
              (std::vector<double>{1., 1., 5.56, 5.56, 5.56, 1., 1.}));
    ASSERT_EQ(adamstate.resources, 0.7);
    ASSERT_EQ(edenstate.modtimes, (std::vector<double>(6, 0.)));

    adamstate.end = 8;
    edenstate.celltrait = std::vector<double>(6, 1.);
    adamstate.phenotype = std::vector<double>(10, 4.8);
    adamstate.resources = 1.;
    model.modify(adam);
    ASSERT_EQ(3, 6);
    ASSERT_EQ(edenstate.celltrait,
              (std::vector<double>{1., 1., 5.56, 5.56, 5.56, 5.56, 5.56, 5.56}));
    ASSERT_EQ(adamstate.resources, 0.4);
    ASSERT_EQ(edenstate.modtimes, (std::vector<double>(8, 0.)));
    assert(3 == 6);

    // reproduce

    // kill

    // update agent

    // update cell

    // decay_celltrait
}

int main(int argc, char** argv)
{
    Dune::MPIHelper::instance(argc, argv);

    using namespace Utopia::Models::AmeeMulti;

    using RNG = std::mt19937;
    using Celltraits = std::vector<double>;
    using Cellstate = Cellstate<Celltraits>;

    // Initialize the PseudoParent from config file path
    Utopia::PseudoParent<RNG> parentmodel_simple(
        "multi_test_config_simple.yml");
    Utopia::PseudoParent<RNG> parentmodel_complex(
        "multi_test_config_complex.yml");
    Utopia::PseudoParent<RNG> parentmodel_highlevel(
        "multi_test_config_highlevel.yml");

    ////////////////////////////////////////////////////////////////////////
    // using simple agent
    // make managers first -> this has to be wramodeled in a factory function
    auto cellmanager_simple =
        Utopia::Setup::create_grid_manager_cells<Cellstate, true, 2, true, false>(
            "AmeeMulti", parentmodel_simple);

    auto grid_simple = cellmanager_simple.grid();
    using GridType = typename decltype(grid_simple)::element_type;
    using Cell = typename decltype(cellmanager_simple)::Cell;

    Utopia::GridWrapper<GridType> wrapper_simple{
        grid_simple, cellmanager_simple.extensions(), cellmanager_simple.grid_cells()};

    using GenotypeS = std::vector<double>;
    using PhenotypeS = std::vector<double>;
    using PolicyS = Agentstate_policy_simple<GenotypeS, PhenotypeS, RNG>;
    using ASS = AgentState<Cell, GenotypeS, PhenotypeS, RNG, PolicyS>;

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
    AmeeMulti_Simple model_simple("AmeeMulti", parentmodel_simple,
                                  cellmanager_simple, agentmanager_simple);

    // actual tests
    test_model_construction(model_simple);
    test_model_functions(model_simple);

    // ////////////////////////////////////////////////////////////////////////
    // // using complex agent
    // // make managers first -> this has to be wramodeled in a factory function
    // auto cellmanager_complex =
    //     Utopia::Setup::create_grid_manager_cells<Cellstate, true, 2, true, false>(
    //         "AmeeMulti", parentmodel_complex);

    // auto grid_complex = cellmanager_complex.grid();
    // using GridType = typename decltype(grid_complex)::element_type;
    // using Cell = typename decltype(cellmanager_complex)::Cell;

    // Utopia::GridWrapper<GridType> wrapper_complex{
    //     grid_complex, cellmanager_complex.extensions(), cellmanager_complex.grid_cells()};

    // using GenotypeC = std::vector<int>;
    // using PhenotypeC = std::vector<double>;
    // using PolicyC = Agentstate_policy_complex<GenotypeC, PhenotypeC, RNG>;
    // using ASC = AgentState<Cell, GenotypeC, PhenotypeC, RNG, PolicyC>;

    // parentmodel_complex.get_logger()->info("Using complex Agentstate");
    // // make agents and agentmanager
    // auto agents_complex =
    //     Utopia::Setup::create_agents_on_grid(wrapper_complex, 1, ASC());

    // auto agentmanager_complex = Utopia::Setup::create_manager_agents<true, true>(
    //     wrapper_complex, agents_complex);

    // // making model types
    // using Modeltypes_Complex =
    //     Utopia::ModelTypes<std::pair<Cell, typename decltype(agentmanager_complex)::Agent>, Utopia::BCDummy, RNG>;

    // // get AmeeMulti typedef
    // using AmeeMulti_Complex =
    //     AmeeMulti<Cell, decltype(cellmanager_complex), decltype(agentmanager_complex), Modeltypes_Complex, true, true>;

    // // make model
    // AmeeMulti_Complex model_complex("AmeeMulti", parentmodel_complex,
    //                                 cellmanager_complex, agentmanager_complex);

    // // actual tests
    // test_model_construction(model_complex);
    // test_model_functions(model_complex);

    // ////////////////////////////////////////////////////////////////////////
    // // using highlevel agent
    // // make managers first -> this has to be wramodeled in a factory function
    // auto cellmanager_highlevel =
    //     Utopia::Setup::create_grid_manager_cells<Cellstate, true, 2, true, false>(
    //         "AmeeMulti", parentmodel_highlevel);

    // auto grid_highlevel = cellmanager_highlevel.grid();
    // using GridType = typename decltype(grid_highlevel)::element_type;
    // using Cell = typename decltype(cellmanager_highlevel)::Cell;

    // Utopia::GridWrapper<GridType> wrapper_highlevel{
    //     grid_highlevel, cellmanager_highlevel.extensions(),
    //     cellmanager_highlevel.grid_cells()};

    // using GenotypeHL = std::vector<double>;
    // using PhenotypeHL = std::vector<double>;
    // using PolicyHL = Agentstate_policy_highlevel<GenotypeHL, PhenotypeHL, RNG>;
    // using ASH = AgentState<Cell, GenotypeHL, PhenotypeHL, RNG, PolicyHL>;

    // parentmodel_highlevel.get_logger()->info("Using highlevel Agentstate");
    // // make agents and agentmanager
    // auto agents_highlevel =
    //     Utopia::Setup::create_agents_on_grid(wrapper_highlevel, 1, ASH());

    // auto agentmanager_highlevel = Utopia::Setup::create_manager_agents<true, true>(
    //     wrapper_highlevel, agents_highlevel);

    // // making model types
    // using Modeltypes_HL =
    //     Utopia::ModelTypes<std::pair<Cell, typename decltype(agentmanager_highlevel)::Agent>, Utopia::BCDummy, RNG>;

    // // get AmeeMulti typedef
    // using AmeeMulti_HL =
    //     AmeeMulti<Cell, decltype(cellmanager_highlevel), decltype(agentmanager_highlevel), Modeltypes_HL, true, true>;

    // // make model
    // AmeeMulti_HL model_highlevel("AmeeMulti", parentmodel_highlevel,
    //                              cellmanager_highlevel, agentmanager_highlevel);

    // // actual tests
    // test_model_construction(model_highlevel);
    // test_model_functions(model_highlevel);

    return 0;
}