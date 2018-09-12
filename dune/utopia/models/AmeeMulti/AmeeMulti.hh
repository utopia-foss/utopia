#ifndef UTOPIA_MODELS_AMEEMULTI_HH
#define UTOPIA_MODELS_AMEEMULTI_HH

#include "adaptionfunctions.hh"
#include "agentstate.hh"
#include "cellstate.hh"
#include "utils/test_utils.hh"
#include "utils/utils.hh"
#include <dune/utopia/base.hh>
#include <dune/utopia/core/apply.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/types.hh>

#include <functional>

namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
using namespace Utils;

/**
 * @brief blah balh
 *
 * @tparam Cell
 * @tparam CellManager
 * @tparam AgentManager
 * @tparam Modeltypes
 * @tparam construction
 * @tparam decay
 */
template <typename Cell, typename CellManager, typename AgentManager, typename Modeltypes, bool construction, bool decay>
class AmeeMulti
    : public Model<AmeeMulti<Cell, CellManager, AgentManager, Modeltypes, construction, decay>, Modeltypes>
{
public:
    using AgentType = typename AgentManager::Agent;
    using CellType = typename CellManager::Cell;
    /// CellState
    using Cellstate = typename CellType::State;

    /// Agentstate
    using Agentstate = typename AgentType::State;

    // cell traits typedefs
    using CT = typename Cellstate::Trait;
    using CTV = typename CT::value_type;

    using AP = typename Agentstate::Trait;
    using APV = typename AP::value_type;

    using Types = Modeltypes;

    /// The base model type
    using Base =
        Model<AmeeMulti<Cell, CellManager, AgentManager, Modeltypes, construction, decay>, Modeltypes>;

    /// Data type that holds the configuration
    using Config = typename Base::Config;

    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename Base::DataGroup;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    /// Data type of the shared RNG
    using RNG = typename Base::RNG;

    // Alias the neighborhood classes to make access more convenient
    using NextNeighbor = Utopia::Neighborhoods::NextNeighbor;
    using MooreNeighbor = Utopia::Neighborhoods::MooreNeighbor;

private:
    // function types
    using AgentFunction = std::function<void(const std::shared_ptr<AgentType>)>;

    using CellFunction = std::function<void(const std::shared_ptr<CellType>)>;

    using Adaptionfunction =
        std::function<std::vector<double>(const std::shared_ptr<AgentType>)>;

    using AgentAdaptor = std::function<double(const std::shared_ptr<AgentType>&)>;
    using AgentAdaptortuple = std::tuple<std::string, AgentAdaptor>;
    using CellAdaptor =
        std::function<std::vector<double>(const std::shared_ptr<CellType>&)>;
    using CellAdaptortuple = std::tuple<std::string, CellAdaptor>;

    using AgentUpdateFunction = std::function<void(const std::shared_ptr<AgentType>)>;
    using CellUpdateFunction = std::function<void(const std::shared_ptr<CellType>)>;
    // managers
    CellManager _cellmanager;
    AgentManager _agentmanager;

    // global cell parameters
    double _decayintensity;

    // global agent parameters
    double _livingcost;
    double _reproductioncost;
    double _offspringresources;
    std::vector<double> _mutationrates;
    double _deathprobability;
    double _modifiercost;
    double _upper_resourcelimit;

    // model parameters
    std::array<unsigned, 2> _highres_interval;
    Adaptionfunction _check_adaption;
    static std::map<std::string, Adaptionfunction> adaptionfunctionmap;
    std::uniform_real_distribution<double> _deathdist;
    std::uniform_int_distribution<std::size_t> _movedist;
    std::shared_ptr<DataGroup> _dgroup_agents;
    std::shared_ptr<DataGroup> _dgroup_cells;

    std::vector<AgentAdaptortuple> _agent_adaptors;
    std::vector<CellAdaptortuple> _cell_adaptors;
    bool _highresoutput;

public:
    CellUpdateFunction check_arraylengths = [&](std::shared_ptr<CellType> cell) {
        auto size = cell->state().celltrait.size();
        EXPECT_EQ(cell->state().resources.size(), size);
        EXPECT_EQ(cell->state().resourceinfluxes.size(), size);
    };

    // update sub functions
    AgentUpdateFunction update_adaption = [&](std::shared_ptr<AgentType> agent) {
        check_arraylengths(agent->state().habitat);
        agent->state().adaption = _check_adaption(agent);
    };

    AgentUpdateFunction metabolism = [&](std::shared_ptr<AgentType> agent) {
        check_arraylengths(agent->state().habitat);
        std::size_t i = agent->state().start;
        std::size_t j = 0;
        double resourcecredit = 0.;
        auto& cell = agent->state().habitat;

        for (; i < agent->state().end && i < cell->state().resources.size(); ++i, ++j)
        {
            resourcecredit = (cell->state().resources[i] > agent->state().adaption[j])
                                 ? agent->state().adaption[j]
                                 : cell->state().resources[i];

            resourcecredit = (resourcecredit > _upper_resourcelimit) ? _upper_resourcelimit
                                                                     : resourcecredit;
            agent->state().resources += resourcecredit;
            cell->state().resources[i] -= resourcecredit;
        }

        agent->state().resources = (agent->state().resources > _livingcost)
                                       ? (agent->state().resources - _livingcost)
                                       : 0.;
        agent->state().age += 1;
    };

    AgentUpdateFunction modify = [&](std::shared_ptr<AgentType> agent) {
        check_arraylengths(agent->state().habitat);
    };

    AgentUpdateFunction move = [&](std::shared_ptr<AgentType> agent) {
        check_arraylengths(agent->state().habitat);

        auto old_home = agent->state().habitat;
        auto& celltrait = old_home->state().celltrait;
        auto& trait = agent->state().trait;
        decltype(old_home) new_home = nullptr;
        if (agent->state().resources < (_offspringresources + _reproductioncost))
        {
            auto nb = MooreNeighbor::neighbors(old_home, _cellmanager);
            std::shuffle(nb.begin(), nb.end(), std::forward<RNG>(*(this->_rng)));
            double testadaption = std::accumulate(
                agent->state().adaption.begin(), agent->state().adaption.end(), 0.);
            double trialadaption = testadaption;
            double curradaption = testadaption;

            // directed search for better habitat
            for (auto& n : nb)
            {
                agent->state().habitat = n;
                update_adaption(agent);
                testadaption = std::accumulate(agent->state().adaption.begin(),
                                               agent->state().adaption.end(), 0.);
                if (testadaption > trialadaption && testadaption > curradaption)
                {
                    trialadaption = testadaption;
                    new_home = n;
                }
            }

            // random movement if nothing better found
            if (new_home == nullptr)
            {
                new_home = nb[_movedist(*(this->_rng))];
            }

            // update adaption and habitat pointer
            agent->state().habitat = new_home;
            update_adaption(agent);

            // get position of agent and move it
            auto pos = new_home->position();
            move_to(pos, agent, _agentmanager);
        }
    };

    AgentUpdateFunction kill = [&](std::shared_ptr<AgentType> agent) {
        check_arraylengths(agent->state().habitat);

        if (is_equal(agent->state().resources, 0.) or _deathdist(*(this->_rng)) < _deathprobability)
        {
            agent->state().deathflag = true;
        }
    };

    AgentUpdateFunction reproduce = [&](std::shared_ptr<AgentType> agent) {
        check_arraylengths(agent->state().habitat);

        auto cell = agent->state().habitat;
        auto& parent_state = agent->state();
        while (parent_state.resources > (_offspringresources + _reproductioncost))
        {
            _agentmanager.agents().emplace_back(std::make_shared<AgentType>(
                Agentstate(parent_state, _offspringresources, _mutationrates),
                0, parent_state.habitat->position()));

            _agentmanager.agents().back()->state().adaption =
                _check_adaption(_agentmanager.agents().back());

            parent_state.resources -= (_offspringresources + _reproductioncost);
            parent_state.fitness += 1;
        }
    };

    CellUpdateFunction celltrait_decay = [&](std::shared_ptr<CellType> cell) {
        check_arraylengths(cell);
    };

    CellUpdateFunction update_cell = [&](std::shared_ptr<CellType> cell) {
        check_arraylengths(cell);

        for (std::size_t i = 0; i < cell->state().celltrait.size(); ++i)
        {
            cell->state().resources[i] += cell->state().resourceinfluxes[i];
        }

        if constexpr (decay)
        {
            celltrait_decay(cell);
        }
    };

    AgentUpdateFunction update_agent = [&](std::shared_ptr<AgentType> agent) {
        move(agent);

        if constexpr (construction)
        {
            modify(agent);
        }

        metabolism(agent);

        reproduce(agent);

        kill(agent);
    };

    template <class ParentModel>
    AmeeMulti(const std::string name, ParentModel& parent, CellManager cellmanager, AgentManager agentmanager)
        : Base(name, parent),
          _cellmanager(cellmanager),
          _agentmanager(agentmanager),
          _livingcost(as_double(this->_cfg["livingcost"])),
          _reproductioncost(as_double(this->_cfg["reproductioncost"])),
          _offspringresources(as_double(this->_cfg["offspringresources"])),
          _mutationrates(std::vector<double>{
              as_double(this->_cfg["substitutionrate"]), as_double(this->_cfg["insertionrate"]),
              as_double(this->_cfg["substitution_std"])}),
          _deathprobability(as_double(this->_cfg["deathprobability"])),
          _decayintensity(as_double(this->_cfg["decayintensity"])),
          _modifiercost(as_double(this->_cfg["modifiercost"])),
          _upper_resourcelimit(as_double(this->_cfg["upper_resourcelimit"])),
          _highresoutput(as_bool(this->_cfg["highresoutput"])),
          _highres_interval(
              as_array<unsigned, 2>(this->_cfg["highresinterval"])),
          _check_adaption(
              adaptionfunctionmap[as_str(this->_cfg["adaptionfunction"])]),
          _deathdist(std::uniform_real_distribution<double>(0., 1.)),
          _movedist(std::uniform_int_distribution<std::size_t>(0, 8)),
          _agent_adaptors({AgentAdaptortuple{"adaption",
                                             [](const auto& agent) -> double {
                                                 return std::accumulate(
                                                     agent->state().adaption.begin(),
                                                     agent->state().adaption.end(), 0.);
                                             }},
                           AgentAdaptortuple{"intensity",
                                             [](const auto& agent) -> double {
                                                 return agent->state().intensity;
                                             }},
                           AgentAdaptortuple{"start",
                                             [](const auto& agent) -> double {
                                                 return agent->state().start;
                                             }},
                           AgentAdaptortuple{"end",
                                             [](const auto& agent) -> double {
                                                 return agent->state().end;
                                             }},
                           AgentAdaptortuple{"fitness",
                                             [](const auto& agent) -> double {
                                                 return agent->state().fitness;
                                             }},
                           AgentAdaptortuple{"age",
                                             [](const auto& agent) -> double {
                                                 return agent->state().age;
                                             }}}),
          _cell_adaptors(
              {CellAdaptortuple{"resources",
                                [](const auto& cell) -> std::vector<double> {
                                    return cell->state().resources;
                                }},
               CellAdaptortuple{"resourceinfluxes", [](const auto& cell) -> std::vector<double> {
                                    return cell->state().resourceinfluxes;
                                }}})
    {
        initialize_cells();
        initialize_agents();
        this->_log->info("Parameters:");
        this->_log->info(" num cells: {}", _cellmanager.cells().size());
        this->_log->info(" livingcost: {}", _livingcost);
        this->_log->info(" reproductioncost: {}", _reproductioncost);
        this->_log->info(" offspringresources: {}", _offspringresources);
        this->_log->info(" _deathprobability: {}", _deathprobability);
        this->_log->info(" mutationrates: {},{},{},{},{} ", "(", _mutationrates[0],
                         _mutationrates[1], _mutationrates[2], ")");
        this->_log->info(" decayintensity: {}", _decayintensity);
        this->_log->info(" modifiercost: {}", _modifiercost);
        this->_log->info(" highresoutput: {}", _highresoutput);
        this->_log->info(" Agenttype: {}", as_str(this->_cfg["Agenttype"]));
        this->_log->info(" upper_resourcelimit: {}", _upper_resourcelimit);

        write_data();
    }

    void initialize_cells()
    {
        this->_log->debug("Starting initialize_cells");

        // Extract the mode that determines the initial state
        const auto init_celltrait_len =
            as_<std::size_t>(this->_cfg["init_cell_traitlen"]);
        const auto init_cellresources =
            as_vector<double>(this->_cfg["init_cell_resources"]);
        const auto init_cellresourceinflux =
            as_vector<double>(this->_cfg["init_cell_resourceinflux"]);

        const auto init_celltrait_values =
            as_array<double, 2>(this->_cfg["init_celltrait_values"]);

        CT init_celltrait(init_celltrait_len);
        std::uniform_real_distribution<CTV> dist(init_celltrait_values[0],
                                                 init_celltrait_values[1]);
        for (auto& val : init_celltrait)
        {
            val = dist(*this->_rng);
        }

        // generate new cellstate
        apply_rule<false>(
            [&]([[maybe_unused]] const auto cell) {
                Cellstate cs(init_celltrait, init_cellresourceinflux, init_cellresources);
                return cs;
            },
            _cellmanager.cells());

        // Write information that cells are initialized to the logger
        this->_log->debug("Cells initialized.");
    }

    void initialize_agents()
    {
        this->_log->debug("Starting initialize_agents");

        // Extract the mode that determines the initial state
        const auto init_genotypelen =
            as_<std::size_t>(this->_cfg["init_genotypelen"]);
        const auto init_resources = as_double(this->_cfg["init_resources"]);
        const auto init_genotype_values =
            as_array<double, 2>(this->_cfg["init_genotype_values"]);

        const auto init_start = as_<int>(this->_cfg["init_start"]);
        const auto init_end = as_<int>(this->_cfg["init_end"]);
        const auto init_intensity = as_double(this->_cfg["init_intensity"]);

        // make adaption which is viable
        auto cell = find_cell(_agentmanager.agents()[0], _cellmanager);

        std::size_t i = 0;
        auto agent = _agentmanager.agents()[0];
        std::uniform_real_distribution<APV> dist(init_genotype_values[0],
                                                 init_genotype_values[1]);

        std::uniform_int_distribution<int> idist(0, init_genotypelen);
        for (; i < 10000; ++i)
        {
            // make initial agent genotype
            AP trait(init_genotypelen);
            for (auto& val : trait)
            {
                val = dist(*(this->_rng));
            }

            agent->state() = Agentstate(trait, cell, init_resources, this->_rng,
                                        init_start, init_end, init_intensity);

            agent->state().adaption = _check_adaption(agent);

            double acc_adaption = std::accumulate(
                agent->state().adaption.begin(), agent->state().adaption.end(), 0.);

            if (acc_adaption > _livingcost)
            {
                break;
            }
        }
        this->_log->debug("Agents initialized");
    }

    void increment_time(const typename Base::Time dt = 1)
    {
        this->_time += dt;
    }

    void perform_step()
    {
        auto& agents = _agentmanager.agents();
        auto& cells = _cellmanager.cells();

        if (agents.size() == 0)
        {
            this->_log->info("Population extinct");
            return;
        }

        std::for_each(agents.begin(), agents.end(), update_adaption);

        std::for_each(cells.begin(), cells.end(), update_cell);

        std::for_each_n(agents.begin(), agents.size(), update_agent);

        // kill agents
        agents.erase(
            std::remove_if(agents.begin(), agents.end(),
                           [](auto agent) { return agent->state().deathflag; }),
            agents.end());
    }

    void write_data()
    {
        auto& agents = _agentmanager.agents();
        auto& cells = _cellmanager.cells();

        std::size_t chunksize = (agents.size() < 1000) ? (agents.size()) : 1000;
        auto agrp = this->_dgroup_agents->open_group(std::to_string(this->_time));
        for (auto& tpl : _agent_adaptors)
        {
            auto& [name, func] = tpl;
            agrp->open_dataset(name, {agents.size()}, {chunksize})
                ->write(agents.begin(), agents.end(), func);
        }

        auto cgrp = this->_dgroup_cells->open_group(std::to_string(this->_time));
        for (auto& tpl : _cell_adaptors)
        {
            auto& [name, func] = tpl;
            cgrp->open_dataset(name)->write(cells.begin(), cells.end(), func);
        };
    }

}; // namespace AmeeMulti

template <typename Cell, typename CellManager, typename AgentManager, typename Modeltypes, bool construction, bool decay>
std::map<std::string, typename AmeeMulti<Cell, CellManager, AgentManager, Modeltypes, construction, decay>::Adaptionfunction>
    AmeeMulti<Cell, CellManager, AgentManager, Modeltypes, construction, decay>::adaptionfunctionmap = {
        {"multi_notnormed", multi_notnormed},
        {"multi_normed", multi_normed},
        {"simple_notnormed", simple_notnormed},
        {"simple_normed", simple_normed}};

} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_COPYME_HH
