#ifndef UTOPIA_MODELS_AMEEMULTI_HH
#define UTOPIA_MODELS_AMEEMULTI_HH

#include "adaptionfunctions.hh"
#include "agentstates/agentstate_gauss.hh"
#include "cellstate.hh"
#include "utils/test_utils.hh"
#include "utils/utils.hh"
#include <cassert>
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

    using AP = typename Agentstate::Phenotype;
    using APV = typename AP::value_type;

    using AG = typename Agentstate::Genotype;
    using AGV = typename AG::value_type;

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
    double _removethreshold;

    // global agent parameters
    double _livingcost;
    double _reproductioncost;
    double _offspringresources;
    std::vector<double> _mutationrates;
    double _deathprobability;
    double _modifiercost;
    double _upper_resourcelimit;

    // model parameters

    bool _highresoutput;
    std::array<unsigned, 2> _highres_interval;
    Adaptionfunction _check_adaption;
    static std::map<std::string, Adaptionfunction> adaptionfunctionmap;
    std::uniform_real_distribution<double> _deathdist;
    std::uniform_real_distribution<double> _resdist;
    std::uniform_int_distribution<std::size_t> _movedist;
    std::shared_ptr<DataGroup> _dgroup_agents;
    std::shared_ptr<DataGroup> _dgroup_cells;

    std::vector<AgentAdaptortuple> _agent_adaptors;
    std::vector<CellAdaptortuple> _cell_adaptors;

    std::size_t _idx;

