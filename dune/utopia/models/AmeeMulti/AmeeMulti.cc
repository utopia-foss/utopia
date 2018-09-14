#include "AmeeMulti.hh"
#include "agentstates/agentstate.hh"
#include "agentstates/agentstate_policy_simple.hh"
#include "agentstates/agentstate_policy_complex.hh"
#include "agentstates/agentstate_policy_highlevel.hh"
#include "agentstates/agentstate_gauss.hh"

#include "utils/generators.hh"
#include <iostream>
#include <thread>
using namespace Utopia::Models::AmeeMulti;
using namespace std::literals::chrono_literals;

int main(int argc, char** argv)
{
    try
    {
        // std::this_thread::sleep_for(30s);

        Dune::MPIHelper::instance(argc, argv);

        using RNG = Xoroshiro<>;
        using Celltraits = std::vector<double>;
        using Cellstate = Cellstate<Celltraits>;
        using Genotype = std::vector<double>;
        using Phenotype = std::vector<double>;

        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent<RNG> pp(argv[1]);

        // make managers first -> this has to be wrapped in a factory function
        auto cellmanager =
            Utopia::Setup::create_grid_manager_cells<Cellstate, true, 2, true, false>(
                "AmeeMulti", pp);

        auto grid = cellmanager.grid();
        using GridType = typename decltype(grid)::element_type;
        using Cell = typename decltype(cellmanager)::Cell;

        Utopia::GridWrapper<GridType> wrapper{grid, cellmanager.extensions(),
                                              cellmanager.grid_cells()};

        // read stuff from the config
        bool construction =
            Utopia::as_bool(pp.get_cfg()["AmeeMulti"]["construction"]);
        bool decay = Utopia::as_bool(pp.get_cfg()["AmeeMulti"]["decay"]);
        std::string agenttype =
            Utopia::as_str(pp.get_cfg()["AmeeMulti"]["Agenttype"]);
        using Cellmanager = decltype(cellmanager);

        if (construction && decay)
        {
            if (agenttype == "simple")
            {
                using Policy = Agentstate_policy_simple<Genotype, Phenotype, RNG>;
                using Agentstate = AgentState<Cell, Genotype, Phenotype, RNG, Policy>;
                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));

                using Agentmanager = decltype(agentmanager);
                // making model types
                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, true, true>;

                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);
                // run model
                model.run();
            }
            if (agenttype == "gauss")
            {
                using Agentstate = AgentStateGauss<Cell, Phenotype, RNG>;
                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));
                using Agentmanager = decltype(agentmanager);
                // making model types
                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, true, true>;

                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);
                // run model
                model.run();
            }
            if (agenttype == "complex")
            {
                using Policy = Agentstate_policy_complex<Genotype, Phenotype, RNG>;
                using Agentstate = AgentState<Cell, Genotype, Phenotype, RNG, Policy>;

                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));
                using Agentmanager = decltype(agentmanager);
                // making model types
                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, true, true>;

                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);
                // run model
                model.run();
            }
            if (agenttype == "highlevel")
            {
                using Policy = Agentstate_policy_highlevel<Genotype, Phenotype, RNG>;
                using Agentstate = AgentState<Cell, Genotype, Phenotype, RNG, Policy>;
                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));
                using Agentmanager = decltype(agentmanager);
                // making model types
                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, true, true>;

                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);
                // run model
                model.run();
            }
        }
        else if (construction && !decay)
        {
            if (agenttype == "simple")
            {
                using Policy = Agentstate_policy_simple<Genotype, Phenotype, RNG>;
                using Agentstate = AgentState<Cell, Genotype, Phenotype, RNG, Policy>;
                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));

                using Agentmanager = decltype(agentmanager);

                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, true, false>;

                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

                // run model
                model.run();
            }
            if (agenttype == "gauss")
            {
                using Agentstate = AgentStateGauss<Cell, Phenotype, RNG>;
                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));

                using Agentmanager = decltype(agentmanager);

                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, true, false>;

                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

                // run model
                model.run();
            }
            if (agenttype == "complex")
            {
                using Policy = Agentstate_policy_complex<Genotype, Phenotype, RNG>;
                using Agentstate = AgentState<Cell, Genotype, Phenotype, RNG, Policy>;
                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));

                using Agentmanager = decltype(agentmanager);

                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, true, false>;

                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

                // run model
                model.run();
            }
            if (agenttype == "highlevel")
            {
                using Policy = Agentstate_policy_highlevel<Genotype, Phenotype, RNG>;
                using Agentstate = AgentState<Cell, Genotype, Phenotype, RNG, Policy>;
                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));

                using Agentmanager = decltype(agentmanager);

                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, true, false>;

                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

                // run model
                model.run();
            }
        }
        else if (!construction && decay)
        {
            if (agenttype == "simple")
            {
                using Policy = Agentstate_policy_simple<Genotype, Phenotype, RNG>;
                using Agentstate = AgentState<Cell, Genotype, Phenotype, RNG, Policy>;
                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));

                using Agentmanager = decltype(agentmanager);

                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, false, true>;

                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

                // run model
                model.run();
            }
            if (agenttype == "gauss")
            {
                using Agentstate = AgentStateGauss<Cell, Phenotype, RNG>;
                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));

                using Agentmanager = decltype(agentmanager);

                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, false, true>;

                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

                // run model
                model.run();
            }
            if (agenttype == "complex")
            {
                using Policy = Agentstate_policy_complex<Genotype, Phenotype, RNG>;
                using Agentstate = AgentState<Cell, Genotype, Phenotype, RNG, Policy>;
                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));

                using Agentmanager = decltype(agentmanager);

                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, false, true>;

                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

                // run model
                model.run();
            }
            if (agenttype == "highlevel")
            {
                using Policy = Agentstate_policy_highlevel<Genotype, Phenotype, RNG>;
                using Agentstate = AgentState<Cell, Genotype, Phenotype, RNG, Policy>;
                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));

                using Agentmanager = decltype(agentmanager);

                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, false, true>;

                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

                // run model
                model.run();
            }
        }
        else
        {
            if (agenttype == "simple")
            {
                using Policy = Agentstate_policy_simple<Genotype, Phenotype, RNG>;
                using Agentstate = AgentState<Cell, Genotype, Phenotype, RNG, Policy>;
                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));
                using Agentmanager = decltype(agentmanager);

                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, false, false>;
                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

                // run model
                model.run();
            }
            if (agenttype == "gauss")
            {
                using Agentstate = AgentStateGauss<Cell, Phenotype, RNG>;
                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));
                using Agentmanager = decltype(agentmanager);

                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, false, false>;

                // build model
                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

                // run model
                model.run();
            }
            if (agenttype == "complex")
            {
                using Policy = Agentstate_policy_complex<Genotype, Phenotype, RNG>;
                using Agentstate = AgentState<Cell, Genotype, Phenotype, RNG, Policy>;
                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));
                using Agentmanager = decltype(agentmanager);

                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, false, false>;

                // build model
                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

                // run model
                model.run();
            }
            if (agenttype == "highlevel")
            {
                using Policy = Agentstate_policy_highlevel<Genotype, Phenotype, RNG>;
                using Agentstate = AgentState<Cell, Genotype, Phenotype, RNG, Policy>;
                auto agentmanager = Utopia::Setup::create_manager_agents<true, true>(
                    wrapper, Utopia::Setup::create_agents_on_grid(wrapper, 1, Agentstate()));
                using Agentmanager = decltype(agentmanager);

                using Modeltypes =
                    Utopia::ModelTypes<std::pair<Cell, typename Agentmanager::Agent>, Utopia::BCDummy, RNG>;
                using AmeeMulti =
                    AmeeMulti<Cell, Cellmanager, Agentmanager, Modeltypes, false, false>;

                // build model
                AmeeMulti model("AmeeMulti", pp, cellmanager, agentmanager);

                // run model
                model.run();
            }
        }

        return 0;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Exception occured!" << std::endl;
        return 1;
    }
}
