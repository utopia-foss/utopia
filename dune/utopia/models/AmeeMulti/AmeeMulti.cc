#include <dune/utopia/core/tags.hh>
#include <dune/utopia/models/Amee/agentpolicies/agent_updatepolicy.hh>
#include <dune/utopia/models/Amee/agentstates/agentstate.hh>
#include <dune/utopia/models/Amee/agentstates/agentstate_policy_complex.hh>
#include <dune/utopia/models/Amee/agentstates/agentstate_policy_simple.hh>
#include <dune/utopia/models/Amee/utils/custom_cell.hh>
#include <dune/utopia/models/Amee/utils/custom_setup.hh>
#include <dune/utopia/models/Amee/utils/generators.hh>
#include <dune/utopia/models/AmeeMulti/AmeeMulti.hh>

#include <iostream>
#include <thread>

using namespace Utopia::Models::AmeeMulti;
using namespace std::literals::chrono_literals;

// build a struct which makes setup simpler
template <template <typename, typename, typename> class AgentPolicy,
          typename G,
          typename P,
          typename RNG,
          template <typename> class Adaptionfunc,
          template <typename, typename, typename, typename> class Agentupdatepolicy,
          bool construction,
          bool decay>
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

        using AgentAdaptor = std::function<double(const std::shared_ptr<AgentType>&)>;
        using AgentAdaptortuple = std::tuple<std::string, AgentAdaptor>;

        using CellAdaptor = std::function<double(const std::shared_ptr<CellType>)>;
        using CellAdaptortuple = std::tuple<std::string, CellAdaptor>;

        std::vector<AgentAdaptortuple> agentadaptors{
            AgentAdaptortuple{"accumulated_adaption",
                              [](const auto& agent) -> double {
                                  return std::accumulate(
                                      agent->state().adaption.begin(),
                                      agent->state().adaption.end(), 0.);
                              }},
            AgentAdaptortuple{
                "sumlen",
                [](const auto& agent) -> double { return agent->state().sumlen; }},
            AgentAdaptortuple{"divisor",
                              [](const auto& agent) -> double {
                                  return agent->state().divisor;
                              }},
            AgentAdaptortuple{"intensity",
                              [](const auto& agent) -> double {
                                  return agent->state().intensity;
                              }},
            AgentAdaptortuple{
                "start",
                [](const auto& agent) -> double { return agent->state().start; }},
            AgentAdaptortuple{
                "end",
                [](const auto& agent) -> double { return agent->state().end; }},
            AgentAdaptortuple{"startmod",
                              [](const auto& agent) -> double {
                                  return agent->state().start_mod;
                              }},
            AgentAdaptortuple{"endmod",
                              [](const auto& agent) -> double {
                                  return agent->state().end_mod;
                              }},
            AgentAdaptortuple{"fitness",
                              [](const auto& agent) -> double {
                                  return agent->state().fitness;
                              }},
            AgentAdaptortuple{"cell_id",
                              [](const auto& agent) -> double {
                                  return agent->state().habitat->id();
                              }},
            AgentAdaptortuple{
                "age",
                [](const auto& agent) -> double { return agent->state().age; }},
            AgentAdaptortuple{"traitlen", [](const auto& agent) -> double {
                                  return agent->state().phenotype.size();
                              }}};

        std::vector<CellAdaptortuple> celladaptors{
            CellAdaptortuple{"resources",
                             [](const auto& cell) -> double {
                                 return std::accumulate(
                                     cell->state().resources.begin(),
                                     cell->state().resources.end(), 0.);
                             }},
            CellAdaptortuple{"resourceinfluxes",
                             [](const auto& cell) -> double {
                                 return std::accumulate(
                                     cell->state().resourceinflux.begin(),
                                     cell->state().resourceinflux.end(), 0.);
                             }},
            CellAdaptortuple{"cell_traitlen", [](const auto& cell) -> double {
                                 return cell->state().celltrait.size();
                             }}};
        // making model types
        using Modeltypes = Utopia::ModelTypes<RNG>;

        return AmeeMulti<CellType, AgentType, Modeltypes, Adaptionfunc<AgentType>,
                         RNG, Agentupdatepolicy, construction, decay>(
            name, parentmodel, cellmanager.cells(),
            adaptionfunctionmap[adaptionfunctionname], agentadaptors, celladaptors);
    }
};

// globally used typedefs for stuff
template <typename Agent>
using Adaptionfunction = std::function<std::vector<double>(Agent&)>;

template <typename Cell, typename Agent, typename Adaptionfunction, typename RNG>
using Updatepolicy =
    Utopia::Models::Amee::SequentialPolicy<Cell, Agent, Adaptionfunction, RNG>;

int main(int argc, char** argv)
{
    Dune::MPIHelper::instance(argc, argv);

    try
    {
        using RNG = Utopia::Models::Amee::Xoroshiro<>;
        using CT = std::vector<double>;
        using CS = Utopia::Models::Amee::Cellstate<CT, std::vector<double>>;

        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent<RNG> pp(argv[1]);

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
            Modelfactory<Utopia::Models::Amee::Agentstate_policy_simple, Genotype,
                         Phenotype, RNG, Adaptionfunction, Updatepolicy, true, true>
                factory;

            auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
            model.run();
        }
        else if (std::make_tuple(construction, decay, agenttype) ==
                 std::tuple<bool, bool, std::string>{true, true, "complex"})
        {
            using Genotype = std::vector<int>;
            using Phenotype = std::vector<double>;
            Modelfactory<Utopia::Models::Amee::Agentstate_policy_complex, Genotype,
                         Phenotype, RNG, Adaptionfunction, Updatepolicy, true, true>
                factory;

            auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
            model.run();
        }
        else if (std::make_tuple(construction, decay, agenttype) ==
                 std::tuple<bool, bool, std::string>{true, false, "simple"})
        {
            using Genotype = std::vector<double>;
            using Phenotype = std::vector<double>;
            Modelfactory<Utopia::Models::Amee::Agentstate_policy_simple, Genotype,
                         Phenotype, RNG, Adaptionfunction, Updatepolicy, true, false>
                factory;

            auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
            model.run();
        }
        else if (std::make_tuple(construction, decay, agenttype) ==
                 std::tuple<bool, bool, std::string>{false, false, "complex"})
        {
            using Genotype = std::vector<int>;
            using Phenotype = std::vector<double>;
            Modelfactory<Utopia::Models::Amee::Agentstate_policy_complex, Genotype,
                         Phenotype, RNG, Adaptionfunction, Updatepolicy, false, false>
                factory;

            auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
            model.run();
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
