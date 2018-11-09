#include <dune/utopia/core/tags.hh>
#include <dune/utopia/models/Amee/agentstates/agentstate.hh>
#include <dune/utopia/models/Amee/agentstates/agentstate_policy_complex.hh>
#include <dune/utopia/models/Amee/agentstates/agentstate_policy_simple.hh>
#include <dune/utopia/models/Amee/utils/custom_cell.hh>
#include <dune/utopia/models/Amee/utils/custom_setup.hh>
#include <dune/utopia/models/Amee/utils/generators.hh>
#include <dune/utopia/models/Amee/utils/test_utils.hh>
#include <dune/utopia/models/AmeeMulti/AmeeMulti.hh>

using namespace Utopia::Models::Amee::Utils;
using namespace Utopia::Models::AmeeMulti;
using namespace std::literals::chrono_literals;
using namespace Utopia;

template <typename Model>
void test_model_construction(Model& model)
{
    model.get_logger()->info("testing construction");
    auto& agents = model.population();
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
    std::vector<double> cell_resourcecapacities =
        as_vector<double>(cfg["cellresourcecapacities"]);
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
    std::vector<std::array<unsigned, 2>> highresinterval =
        as_vector<std::array<unsigned, 2>>(cfg["highresinterval"]);

    std::reverse(highresinterval.begin(), highresinterval.end());
    // check that the initialization of model,  cells and agent was correct
    // check model parameters
    ASSERT_EQ(model.get_highresinterval(), highresinterval);
    ASSERT_EQ(model.get_decay(), decay);
    ASSERT_EQ(model.get_construction(), construction);
    ASSERT_EQ(model.cells().size(), std::size_t(1024));
    // check agent parameters
    ASSERT_EQ(int(agents.size()), 1);
    ASSERT_EQ(agents[0]->state().age, std::size_t(0));
    ASSERT_EQ(agents[0]->state().fitness, 0.);

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

            ASSERT_EQ(state.resourceinflux, init_cell_resourceinflux_values);
            ASSERT_EQ(state.resource_capacity, cell_resourcecapacities);

            ASSERT_EQ(state.celltrait, state.original);
            ASSERT_EQ(state.modtimes, std::vector<double>(state.celltrait.size(), 0.));
        }
    }
    ASSERT_EQ(model.get_removethreshold(), removethreshold);
    ASSERT_EQ(model.get_decayintensity(), decayintensity);
}

