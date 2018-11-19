#include <dune/utopia/core/tags.hh>
#include <dune/utopia/models/Amee/adaptionfunctions.hh>
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
using namespace Utopia::Models::Amee;
using namespace std::literals::chrono_literals;

// build a struct which makes setup simpler
template <template <typename Gt, typename Pt, typename RNG> class AgentPolicy,
          template <typename, bool, bool> class Adaptionfunction,
          typename G,
          typename P,
          typename RNG,
          typename Adaptiontype,
          bool construction,
          bool decay,
          bool multi,
          bool normed>
struct Modelfactory
{
    template <typename Model, typename Cellmanager>
    auto operator()(std::string name, Model& parentmodel, Cellmanager& cellmanager, std::string adaptionfunctionname)
    {
        using namespace Utopia;
        // making model types
        using Genotype = G;
        using Phenotype = P;
        using Policy = AgentPolicy<Genotype, Phenotype, RNG>;
        using Position = Dune::FieldVector<double, 2>;
        using CellType = typename Cellmanager::Cell;

        using Agentstate = AgentState<CellType, Policy, Adaptiontype>;
        using AgentType = Agent<Agentstate, EmptyTag, std::size_t, Position>;

        using Modeltypes = ModelTypes<RNG>;

        using Modeltraits =
            Modeltraits<CellType, AgentType, Adaptionfunction, Adaptiontype, construction, decay, multi, normed>;

        using AgentAdaptor = typename Modeltraits::AgentAdaptor;
        using AgentAdaptortuple = std::tuple<std::string, AgentAdaptor>;

        using CellAdaptor = typename Modeltraits::CellAdaptor;

        using CellAdaptortuple = std::tuple<std::string, CellAdaptor>;

        std::vector<AgentAdaptortuple> agentadaptors{
            AgentAdaptortuple{"cummulative_adaption",
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
            CellAdaptortuple{"resourceinflux",
                             [](const auto& cell) -> double {
                                 return std::accumulate(
                                     cell->state().resourceinflux.begin(),
                                     cell->state().resourceinflux.end(), 0.);
                             }},
            CellAdaptortuple{"cell_traitlen", [](const auto& cell) -> double {
                                 return cell->state().celltrait.size();
                             }}};

        return AmeeMulti<Modeltraits, Modeltypes>(
            name, parentmodel, cellmanager.cells(), agentadaptors, celladaptors);
    }
};

// type aliases for later usage
using RNG = Utils::Xoroshiro512starstar;
using Celltrait = std::vector<double>;
using Resourcetype = std::vector<double>;
using Adaptiontype = std::vector<double>;
using CS = Cellstate<Celltrait, Resourcetype>;

template <typename Adaptiontype, bool multi, bool normed>
using Adaptiongenerator = SchurproductLike<Adaptiontype, multi, normed>;

int main(int argc, char** argv)
{
    Dune::MPIHelper::instance(argc, argv);

    try
    {
        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent<RNG> pp(argv[1]);

        // make managers first -> this has to be wrapped in a factory function
        auto cellmanager =
            Setup::create_grid_manager_cells<StaticCell, CS, true, 2, true, false>(
                "AmeeMulti", pp);

        // read stuff from the config
        bool construction =
            Utopia::as_bool(pp.get_cfg()["AmeeMulti"]["construction"]);

        bool decay = Utopia::as_bool(pp.get_cfg()["AmeeMulti"]["decay"]);

        bool multi = Utopia::as_bool(
            pp.get_cfg()["AmeeMulti"]["adaptionfunction"]["multi"]);

        bool normed = Utopia::as_bool(
            pp.get_cfg()["AmeeMulti"]["adaptionfunction"]["normed"]);

        std::string agenttype =
            Utopia::as_str(pp.get_cfg()["AmeeMulti"]["agenttype"]);

        std::string adaptionfunction =
            Utopia::as_str(pp.get_cfg()["AmeeMulti"]["adaptionfunction"]);

        if (agenttype == "simple")
        {
            if (construction && decay)
            {
                if (multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;

                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, true, true, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }

                else if (multi && !normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, true, true, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else if (!multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, true, false, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, true, false, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
            }
            else if (construction && !decay)
            {
                if (multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, false, true, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else if (multi && !normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, false, true, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else if (!multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, false, false, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, false, false, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
            }
            else if (!construction && decay)
            {
                if (multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, false, true, true, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else if (multi && !normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, false, true, true, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else if (!multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, false, true, false, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, false, true, false, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
            }
            else
            {
                if (multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, false, false, true, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else if (multi && !normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, true, true, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else if (!multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, false, false, false, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_simple, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, false, false, false, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
            }
        }
        else if (agenttype == "complex")
        {
            if (construction && decay)
            {
                if (multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, true, true, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else if (multi && !normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, true, true, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else if (!multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, true, false, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, true, false, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
            }
            else if (construction && !decay)
            {
                if (multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, false, true, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else if (multi && !normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, false, true, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else if (!multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, false, false, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, false, false, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
            }
            else if (!construction && decay)
            {
                if (multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, false, true, true, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else if (multi && !normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, false, true, true, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else if (!multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, false, true, false, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, false, true, false, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
            }
            else
            {
                if (multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, false, false, true, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else if (multi && !normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, true, true, true, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else if (!multi && normed)
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, false, false, false, true>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
                else
                {
                    using Genotype = std::vector<double>;
                    using Phenotype = std::vector<double>;
                    Modelfactory<Agentstate_policy_complex, Adaptiongenerator, Genotype,
                                 Phenotype, RNG, Adaptiontype, false, false, false, false>
                        factory;

                    auto model = factory("AmeeMulti", pp, cellmanager, adaptionfunction);
                    model.run();
                }
            }
        }
        else
        {
            throw std::runtime_error("unknown agenttype");
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