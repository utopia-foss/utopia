#ifndef UTOPIA_MODELS_AMEEMULTI_HH
#define UTOPIA_MODELS_AMEEMULTI_HH

#include "adaptionfunctions.hh"
#include "cellstate.hh"
#include "utils/statistics.hh"
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

    using AgentAdaptor = std::function<double(const std::shared_ptr<AgentType>)>;
    using AgentAdaptortuple = std::tuple<std::string, AgentAdaptor>;

    using CellAdaptor =
        std::function<std::vector<double>(const std::shared_ptr<CellType>)>;
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
    unsigned _highrestime;
    unsigned _statisticstime;
    Adaptionfunction _check_adaption;
    static std::map<std::string, Adaptionfunction> adaptionfunctionmap;
    std::uniform_real_distribution<double> _deathdist;
    std::uniform_real_distribution<double> _resdist;
    std::uniform_int_distribution<std::size_t> _movedist;
    std::shared_ptr<DataGroup> _dgroup_agents;
    std::shared_ptr<DataGroup> _dgroup_cells;
    std::shared_ptr<DataGroup> _dgroup_statistics;
    std::vector<std::shared_ptr<DataSet>> _dsets_statistics;
    std::vector<AgentAdaptortuple> _agent_adaptors;
    std::vector<CellAdaptortuple> _cell_adaptors;
    std::vector<std::vector<std::array<double, 8>>> _statistics_data;
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
        // check_arraylengths(agent->state().habitat);

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
        // check_arraylengths(agent->state().habitat);

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
        this->_log->debug(" agent: {}", agent->id());
        // check_arraylengths(agent->state().habitat);
        auto cell = agent->state().habitat;
        auto& trt = agent->state().phenotype;
        auto& ctrt = cell->state().celltrait;
        int start = agent->state().start_mod;
        int end = agent->state().end_mod;
        double intensity = agent->state().intensity;

        if (std::abs(intensity) < 1e-16)
        {
            return;
        }
        if (start < 0 or end < 0 or start >= (int)trt.size() or end < start)
        {
            return;
        }

        int min_m = std::min({end, int(ctrt.size()), int(trt.size())});
        int min_a = std::min(end, int(trt.size()));

        for (int i = start; i < min_m; ++i)
        {
            this->_log->debug("  modifying: i = {} , end = {}, ctrtsize = {}",
                              i, end, ctrt.size());

            if (agent->state().resources < (_reproductioncost + _offspringresources))
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

                // replace value at locus with scaled value of organism phenotype
                double value = intensity * trt[i];
                double cost = _modifiercost * std::abs(value - ctrt[i]);
                if (cost < agent->state().resources)
                {
                    ctrt[i] = value;
                    cell->state().modtimes[i] = this->_time;
                    agent->state().resources -= cost;
                }
            }
        }

        for (int i = min_m; i < min_a; ++i)
        {
            this->_log->debug("  appending: i = {} , end = {}", i, end);
            if (agent->state().resources < (_reproductioncost + _offspringresources))
            {
                break;
            }
            else
            {
                double value = intensity * trt[i];
                double cost = _modifiercost * std::abs(value);

                if (cost < agent->state().resources)
                {
                    ctrt.emplace_back(intensity * trt[i]);
                    cell->state().modtimes.emplace_back(this->_time);
                    cell->state().resources.emplace_back(0.);
                    // cell->state().resourceinfluxes.emplace_back(
                    //     (intensity * trt[i] > 0. ? intensity * trt[i] : 0.) *
                    //     _resdist(*this->_rng));
                    cell->state().resourceinfluxes.emplace_back(_resdist(*this->_rng));
                    agent->state().resources -= cost;
                }
            }
        }
    };

    AgentUpdateFunction move = [&](std::shared_ptr<AgentType> agent) {
        // check_arraylengths(agent->state().habitat);

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
        // check_arraylengths(agent->state().habitat);

        if (is_equal(agent->state().resources, 0.) or _deathdist(*(this->_rng)) < _deathprobability)
        {
            agent->state().deathflag = true;
        }
    };

    AgentUpdateFunction reproduce = [&](std::shared_ptr<AgentType> agent) {
        // check_arraylengths(agent->state().habitat);
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
        auto t = this->_time;
        for (std::size_t i = 0; i < org.size(); ++i)
        {
            ctrt[i] = org[i] + (ctrt[i] - org[i]) *
                                   std::exp(-_decayintensity * (t - times[i]));
        }

        for (std::size_t i = org.size(); i < ctrt.size(); ++i)
        {
            if (std::isnan(ctrt[i]))
            {
                continue;
            }
            ctrt[i] *= std::exp(-_decayintensity * (t - times[i]));
            if (std::abs(ctrt[i]) < _removethreshold)
            {
                ctrt[i] = std::numeric_limits<CTV>::quiet_NaN();
                cell->state().resourceinfluxes[i] = 0.;
                times[i] = std::numeric_limits<double>::quiet_NaN();

                // cellresources are left alone, can still be used, but nothing else anymore is done
            }
        }

        // // remove shit again
        // ctrt.erase(std::remove_if(ctrt.begin(), ctrt.end(),
        //                           [](auto value) { return std::isnan(value); }),
        //            ctrt.end());
        // cell->state().resourceinfluxes.erase(
        //     std::remove_if(cell->state().resourceinfluxes.begin(),
        //                    cell->state().resourceinfluxes.end(),
        //                    [](auto value) { return std::abs(value) < 1e-16; }),
        //     ctrt.end());
        // times.erase(std::remove_if(times.begin(), times.end(),
        //                            [](auto value) { return std::isnan(value); }),
        //             ctrt.end());
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
        update_adaption(agent);
        // this->_log->info("  begin adaption: {} ",
        //                  std::accumulate(agent->state().adaption.begin(),
        //                                  agent->state().adaption.end(), 0.));
        // this->_log->info("  begin resources: {}", agent->state().resources);
        move(agent);
        // this->_log->info("  move adaption: {} ",
        //                  std::accumulate(agent->state().adaption.begin(),
        //                                  agent->state().adaption.end(), 0.));
        // this->_log->info("  move resources: {}", agent->state().resources);
        if constexpr (construction)
        {
            modify(agent);
            // this->_log->info("  construction adaption: {} ",
            //                  std::accumulate(agent->state().adaption.begin(),
            //                                  agent->state().adaption.end(), 0.));
            // this->_log->info("  construction resources: {}", agent->state().resources);
        }
        update_adaption(agent);
        // this->_log->info("  update adaption adaption: {} ",
        //                  std::accumulate(agent->state().adaption.begin(),
        //                                  agent->state().adaption.end(), 0.));
        // this->_log->info("  update adaption resources: {}", agent->state().resources);
        metabolism(agent);
        // this->_log->info("  metabolism adaption: {} ",
        //                  std::accumulate(agent->state().adaption.begin(),
        //                                  agent->state().adaption.end(), 0.));
        // this->_log->info("  metabolism resources: {}", agent->state().resources);
        reproduce(agent);
        // this->_log->info("  reproduce adaption: {} ",
        //                  std::accumulate(agent->state().adaption.begin(),
        //                                  agent->state().adaption.end(), 0.));
        // this->_log->info("  reproduce resources: {}", agent->state().resources);
        kill(agent);
        // this->_log->info("  kill adaption: {}",
        //                  std::accumulate(agent->state().adaption.begin(),
        //                                  agent->state().adaption.end(), 0.));
        // this->_log->info("  kill resources: {}", agent->state().resources);
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
          _highrestime(as_<unsigned>(this->_cfg["highrestime"])),
          _statisticstime(as_<unsigned>(this->_cfg["statisticstime"])),
          _check_adaption(
              adaptionfunctionmap[as_str(this->_cfg["adaptionfunction"])]),
          _deathdist(std::uniform_real_distribution<double>(0., 1.)),
          _resdist(std::uniform_real_distribution<double>(
              as_vector<double>(this->_cfg["resourceinflux_limits"])[0],
              as_vector<double>(this->_cfg["resourceinflux_limits"])[1])),
          _movedist(std::uniform_int_distribution<std::size_t>(0, 7)),
          _dgroup_agents(this->_hdfgrp->open_group("Agents")),
          _dgroup_cells(this->_hdfgrp->open_group("Cells")),
          _dgroup_statistics(this->_hdfgrp->open_group("Agent_statistics")),
          _agent_adaptors(
              {AgentAdaptortuple{"accumulated_adaption",
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

        _dgroup_statistics->add_attribute(
            "Stored quantities", "mean, var, mode, min, q25, q50, q75, max");
        _dgroup_statistics->add_attribute("Save time", 50);

        for (std::size_t i = 0; i < _agent_adaptors.size(); ++i)
        {
            _dsets_statistics.push_back(_dgroup_statistics->open_dataset(
                std::get<0>(_agent_adaptors[i]), {1 + (this->_time_max / _statisticstime)}));

            _statistics_data.push_back(std::vector<std::array<double, 8>>());
            _statistics_data.back().reserve(1 + this->_time_max / _statisticstime);
        }

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

        const auto init_cell_resourceinflux_kind =
            as_str(this->_cfg["init_cellresourceinflux_kind"]);

        std::vector<double> init_cellresourceinfluxes;
        if (init_cell_resourceinflux_kind == "random")
        {
            init_cellresourceinfluxes.resize(init_celltrait_len);
            std::generate(init_cellresourceinfluxes.begin(),
                          init_cellresourceinfluxes.end(),
                          [this]() { return _resdist(*this->_rng); });
        }
        else if (init_cell_resourceinflux_kind == "given")
        {
            init_cellresourceinfluxes =
                as_vector<double>(this->_cfg["init_cell_influxvalues"]);
            if (init_celltrait_len != init_cellresourceinfluxes.size())
            {
                throw std::runtime_error(
                    "init_cell_influxvalues must be as long as "
                    "init_celltraitlen");
            }
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
                Cellstate cs(init_celltrait, init_cellresources, init_cellresourceinfluxes);
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

        auto agent = _agentmanager.agents()[0];
        std::uniform_real_distribution<double> dist(init_genotype_values[0],
                                                    init_genotype_values[1]);

        bool found = false;

        for (std::size_t i = 0; i < 100000; ++i)
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

            agent->state().adaption = _check_adaption(agent);

            double cum_res = 0;
            int s = agent->state().start;
            int e = agent->state().end;
            int amin =
                std::min({e, int(agent->state().habitat->state().celltrait.size()),
                          int(agent->state().phenotype.size())});
            for (int i = s; i < amin; ++i)
            {
                cum_res += (agent->state().habitat->state().resourceinfluxes[i] >
                            agent->state().adaption[i - s])
                               ? agent->state().adaption[i - s]
                               : agent->state().habitat->state().resourceinfluxes[i];
            }
            // std::cout << " candidate build: " << std::endl;
            // std::cout << "  start    : " << agent->state().start <<
            // std::endl; std::cout << "  end      : " << agent->state().end <<
            // std::endl; std::cout << "  cumad    : " << cum_res << std::endl;

            // std::cout << "  intensity: " << agent->state().intensity << std::endl;

            if (cum_res > 1.2 * _livingcost)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            throw std::runtime_error("Could not build viable organism!");
        }

        // reserve memory for a lot of agents:
        _agentmanager.agents().reserve(1000000);

        this->_log->info("Initial agent: ");

        this->_log->info(" agent start {}", agent->state().start);
        this->_log->info(" agent end {}", agent->state().end);
        this->_log->info(" agent start_mod {}", agent->state().start_mod);
        this->_log->info(" agent end_mod {}", agent->state().end_mod);
        this->_log->info(" adaption size: {}", agent->state().adaption.size());
        // this->_log->info(" adaption");
        // for (auto& val : agent->state().adaption)
        // {
        //     this->_log->info("  {}", val);
        // }

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

        if (this->_time % 5 == 0)
        {
            auto mean = [](auto begin, auto end, auto getter) {
                double m = 0.;
                double s = double(std::distance(begin, end));
                for (; begin != end; ++begin)
                {
                    m += getter(*begin);
                }
                return m / s;
            };
            this->_log->info(
                "Current time: {}\n current populationsize: {}\n"
                " <adaption> {}\n <adaption_size> {}\n <resourceinfluxes> {}\n"
                " <resourceinfluxesize> {}",
                this->_time, agents.size(),
                mean(agents.begin(), agents.end(),
                     [](auto agent) {
                         return std::accumulate(agent->state().adaption.begin(),
                                                agent->state().adaption.end(), 0.);
                     }),
                mean(agents.begin(), agents.end(),
                     [](auto agent) { return agent->state().adaption.size(); }),
                mean(cells.begin(), cells.end(),
                     [](auto cell) {
                         return std::accumulate(
                             cell->state().resourceinfluxes.begin(),
                             cell->state().resourceinfluxes.end(), 0.);
                     }),
                mean(cells.begin(), cells.end(), [](auto cell) {
                    return cell->state().resourceinfluxes.size();
                }));
        }
        if (agents.size() == 0)
        {
            this->_log->info("Population extinct");
            // throw std::runtime_error(" Population extinct");
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
        if ((this->_time < _highres_interval[1] and this->_time >= _highres_interval[0]))
        {
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
                cgrp->open_dataset(name, {cells.size()})
                    ->write(cells.begin(), cells.end(), func);
            };

            cgrp->open_dataset("celltraitlen", {cells.size()})
                ->write(cells.begin(), cells.end(), [](const auto& cell) {
                    return cell->state().celltrait.size();
                });
        }

        if (this->_time % _statisticstime == 0)
        {
            for (std::size_t i = 0; i < _agent_adaptors.size(); ++i)
            {
                _statistics_data[i].push_back(Utils::Describe()(
                    agents.begin(), agents.end(), std::get<1>(_agent_adaptors[i])));
            }
        }

        if (_statistics_data.front().size() == _statistics_data.front().capacity())
        {
            for (std::size_t i = 0; i < _agent_adaptors.size(); ++i)
            {
                _dsets_statistics[i]->write(_statistics_data[i]);
                _statistics_data[i].clear();
            }
        }
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

    auto get_time()
    {
        return this->_time;
    }

    auto set_time(std::size_t t)
    {
        this->_time = t;
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