template <typename Model, typename Cellmanager>
void test_model_functions(Model& model, Cellmanager& cellmanager)
{
    auto& agents = model.population();

    auto adam = agents[0];
    auto& adamstate = adam->state();
    auto eden = adamstate.habitat;
    auto& edenstate = eden->state();

    // backup
    auto adamphenotype = adamstate.phenotype;

    auto edentrait = edenstate.celltrait;

    ////////////////////////////////////////////////////////////////////////////
    // update adaption
    ////////////////////////////////////////////////////////////////////////////

    adamstate.phenotype = std::vector<double>{1., 2., -1., 2., 4.};
    edenstate.celltrait = std::vector<double>{-1., 1., 2., 1., 3.};
    edenstate.resources = std::vector<double>{1., 1., 1., 1., 1.};
    edenstate.resourceinflux = std::vector<double>{1., 1., 1., 1., 1.};
    adamstate.start = 0;
    adamstate.end = 5;
    adamstate.adaption = {0., 0., 0., 0., 0.};
    model.update_adaption(*adam);
    ASSERT_EQ(adamstate.adaption, (std::vector<double>{0., 1., 0., 1., 6.}));

    ////////////////////////////////////////////////////////////////////////////
    // metabolism
    ////////////////////////////////////////////////////////////////////////////

    adamstate.resources = 0.;
    edenstate.resources = std::vector<double>(5, 10.);
    model.metabolism(*adam);
    ASSERT_EQ(adamstate.resources, 3.);
    ASSERT_EQ(edenstate.resources, (std::vector<double>{10., 9., 10., 9., 4.}));
    ASSERT_EQ(int(adamstate.age), 1);

    // metabolism values odd: too few resources on cell
    edenstate.resources = std::vector<double>{2., 2., 2., 2., 2.};
    adamstate.adaption = std::vector<double>{8., 8., 8., 8., 8.};
    adamstate.resources = 5.;
    model.metabolism(*adam);
    ASSERT_EQ(adamstate.resources, 10.);
    ASSERT_EQ(edenstate.resources, (std::vector<double>{0., 0., 0, 0, 0}));
    ASSERT_EQ(int(adamstate.age), 2);

    // metabolism values odd: too much adaption -> limited by upper influx
    edenstate.resources = std::vector<double>{20., 20., 20., 20., 20.};
    adamstate.adaption = std::vector<double>{20., 20., 20., 20., 20.};
    adamstate.resources = 5.;
    model.metabolism(*adam);
    ASSERT_EQ(adamstate.resources, 75.);
    ASSERT_EQ(edenstate.resources, (std::vector<double>{5., 5., 5, 5, 5}));
    ASSERT_EQ(int(adamstate.age), 3);

    ////////////////////////////////////////////////////////////////////////////
    // move
    ////////////////////////////////////////////////////////////////////////////

    auto neighbors = Utopia::Neighborhoods::MooreNeighbor::neighbors(eden, cellmanager);
    edenstate.celltrait = std::vector<double>(8, 0.);
    edenstate.resourceinflux = std::vector<double>(8, 10.);
    edenstate.resources = std::vector<double>(8, 10.);
    adamstate.phenotype = std::vector<double>(8, 1);
    adamstate.start = 1;
    adamstate.end = 5;
    adamstate.resources = 0.5;
    adamstate.adaption = std::vector<double>(4, 0.);
    model.update_adaption(*adam);

    // directed movement
    for (auto& neighbor : neighbors)
    {
        neighbor->state().celltrait = std::vector<double>(8, 0);
        neighbor->state().resources = std::vector<double>(8, 10.);
        neighbor->state().resourceinflux = std::vector<double>(8, 10.);
    }

    neighbors[2]->state().celltrait = adamstate.phenotype;

    model.move(*adam);
    model.update_adaption(*adam);

    assert(neighbors[2].get() == adamstate.habitat.get());
    ASSERT_EQ(adamstate.adaption, (std::vector<double>{1, 1, 1, 1}));

    eden = adamstate.habitat;
    edenstate = eden->state();
    adamstate = adam->state();

    // random movement
    adamstate.resources = 0.5; // has to move
    edenstate.celltrait = std::vector<double>(8, 1.);
    edenstate.resourceinflux = std::vector<double>(8, 10.);
    edenstate.resources = std::vector<double>(8, 10.);
    adamstate.phenotype = std::vector<double>(8, 1.);
    neighbors = Utopia::Neighborhoods::MooreNeighbor::neighbors(eden, cellmanager);
    adamstate.start = 1;
    adamstate.end = 5;
    adamstate.resources = 0.5;
    adamstate.adaption = std::vector<double>(4, 0.);
    model.update_adaption(*adam);

    for (auto& neighbor : neighbors)
    {
        neighbor->state().celltrait = std::vector<double>(8, 1.);
        neighbor->state().resourceinflux = std::vector<double>(8, 10.);
        neighbor->state().resources = std::vector<double>(8, 10.);
    }
    model.update_adaption(*adam);
    model.move(*adam);
    model.update_adaption(*adam);

    assert(adamstate.habitat != eden);
    for (auto& neighbor : neighbors)
    {
        neighbor->state().celltrait = std::vector<double>(8, 1.);
    }
    std::cout << adamstate.phenotype << ","
              << adamstate.habitat->state().celltrait << std::endl;

    ASSERT_EQ(adamstate.adaption, (std::vector<double>{1., 1., 1., 1.}))

    eden = adamstate.habitat;
    edenstate = eden->state();

    ////////////////////////////////////////////////////////////////////////////
    // modify
    ////////////////////////////////////////////////////////////////////////////
    adamstate.intensity = 0.5;
    adamstate.start = 2;
    adamstate.end = 5;
    adamstate.start_mod = 2;
    adamstate.end_mod = 5;
    model.increment_time(1);
    adam->state().habitat->state().celltrait = std::vector<double>(6, 6.);
    adam->state().habitat->state().resources = std::vector<double>(6, 1.);
    adam->state().habitat->state().resourceinflux = std::vector<double>(6, 1.);
    adam->state().habitat->state().modtimes = std::vector<double>(6, 0.);
    adamstate.phenotype = std::vector<double>(6, 4.);
    adamstate.resources = 10.;
    model.set_modifiercost(0.1);

    model.modify(*adam);

    ASSERT_EQ(adam->state().habitat->state().celltrait,
              (std::vector<double>{6., 6., 2., 2., 2., 6.}));

    ASSERT_EQ(adamstate.resources, 8.8);
    ASSERT_EQ(adam->state().habitat->state().modtimes,
              (std::vector<double>{0., 0., 1., 1., 1., 0.}));

    adamstate.end = 8;
    adamstate.end_mod = 8;
    adamstate.phenotype = std::vector<double>(8, 4.);
    adam->state().habitat->state().celltrait = std::vector<double>(6, 6.);
    adam->state().habitat->state().resources = std::vector<double>(6, 1.);
    adam->state().habitat->state().resourceinflux = std::vector<double>(6, 1.);
    adam->state().habitat->state().modtimes = std::vector<double>(6, 0.);
    adam->state().resources = 10.;
    model.set_modifiercost(0.1);

    model.modify(*adam);

    ASSERT_EQ(adam->state().habitat->state().celltrait,
              (std::vector<double>{6., 6., 2., 2., 2., 2., 2., 2.}));
    ASSERT_EQ(adam->state().habitat->state().modtimes,
              (std::vector<double>{0., 0., 1., 1., 1., 1., 1., 1.}));
    ASSERT_EQ(adamstate.resources, 8.);

    // test bad values -> should do nothing and hence everything is as it was
    adam->state().end = adam->state().start;
    adam->state().end_mod = adam->state().start_mod;

    model.modify(*adam);
    ASSERT_EQ(adam->state().habitat->state().celltrait,
              (std::vector<double>{6., 6., 2., 2., 2., 2., 2., 2.}));
    ASSERT_EQ(adamstate.resources, 8.);
    ASSERT_EQ(adam->state().habitat->state().modtimes,
              (std::vector<double>{0., 0., 1., 1., 1., 1., 1., 1.}));

    // cannot afford modification - internally
    adamstate.intensity = 2.;
    adamstate.start = 2;
    adamstate.end = 5;
    adamstate.start_mod = 2;
    adamstate.end_mod = 5;

    adamstate.phenotype = std::vector<double>(6, 2.);
    adamstate.habitat->state().celltrait = std::vector<double>(6, 6.);
    adam->state().habitat->state().resources = std::vector<double>(6, 1.);
    adam->state().habitat->state().resourceinflux = std::vector<double>(6, 1.);
    adam->state().habitat->state().modtimes = std::vector<double>(6, 0.);
    adamstate.resources = 10.;
    model.set_modifiercost(2.);
    model.modify(*adam);
    ASSERT_EQ(adam->state().habitat->state().celltrait,
              (std::vector<double>{6., 6., 4., 4., 6., 6.}));

    ASSERT_EQ(adamstate.resources, 2.);
    ASSERT_EQ(adam->state().habitat->state().modtimes,
              (std::vector<double>{0., 0., 1., 1., 0., 0.}));

    // cannot afford modification - beyond celltraitlength
    model.increment_time();
    adamstate.start = 2;
    adamstate.end = 8;
    adamstate.start_mod = 2;
    adamstate.end_mod = 8;

    adamstate.intensity = 2;
    adamstate.phenotype = std::vector<double>(8, 2.);

    adam->state().habitat->state().celltrait = std::vector<double>(6, 6.);
    adam->state().habitat->state().resources = std::vector<double>(6, 1.);
    adam->state().habitat->state().resourceinflux = std::vector<double>(6, 1.);
    adam->state().resources = 15.;
    model.set_modifiercost(1);

    model.modify(*adam);

    ASSERT_EQ(adamstate.resources, 3.);
    ASSERT_EQ(adam->state().habitat->state().resourceinflux.size(), (unsigned long)7);

    ASSERT_EQ(adam->state().habitat->state().celltrait,
              (std::vector<double>{6., 6., 4., 4., 4., 4., 4.}));
    ASSERT_EQ(adam->state().habitat->state().modtimes,
              (std::vector<double>{0., 0., 2., 2., 2., 2., 2.}));

    // modify, [start, end) and [start_mod, end_mod) overlap only partially
    // overlap
    adamstate.start = 2;
    adamstate.end = 8;
    adamstate.start_mod = 6;
    adamstate.end_mod = 9;

    adamstate.intensity = 1;
    adamstate.phenotype = std::vector<double>(15, 4.);

    adam->state().habitat->state().celltrait = std::vector<double>(11, 6.);
    adam->state().habitat->state().resources = std::vector<double>(11, 1.);
    adam->state().habitat->state().resourceinflux = std::vector<double>(11, 1.);
    adam->state().habitat->state().modtimes = std::vector<double>(11, 0.);
    adam->state().resources = 15.;
    model.set_modifiercost(0.5);
    adam->state().adaption = std::vector<double>(6, 0.);

    model.modify(*adam);
    model.update_adaption(*adam);

    ASSERT_EQ(adam->state().habitat->state().celltrait,
              (std::vector<double>{6., 6., 6., 6., 6., 6., 4., 4., 4., 6., 6.}));
    ASSERT_EQ(adam->state().habitat->state().modtimes,
              (std::vector<double>{0., 0., 0., 0., 0., 0., 2., 2., 2., 0., 0.}));
    ASSERT_EQ(adamstate.resources, 12.);
    ASSERT_EQ(adamstate.adaption, (std::vector<double>{8., 8., 8., 8., 16., 16.}));

    // modify, [start, end) and [start_mod, end_mod) overlap only partially
    // overlap but end_mod goes beyond celltraitlength
    adamstate.start = 2;
    adamstate.end = 8;
    adamstate.start_mod = 6;
    adamstate.end_mod = 12;

    adamstate.intensity = 1;
    adamstate.phenotype = std::vector<double>(15, 4.);

    adam->state().habitat->state().celltrait = std::vector<double>(9, 6.);
    adam->state().habitat->state().celltrait[7] =
        std::numeric_limits<double>::quiet_NaN(); // add this for testing nan
    adam->state().habitat->state().resources = std::vector<double>(9, 1.);
    adam->state().habitat->state().resourceinflux = std::vector<double>(9, 1.);
    adam->state().habitat->state().modtimes = std::vector<double>(9, 0.);
    adam->state().resources = 15.;
    model.set_modifiercost(0.5);
    adam->state().adaption = std::vector<double>(6, 0.);

    model.modify(*adam);
    model.update_adaption(*adam);

    ASSERT_EQ(adam->state().habitat->state().celltrait,
              (std::vector<double>{6., 6., 6., 6., 6., 6., 4., 4., 4., 4., 4., 4.}));

    ASSERT_EQ(adam->state().habitat->state().modtimes,
              (std::vector<double>{0., 0., 0., 0., 0., 0., 2., 2., 2., 2., 2., 2.}));
    ASSERT_EQ(adamstate.resources, 5.);
    ASSERT_EQ(adamstate.adaption, (std::vector<double>{8., 8., 8., 8., 16., 16.}));

    // reset time again for later reference
    model.set_time(0);

    ////////////////////////////////////////////////////////////////////////////
    // reproduce
    ////////////////////////////////////////////////////////////////////////////

    adam->state().resources = 10;
    model.reproduce(*adam);
    ASSERT_EQ(adam->state().fitness, 4.);
    ASSERT_EQ(model.population().size(), std::size_t(5));
    ASSERT_EQ(adam->state().resources, 2.);
    for (std::size_t i = 1; i < model.population().size(); ++i)
    {
        ASSERT_EQ(model.population()[i]->state().resources, 1.);
        assert(model.population()[i]->state().habitat == adam->state().habitat);
        ASSERT_EQ(model.population()[i]->state().fitness, 0.);
        ASSERT_EQ(model.population()[i]->state().age, std::size_t(0));
    }

    ////////////////////////////////////////////////////////////////////////////
    // kill
    ////////////////////////////////////////////////////////////////////////////

    auto deadmanwalking = model.population().back();
    deadmanwalking->state().resources = 0.;
    ASSERT_EQ(deadmanwalking->state().deathflag, false);
    model.kill(*deadmanwalking);
    ASSERT_EQ(deadmanwalking->state().deathflag, true);

    ////////////////////////////////////////////////////////////////////////////
    // update cell
    ////////////////////////////////////////////////////////////////////////////

    auto cell = model.cells().front();
    cell->state().celltrait = std::vector<double>(10, 1.);
    cell->state().original = std::vector<double>(10, 1.);
    cell->state().resourceinflux =
        std::vector<double>(cell->state().celltrait.size(), 10);

    // if has no resources, is set to resourceinfluxes
    cell->state().resources = std::vector<double>(cell->state().celltrait.size(), 0);

    model.update_cell(cell);
    ASSERT_EQ(cell->state().resources, std::vector<double>(10, 10.));

    // if has resources is set to logisitc function with t = 1 and u0 = current
    // resources: u(t) = (K*u(t-1)*exp(-rt))/(K + u(t-1)*(exp(-rt) - 1))

    cell->state().resources = std::vector<double>(cell->state().celltrait.size(), 1.5);
    cell->state().resourceinflux =
        std::vector<double>(cell->state().celltrait.size(), 3.);
    cell->state().resource_capacity =
        std::vector<double>(cell->state().celltrait.size(), 50.);
    model.update_cell(cell);

    ASSERT_EQ(cell->state().resources, std::vector<double>(10, 19.15868925150003));

    for (std::size_t i = 0; i < 100; ++i)
    {
        model.update_cell(cell);
    }
    ASSERT_EQ(cell->state().resources, std::vector<double>(10, 50.));

    ////////////////////////////////////////////////////////////////////////////
    // decay_celltrait
    ////////////////////////////////////////////////////////////////////////////
    model.set_time(0);
    ASSERT_EQ(model.get_time(), std::size_t(0)); // test if setting works out
    cell->state().celltrait = std::vector<double>(7, 5.);
    cell->state().original = std::vector<double>(5, 1.);
    cell->state().modtimes = std::vector<double>(7, 2);
    cell->state().resources = std::vector<double>(7, 2);
    cell->state().resourceinflux = std::vector<double>(7, 5);

    model.increment_time(5);
    ASSERT_EQ(model.get_time(), std::size_t(5));

    model.set_decayintensity(0.5);
    model.celltrait_decay(cell);
    ASSERT_EQ(cell->state().celltrait,
              (std::vector<double>{1.892520640593719, 1.892520640593719,
                                   1.892520640593719, 1.892520640593719, 1.892520640593719,
                                   1.115650800742149, 1.115650800742149}));

    // decay until the first added locus is removed
    model.increment_time(2);
    model.set_decayintensity(2.5);

    cell->state().celltrait = std::vector<double>(7, 5.);
    cell->state().original = std::vector<double>(5, 1.);
    cell->state().modtimes = std::vector<double>{4, 4, 4, 4, 4, 1, 4};
    cell->state().resources = std::vector<double>(7, 2);
    cell->state().resourceinflux = std::vector<double>(7, 5);
    model.celltrait_decay(cell);

    for (std::size_t i = 0; i < 5; ++i)
    {
        ASSERT_EQ(cell->state().celltrait[i], 1.002212337480591);
        ASSERT_EQ(cell->state().modtimes[i], 4.);
    }

    ASSERT_EQ(std::isnan(cell->state().celltrait[5]), true);
    ASSERT_EQ(std::isnan(cell->state().modtimes[5]), true);

    ASSERT_EQ(cell->state().celltrait[6], 0.002765421850739168);
    ASSERT_EQ(cell->state().modtimes[6], 4.);

    ASSERT_EQ(cell->state().resourceinflux, (std::vector<double>{5, 5, 5, 5, 5, 0, 5}));
    ASSERT_EQ(cell->state().resources, std::vector<double>(7, 2));
}

