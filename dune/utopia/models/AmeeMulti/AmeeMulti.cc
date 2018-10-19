#include <dune/utopia/core/tags.hh>
#include <dune/utopia/models/AmeeMulti/AmeeMulti.hh>
#include <dune/utopia/models/AmeeMulti/agentstates/agentstate.hh>
#include <dune/utopia/models/AmeeMulti/agentstates/agentstate_policy_complex.hh>
#include <dune/utopia/models/AmeeMulti/agentstates/agentstate_policy_simple.hh>
#include <dune/utopia/models/AmeeMulti/utils/custom_cell.hh>
#include <dune/utopia/models/AmeeMulti/utils/custom_setup.hh>
#include <dune/utopia/models/AmeeMulti/utils/generators.hh>
#include <iostream>
#include <thread>

using namespace Utopia::Models::AmeeMulti;
using namespace std::literals::chrono_literals;

// build a struct which makes setup simpler
template <template <typename, typename, typename> class AgentPolicy, typename G, typename P, typename RNG, bool construction, bool decay>
struct Modelfactory
{
    template <typename Model, typename Cellmanager>
    auto operator()(std::string name, Model& parentmodel, Cellmanager& cellmanager)
    {
        using CellType = typename Cellmanager::Cell;
        using Genotype = G;
        using Phenotype = P;
        using Policy = AgentPolicy<Genotype, Phenotype, RNG>;
        using Agentstate = AgentState<CellType, Policy>;
        using Position = Dune::FieldVector<double, 2>;
        using AgentType =
            Utopia::Agent<Agentstate, Utopia::EmptyTag, std::size_t, Position>;

        // making model types
        using Modeltypes = Utopia::ModelTypes<RNG>;

        return AmeeMulti<CellType, AgentType, Modeltypes, construction, decay>(
            name, parentmodel, cellmanager.cells());
    }
};

int main(int argc, char** argv)
{
    Dune::MPIHelper::instance(argc, argv);

    try
    {
        using RNG = Xoroshiro<>;
        using Celltraits = std::vector<double>;
        using Cellstate = Cellstate<Celltraits>;

        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent<RNG> pp(argv[1]);

        // make managers first -> this has to be wrapped in a factory function
        auto cellmanager = Utopia::Models::AmeeMulti::Setup::create_grid_manager_cells<
            Utopia::Models::AmeeMulti::StaticCell, Cellstate, true, 2, true, false>(
            "AmeeMulti", pp);

        // read stuff from the config
        bool construction =
            Utopia::as_bool(pp.get_cfg()["AmeeMulti"]["construction"]);
        bool decay = Utopia::as_bool(pp.get_cfg()["AmeeMulti"]["decay"]);
        std::string agenttype =
            Utopia::as_str(pp.get_cfg()["AmeeMulti"]["Agenttype"]);

        if (std::make_tuple(construction, decay, agenttype) ==
            std::tuple<bool, bool, std::string>{true, true, "simple"})
        {
            using Genotype = std::vector<int>;
            using Phenotype = std::vector<double>;
            auto model =
                Modelfactory<Agentstate_policy_simple, Genotype, Phenotype, RNG, true, true>()(
                    "AmeeMulti", pp, cellmanager);
            model.run();
        }
        else if (std::make_tuple(construction, decay, agenttype) ==
                 std::tuple<bool, bool, std::string>{true, true, "complex"})
        {
            using Genotype = std::vector<double>;
            using Phenotype = std::vector<double>;
            auto model =
                Modelfactory<Agentstate_policy_complex, Genotype, Phenotype, RNG, true, true>()(
                    "AmeeMulti", pp, cellmanager);
            model.run();
        }
        else if (std::make_tuple(construction, decay, agenttype) ==
                 std::tuple<bool, bool, std::string>{true, false, "simple"})
        {
            using Genotype = std::vector<int>;
            using Phenotype = std::vector<double>;
            auto model =
                Modelfactory<Agentstate_policy_simple, Genotype, Phenotype, RNG, true, false>()(
                    "AmeeMulti", pp, cellmanager);
            model.run();
        }
        else if (std::make_tuple(construction, decay, agenttype) ==
                 std::tuple<bool, bool, std::string>{false, false, "complex"})
        {
            using Genotype = std::vector<double>;
            using Phenotype = std::vector<double>;
            auto model =
                Modelfactory<Agentstate_policy_complex, Genotype, Phenotype, RNG, false, false>()(
                    "AmeeMulti", pp, cellmanager);
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
