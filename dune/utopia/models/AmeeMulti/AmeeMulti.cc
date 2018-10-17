#include <dune/utopia/core/tags.hh>
#include <dune/utopia/models/AmeeMulti/AmeeMulti.hh>
#include <dune/utopia/models/AmeeMulti/agentstates/agentstate.hh>
#include <dune/utopia/models/AmeeMulti/agentstates/agentstate_policy_complex.hh>
#include <dune/utopia/models/AmeeMulti/agentstates/agentstate_policy_simple.hh>
#include <dune/utopia/models/AmeeMulti/utils/custom_cell.hh>
#include <dune/utopia/models/AmeeMulti/utils/custom_setup.hh>
#include <dune/utopia/models/AmeeMulti/utils/generators.hh>
#include <dune/utopia/models/AmeeMulti/utils/memorypool.hh>
#include <iostream>
#include <thread>

using namespace Utopia::Models::AmeeMulti;
using namespace std::literals::chrono_literals;

// build a struct which makes setup simpler

template <template <typename, typename, typename> class AgentPolicy, typename G, typename P, typename RNG, bool construction, bool decay>
struct Modelfactory
{
    template <typename Model, typename Cellmanager>
    auto operator()(std::string name,
                    Model& parentmodel,
                    Cellmanager& cellmanager,
                    std::size_t mempoolsize = 1000000)
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
            name, parentmodel, cellmanager.cells(), mempoolsize);
    }
};

int main(int argc, char** argv)
{
    Dune::MPIHelper::instance(argc, argv);

    using RNG = Xoroshiro<>;
    using Celltraits = std::vector<double>;
    using Cellstate = Cellstate<Celltraits>;
    using Genotype = std::vector<double>;
    using Phenotype = std::vector<double>;

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
        auto model =
            Modelfactory<Agentstate_policy_simple, Genotype, Phenotype, RNG, true, true>()(
                "AmeeMulti", pp, cellmanager);
    }
    else if (std::make_tuple(construction, decay, agenttype) ==
             std::tuple<bool, bool, std::string>{true, true, "complex"})
    {
        auto model =
            Modelfactory<Agentstate_policy_complex, Genotype, Phenotype, RNG, true, true>()(
                "AmeeMulti", pp, cellmanager);
    }
    else if (std::make_tuple(construction, decay, agenttype) ==
             std::tuple<bool, bool, std::string>{true, false, "simple"})
    {
        auto model =
            Modelfactory<Agentstate_policy_simple, Genotype, Phenotype, RNG, true, false>()(
                "AmeeMulti", pp, cellmanager);
    }
    else if (std::make_tuple(construction, decay, agenttype) ==
             std::tuple<bool, bool, std::string>{false, false, "complex"})
    {
        auto model =
            Modelfactory<Agentstate_policy_complex, Genotype, Phenotype, RNG, false, false>()(
                "AmeeMulti", pp, cellmanager);
    }

    return 0;
}