// build a struct which makes setup simpler
template <template <typename, typename, typename> class AgentPolicy, typename G, typename P, typename RNG, template <typename> class Adaptionfunc, bool construction, bool decay>
struct Modelfactory
{
    template <typename Model, typename Cellmanager>
    auto operator()(std::string name, Model& parentmodel, Cellmanager& cellmanager, std::string adaptionfunctionname)
    {
        using CellType = typename Cellmanager::Cell;
        using Genotype = G;
        using Phenotype = P;
        using Policy = AgentPolicy<Genotype, Phenotype, RNG>;
        using Agentstate =
            Utopia::Models::Amee::AgentState<CellType, Policy, std::vector<double>>;
        using Position = Dune::FieldVector<double, 2>;
        using AgentType =
            Utopia::Agent<Agentstate, Utopia::EmptyTag, std::size_t, Position>;

        std::map<std::string, Adaptionfunc<AgentType>> adaptionfunctionmap = {
            {"multi_notnormed", multi_notnormed},
            {"multi_normed", multi_normed},
            {"simple_notnormed", simple_notnormed},
            {"simple_normed", simple_normed}};

        using AgentAdaptor = std::function<double(std::shared_ptr<AgentType>)>;
        using AgentAdaptortuple = std::tuple<std::string, AgentAdaptor>;

        using CellAdaptor = std::function<double(const std::shared_ptr<CellType>)>;
        using CellAdaptortuple = std::tuple<std::string, CellAdaptor>;

        std::vector<AgentAdaptortuple> agentadaptors{AgentAdaptortuple{
            "accumulated_adaption", [](const auto& agent) -> double {
                return std::accumulate(agent->state().adaption.begin(),
                                       agent->state().adaption.end(), 0.);
            }}};

        std::vector<CellAdaptortuple> celladaptors{CellAdaptortuple{
            "resources", [](const auto& cell) -> double {
                return std::accumulate(cell->state().resources.begin(),
                                       cell->state().resources.end(), 0.);
            }}};
        // making model types
        using Modeltypes = Utopia::ModelTypes<RNG>;

        return AmeeMulti<CellType, AgentType, Modeltypes, Adaptionfunc<AgentType>, construction, decay>(
            name, parentmodel, cellmanager.cells(),
            adaptionfunctionmap[adaptionfunctionname], agentadaptors, celladaptors);
    }
};