public:
    CellUpdateFunction check_arraylengths = [&](std::shared_ptr<CellType> cell) {
        auto size = cell->state().celltrait.size();
        // EXPECT_EQ(cell->state().resources.size(), size);
        // EXPECT_EQ(cell->state().resourceinfluxes.size(), size);
        assert(cell->state().resources.size() == size);
        assert(cell->state().resourceinfluxes.size() == size);
    };

    // update sub functions
    AgentUpdateFunction update_adaption = [&](std::shared_ptr<AgentType> agent) {
        check_arraylengths(agent->state().habitat);

        agent->state().adaption = _check_adaption(agent);
        // std::cout << "update adaption" << std::endl;
        // std::cout << "  " << agent->state().start << "," << agent->state().end << std::endl;
        // std::cout << "  "
        //           << AP(agent->state().phenotype.begin() + agent->state().start,
        //                 agent->state().phenotype.begin() + agent->state().end)
        //           << "\n  " << agent->state().habitat->state().celltrait << "\n"
        //           << std::endl;
        // std::cout << "  " << agent->state().adaption << std::endl;
    };

    AgentUpdateFunction metabolism = [&](std::shared_ptr<AgentType> agent) {
        check_arraylengths(agent->state().habitat);

        std::size_t i = agent->state().start;
        std::size_t j = 0;
        double resourcecredit = 0.;
        auto& cell = agent->state().habitat;

        for (; i < unsigned(agent->state().end) && i < cell->state().resources.size();
             ++i, ++j)
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
        auto cell = agent->state().habitat;
        auto& trt = agent->state().phenotype;
        auto& ctrt = cell->state().celltrait;
        auto start = agent->state().start;
        auto end = agent->state().end;
        auto intensity = agent->state().intensity;
        if (std::abs(intensity) < 1e-16)
        {
            return;
        }
        if (start < 0 or end < 0 or start >= (int)trt.size() or end <= start)
        {
            return;
        }

        int i = start;
        for (; i < end && i < int(ctrt.size()); ++i)
        {
            if (agent->state().resources < _modifiercost)
            {
                break;
            }
            else
            {
                // when decayed to naught, revive with random influx
                if (std::isnan(ctrt[i]))
                {
                    ctrt[i] = 0.;
                    cell->state().resources[i] = 0.;
                    cell->state().resourceinfluxes[i] = _resdist(*this->_rng);
                    cell->state().modtimes[i] = this->_time;
                }
                // nudge value towards own value at this locus
                ctrt[i] -= intensity * (ctrt[i] - trt[i]);
                cell->state().modtimes[i] = this->_time;
                agent->state().resources -= _modifiercost;
            }
        }

        for (; i < end && i < int(trt.size()); ++i)
        {
            if (agent->state().resources < _modifiercost)
            {
                break;
            }
            else
            {
                ctrt.emplace_back(intensity * trt[i]);
                cell->state().modtimes.emplace_back(this->_time);
                cell->state().resources.emplace_back(0.);
                cell->state().resourceinfluxes.emplace_back(
                    (intensity * trt[i] > 0. ? intensity * trt[i] : 0.) *
                    _resdist(*this->_rng));
                agent->state().resources -= _modifiercost;
            }
        }
    };

    AgentUpdateFunction move = [&](std::shared_ptr<AgentType> agent) {
        check_arraylengths(agent->state().habitat);

        auto old_home = agent->state().habitat;

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

        while (agent->state().resources > (_offspringresources + _reproductioncost))
        {
            _agentmanager.agents().emplace_back(std::make_shared<AgentType>(
                Agentstate(agent->state(), _offspringresources, _mutationrates),
                _idx++, agent->state().habitat->position()));

            _agentmanager.agents().back()->state().adaption =
                _check_adaption(_agentmanager.agents().back());

            agent->state().resources -= (_offspringresources + _reproductioncost);
            agent->state().fitness += 1;
        }
    };

    CellUpdateFunction celltrait_decay = [&](std::shared_ptr<CellType> cell) {
        // check_arraylengths(cell);

        auto& org = cell->state().original;
        auto& ctrt = cell->state().celltrait;
        auto& times = cell->state().modtimes;

        for (std::size_t i = 0; i < org.size(); ++i)
        {
            ctrt[i] = org[i] + (ctrt[i] - org[i]) *
                                   std::exp(-_decayintensity * (ctrt[i] - org[i]));
        }

        for (std::size_t i = org.size(); i < ctrt.size(); ++i)
        {
            ctrt[i] *= std::exp(-_decayintensity * (ctrt[i] - org[i]));
            if (std::abs(ctrt[i]) < _removethreshold)
            {
                ctrt[i] = std::numeric_limits<CTV>::quiet_NaN();
                cell->state().resourceinfluxes[i] = 0.;
                times[i] = std::numeric_limits<double>::quiet_NaN();
                // cellresources are left alone, can still be used, but nothing else anymore is done
            }
        }
    };

    CellUpdateFunction update_cell = [&](std::shared_ptr<CellType> cell) {
        // check_arraylengths(cell);

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

        metabolism(agent);

        if constexpr (construction)
        {
            modify(agent);
        }

        reproduce(agent);

        kill(agent);

        // std::size_t i = agent->state().start;
        // std::size_t j = 0;
        // for (; i < agent->state().end &&
        //        i < agent->state().habitat->state().celltrait.size();
        //      ++i, ++j)
        // {
        //     if (std::isnan(agent->state().adaption[j]) or
        //         std::isnan(agent->state().habitat->state().celltrait[i]) or
        //         std::isnan(agent->state().phenotype[i]))
        //     {
        //         this->_log->warn("NaN found!");
        //         std::cout << " adaption:  " << agent->state().adaption << std::endl;
        //         std::cout << " celltrait: " << agent->state().habitat->state().celltrait
        //                   << std::endl;
        //         std::cout << " trait    : " << agent->state().phenotype << std::endl;
        //         // throw std::runtime_error(" nan found!");
        //     }
        // }
    };

    template <class ParentModel>
    AmeeMulti(const std::string name, ParentModel& parent, CellManager cellmanager, AgentManager agentmanager)
        : Base(name, parent),
          _cellmanager(cellmanager),
          _agentmanager(agentmanager),
          _decayintensity(as_double(this->_cfg["decayintensity"])),
          _removethreshold(as_double(this->_cfg["removethreshold"])),
          _livingcost(as_double(this->_cfg["livingcost"])),
          _reproductioncost(as_double(this->_cfg["reproductioncost"])),
          _offspringresources(as_double(this->_cfg["offspringresources"])),
          _mutationrates(std::vector<double>{
              as_double(this->_cfg["substitutionrate"]), as_double(this->_cfg["insertionrate"]),
              as_double(this->_cfg["substitution_std"])}),
          _deathprobability(as_double(this->_cfg["deathprobability"])),
          _modifiercost(as_double(this->_cfg["modifiercost"])),
          _upper_resourcelimit(as_double(this->_cfg["upper_resourcelimit"])),
          _highresoutput(as_bool(this->_cfg["highresoutput"])),
          _highres_interval(
              as_array<unsigned, 2>(this->_cfg["highresinterval"])),
          _check_adaption(
              adaptionfunctionmap[as_str(this->_cfg["adaptionfunction"])]),
          _deathdist(std::uniform_real_distribution<double>(0., 1.)),
          _resdist(std::uniform_real_distribution<double>(
              as_vector<double>(this->_cfg["resourceinflux_limits"])[0],
              as_vector<double>(this->_cfg["resourceinflux_limits"])[1])),
          _movedist(std::uniform_int_distribution<std::size_t>(0, 7)),
          _dgroup_agents(this->_hdfgrp->open_group("Agents")),
          _dgroup_cells(this->_hdfgrp->open_group("Cells")),
          _agent_adaptors(
              {AgentAdaptortuple{"adaption",
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
               AgentAdaptortuple{
                   "end",
                   [](const auto& agent) -> double { return agent->state().end; }},
               AgentAdaptortuple{"fitness",
                                 [](const auto& agent) -> double {
                                     return agent->state().fitness;
                                 }},
               AgentAdaptortuple{
                   "age",
                   [](const auto& agent) -> double { return agent->state().age; }},
               AgentAdaptortuple{"traitlen",
                                 [](const auto& agent) -> double {
                                     return agent->state().phenotype.size();
                                 }}}),
          _cell_adaptors({CellAdaptortuple{"resources",
                                           [](const auto& cell) -> std::vector<double> {
                                               return cell->state().resources;
                                           }},
                          CellAdaptortuple{"resourceinfluxes",
                                           [](const auto& cell) -> std::vector<double> {
                                               return cell->state().resourceinfluxes;
                                           }}}),
          _idx(0)
    {
        initialize_cells();
        initialize_agents();
        this->_log->info("Model Parameters:");
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
        this->_log->info(" upper_resourcelimit: {}", _upper_resourcelimit);
    }

    void initialize_cells()
    {
        this->_log->debug("Starting initialize_cells");

        // Extract the mode that determines the initial state
        const auto init_celltrait_len =
            as_<std::size_t>(this->_cfg["init_cell_traitlen"]);

        const auto init_cellresourceinflux =
            as_str(this->_cfg["init_cell_resourceinflux"]);
        std::vector<double> init_cellresourceinfluxes(init_celltrait_len, 0.);
        if (init_cellresourceinflux == "random")
        {
            std::generate(init_cellresourceinfluxes.begin(),
                          init_cellresourceinfluxes.end(),
                          [this]() { return _resdist(*this->_rng); });
        }
        else if (init_cellresourceinflux == "given")
        {
            init_cellresourceinfluxes =
                as_vector<double>(this->_cfg["cell_influxvalues"]);
        }
        else
        {
            throw std::runtime_error(
                "Unknown init_cell_resourceinflux given in config");
        }

        std::vector<double> init_cellresources(init_celltrait_len, 1.);
        const auto init_celltrait_values =
            as_array<double, 2>(this->_cfg["init_celltrait_values"]);

        this->_log->info("Cell Parameters:");
        this->_log->info(" init_celltrait_len: {}", init_celltrait_len);
        this->_log->info(" init_cell_resources");
        for (auto& value : init_cellresources)
        {
            this->_log->info("  {}", value);
        }

        this->_log->info(" init_cell_resourceinflux");
        for (auto& value : init_cellresourceinfluxes)
        {
            this->_log->info("  {}", value);
        }

        this->_log->info(" init_celltrait_values");
        for (auto& value : init_celltrait_values)
        {
            this->_log->info("  {}", value);
        }

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
                Cellstate cs(init_celltrait, init_cellresourceinfluxes, init_cellresources);
                return cs;
            },
            _cellmanager.cells());

        // Write information that cells are initialized to the logger
        this->_log->info("Cells initialized.");
    }

    void initialize_agents()
    {
        this->_log->info("Starting initialize_agents");

        // Extract the mode that determines the initial state
        const auto init_genotypelen =
            as_<std::size_t>(this->_cfg["init_genotypelen"]);
        const auto init_resources = as_double(this->_cfg["init_resources"]);
        const auto init_genotype_values =
            as_array<double, 2>(this->_cfg["init_genotype_values"]);

        this->_log->info(" Agent Parameters:");
        this->_log->info(" init_genotypelen: {}", init_genotypelen);
        this->_log->info(" init_resources: {}", init_resources);

        this->_log->info(" init_genotype_values");
        for (auto& value : init_genotype_values)
        {
            this->_log->info("  {}", value);
        }

        // make adaption which is viable
        auto cell = find_cell(_agentmanager.agents()[0], _cellmanager);

        std::size_t i = 0;
        auto agent = _agentmanager.agents()[0];
        std::uniform_real_distribution<AGV> dist(init_genotype_values[0],
                                                 init_genotype_values[1]);

        std::uniform_int_distribution<int> idist(0, init_genotypelen);
        // std::uniform_real_distribution<double> zo_dist(0., 1.);
        for (; i < 10000; ++i)
        {
            // make initial agent genotype
            AG trait(init_genotypelen);
            for (auto& val : trait)
            {
                if constexpr (std::is_integral_v<AGV>)
                {
                    val = std::round(dist(*(this->_rng)));
                }
                else
                {
                    val = dist(*(this->_rng));
                }
            }

            agent->state() = Agentstate(trait, cell, init_resources, this->_rng);

            if constexpr (std::is_same_v<Agentstate, AgentStateGauss<CellType, AP, RNG>>)
            {
                int s = idist(*this->_rng);
                int e = idist(*this->_rng);

                if (s > int(cell->state().celltrait.size()))
                {
                    continue;
                }

                // swap if end smaller than start

                if (e < s)
                {
                    std::swap(s, e);
                }

                agent->state().start = s;
                agent->state().end = e;
                agent->state().intensity = 0.;
            }
            agent->state().adaption = _check_adaption(agent);

            std::size_t i = agent->state().start;
            std::size_t j = 0;
            bool found = false;
            double cum_res = 0.;
            for (; i < unsigned(agent->state().end) and
                   i < cell->state().resources.size() and
                   j < agent->state().adaption.size();
                 ++i, ++j)
            {
                cum_res += agent->state().adaption[j];
                if (cum_res > _livingcost)
                {
                    found = true;
                }
            }
            if (found == true)
            {
                break;
            }
        }

        this->_log->info("Initial agent: ");
        this->_log->info(" adaption");
        for (auto& val : agent->state().adaption)
        {
            this->_log->info(val);
        }

        this->_log->info(" agent start {}", agent->state().start);
        this->_log->info(" agent end {}", agent->state().end);

        this->_log->info("Agents initialized");
    }

    void increment_time(const typename Base::Time dt = 1)
    {
        this->_time += dt;
    }

    void perform_step()
    {
        auto& agents = _agentmanager.agents();
        auto& cells = _cellmanager.cells();
        auto mean = [](auto begin, auto end, auto getter) {
            double m = 0.;
            double s = double(std::distance(begin, end));
            for (; begin != end; ++begin)
            {
                m += getter(*begin);
            }
            return m / s;
        };

        if (this->_time % 250 == 0)
        {
            this->_log->info(
                "\nCurrent time: {},\n current populationsize: {},\n "
                "<adaption> {},\n "
                "<intensity> {},\n "
                "<celltraitlen> {},\n <traitlen> {}, \n <start> {}, \n <end> "
                "{}",
                this->_time, agents.size(),
                mean(agents.begin(), agents.end(),
                     [](auto agent) {
                         return std::accumulate(agent->state().adaption.begin(),
                                                agent->state().adaption.end(), 0.);
                     }),
                mean(agents.begin(), agents.end(),
                     [](auto agent) { return agent->state().intensity; }),
                mean(cells.begin(), cells.end(),
                     [](auto cell) { return cell->state().celltrait.size(); }),
                mean(agents.begin(), agents.end(),
                     [](auto agent) { return agent->state().phenotype.size(); }),
                mean(agents.begin(), agents.end(),
                     [](auto agent) { return agent->state().start; }),
                mean(agents.begin(), agents.end(),
                     [](auto agent) { return agent->state().end; }));
        }
        if (agents.size() == 0)
        {
            this->_log->info("Population extinct");
            return;
        }

        std::for_each(agents.begin(), agents.end(), update_adaption);

        std::for_each(cells.begin(), cells.end(), update_cell);

        std::shuffle(agents.begin(), agents.end(), *this->_rng);
        // std::for_each_n(agents.begin(), agents.size(), update_agent);

        std::vector<unsigned> indices(agents.size());
        std::iota(indices.begin(), indices.end(), 0);

        std::shuffle(indices.begin(), indices.end(), *(this->_rng));

        for (auto& idx : indices)
        {
            update_agent(agents[idx]);
        }

        agents.erase(
            std::remove_if(agents.begin(), agents.end(),
                           [](auto agent) { return agent->state().deathflag; }),
            agents.end());
    }

    void write_data()
    {
        auto& agents = _agentmanager.agents();
        auto& cells = _cellmanager.cells();

        if (agents.size() == 0)
        {
            return;
        }

        std::size_t chunksize = (agents.size() < 1000) ? (agents.size()) : 1000;
        this->_log->debug("Writing data at time {}", this->_time);

        auto agrp = this->_dgroup_agents->open_group(std::to_string(this->_time));
        for (auto& tpl : _agent_adaptors)
        {
            auto& [name, func] = tpl;
            this->_log->debug("name of dataset: {}", name);
            agrp->open_dataset(name, {agents.size()}, {chunksize})
                ->write(agents.begin(), agents.end(), func);
        }

        agrp->open_dataset("adaptionvector", {agents.size()}, {chunksize})
            ->write(agents.begin(), agents.end(),
                    [](const auto& agent) { return agent->state().adaption; });

        auto cgrp = this->_dgroup_cells->open_group(std::to_string(this->_time));
        for (auto& tpl : _cell_adaptors)
        {
            auto& [name, func] = tpl;
            cgrp->open_dataset(name, {cells.size()})->write(cells.begin(), cells.end(), func);
        };

        cgrp->open_dataset("celltraitlen", {cells.size()})
            ->write(cells.begin(), cells.end(), [](const auto& cell) {
                return cell->state().celltrait.size();
            });
    }

    auto& cellmanager()
    {
        return _cellmanager;
    }

    auto& agentmanager()
    {
        return _agentmanager;
    }

    const auto& agents()
    {
        return _agentmanager.agents();
    }

    const auto& cells()
    {
        return _cellmanager.cells();
    }

    Adaptionfunction get_adaptionfunction()
    {
        return _check_adaption;
    }

    void set_adaptionfunction(Adaptionfunction new_adaptionfunc)
    {
        _check_adaption = new_adaptionfunc;
    }

    double get_livingcost()
    {
        return _livingcost;
    }

    void set_livingcost(double lv)
    {
        _livingcost = lv;
    }

    double get_reproductioncost()
    {
        return _reproductioncost;
    }

    void set_reproductioncost(double rc)
    {
        _reproductioncost = rc;
    }

    double get_offspringresources()
    {
        return _offspringresources;
    }

    void set_offspringresources(double oc)
    {
        _offspringresources = oc;
    }

    double get_deathprobability()
    {
        return _deathprobability;
    }

    void set_deathprobability(double dth)
    {
        _deathprobability = dth;
    }

    double get_decayintensity()
    {
        return _decayintensity;
    }

    void set_decayintensity(double dci)
    {
        _decayintensity = dci;
    }

    double get_removethreshold()
    {
        return _removethreshold;
    }

    void set_removethreshold(double rmth)
    {
        _removethreshold = rmth;
    }

    double get_modifiercost()
    {
        return _modifiercost;
    }

    void set_modifiercost(double mc)
    {
        _modifiercost = mc;
    }

    bool get_highresoutput()
    {
        return _highresoutput;
    }

    void set_highresoutput(bool hro)
    {
        _highresoutput = hro;
    }

    std::size_t get_idx()
    {
        return _idx;
    }

    auto get_mutationrates()
    {
        return _mutationrates;
    }

    void set_mutationrates(std::vector<double> mutationrates)
    {
        _mutationrates = mutationrates;
    }

    auto get_upperresourcelimit()
    {
        return _upper_resourcelimit;
    }

    auto get_highresinterval()
    {
        return _highres_interval;
    }

    auto get_decay()
    {
        return decay;
    }

    auto get_construction()
    {
        return construction;
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