template <typename Agent>
using Adaptionfunction = std::function<std::vector<double>(Agent&)>;

int main(int argc, char** argv)
{
    Dune::MPIHelper::instance(argc, argv);

    try
    {
        using RNG = Utopia::Models::Amee::Xoroshiro512starstar;
        using CT = std::vector<double>;
        using CS = Utopia::Models::Amee::Cellstate<CT, std::vector<double>>;

        for (auto cfg :
             {"multi_test_config_simple.yml", "multi_test_config_complex.yml"})
        {
            std::cout << cfg << std::endl;
            // Initialize the PseudoParent from config file path
            Utopia::PseudoParent<RNG> pp(cfg);

            // make managers first -> this has to be wrapped in a factory function
            auto cellmanager = Utopia::Models::Amee::Setup::create_grid_manager_cells<
                Utopia::Models::Amee::StaticCell, CS, true, 2, true, false>(
                "AmeeMulti", pp);

            // read stuff from the config
            bool construction =
                Utopia::as_bool(pp.get_cfg()["AmeeMulti"]["construction"]);

            bool decay = Utopia::as_bool(pp.get_cfg()["AmeeMulti"]["decay"]);

            std::string agenttype =
                Utopia::as_str(pp.get_cfg()["AmeeMulti"]["agenttype"]);

            std::string adaptionfunction =
                Utopia::as_str(pp.get_cfg()["AmeeMulti"]["adaptionfunction"]);

            if (std::make_tuple(construction, decay, agenttype) ==
                std::tuple<bool, bool, std::string>{true, true, "simple"})
            {
                using Genotype = std::vector<double>;
                using Phenotype = std::vector<double>;
                Modelfactory<Utopia::Models::Amee::Agentstate_policy_simple, Genotype, Phenotype, RNG, Adaptionfunction, true, true> factory;

                auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                test_model_construction(model);
                test_model_functions(model, cellmanager);
            }
            else if (std::make_tuple(construction, decay, agenttype) ==
                     std::tuple<bool, bool, std::string>{true, true, "complex"})
            {
                using Genotype = std::vector<int>;
                using Phenotype = std::vector<double>;
                Modelfactory<Utopia::Models::Amee::Agentstate_policy_complex, Genotype, Phenotype, RNG, Adaptionfunction, true, true> factory;

                auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                test_model_construction(model);
                test_model_functions(model, cellmanager);
            }
            else if (std::make_tuple(construction, decay, agenttype) ==
                     std::tuple<bool, bool, std::string>{true, false, "simple"})
            {
                using Genotype = std::vector<double>;
                using Phenotype = std::vector<double>;
                Modelfactory<Utopia::Models::Amee::Agentstate_policy_simple, Genotype, Phenotype, RNG, Adaptionfunction, true, false> factory;

                auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                test_model_construction(model);
                test_model_functions(model, cellmanager);
            }
            else if (std::make_tuple(construction, decay, agenttype) ==
                     std::tuple<bool, bool, std::string>{false, false,
                                                         "complex"})
            {
                using Genotype = std::vector<int>;
                using Phenotype = std::vector<double>;
                Modelfactory<Utopia::Models::Amee::Agentstate_policy_complex, Genotype, Phenotype, RNG, Adaptionfunction, false, false> factory;

                auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                test_model_construction(model);
                test_model_functions(model, cellmanager);
            }
        }
        return 0;
    }
    catch (std::exception& e)
    {
        std::cerr << "An exception occured: " << e.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        std::cerr << "An unidentifiable exception occured" << std::endl;
        return -1;
    }
}
