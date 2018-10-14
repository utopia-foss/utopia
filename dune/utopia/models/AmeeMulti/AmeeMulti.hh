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
#include <fstream>

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

    using CellAdaptor = std::function<double(const std::shared_ptr<CellType>)>;
    using CellAdaptortuple = std::tuple<std::string, CellAdaptor>;

    using AgentUpdateFunction = std::function<void(const std::shared_ptr<AgentType>)>;
    using CellUpdateFunction = std::function<void(const std::shared_ptr<CellType>)>;

    // managers
    CellManager _cellmanager;
    AgentManager _agentmanager;
    std::unordered_map<std::shared_ptr<CellType>, MooreNeighbor::neighbors> _neighborhoods;

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

    std::vector<std::array<unsigned, 2>> _highres_interval;
    unsigned _statisticstime;
    Adaptionfunction _check_adaption;
    static std::map<std::string, Adaptionfunction> adaptionfunctionmap;
    std::uniform_real_distribution<double> _deathdist;
    std::uniform_real_distribution<double> _resdist;
    std::uniform_int_distribution<std::size_t> _movedist;
    std::shared_ptr<DataGroup> _dgroup_agents;
    std::shared_ptr<DataGroup> _dgroup_cells;
    std::shared_ptr<DataGroup> _dgroup_agent_statistics;
    std::shared_ptr<DataGroup> _dgroup_cell_statistics;
    std::vector<std::shared_ptr<DataSet>> _dsets_agent_statistics;
    std::vector<std::shared_ptr<DataSet>> _dsets_cell_statistics;

    std::vector<AgentAdaptortuple> _agent_adaptors;
    std::vector<CellAdaptortuple> _cell_adaptors;
    std::vector<std::vector<std::array<double, 7>>> _agent_statistics_data;
    std::vector<std::vector<std::array<double, 7>>> _cell_statistics_data;

    std::size_t _idx;

public:
    // update sub functions

    double logistic_function(double r, double K, double u0, double t)
    {
        return (K * u0 * std::exp(r * t)) / (K + u0 * (std::exp(r * t) - 1.));
    }

    void update_adaption(const std::shared_ptr<AgentType> agent)
    {
        agent->state().adaption = _check_adaption(agent);
    }

    void metabolism(const std::shared_ptr<AgentType> agent)
    {
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

        if (agent->state().resources < 0.)
        {
            throw std::runtime_error("negative resources found for agent!");
        }

        for (auto& val : agent->state().habitat->state().resources)
        {
            if (val < 0.)
            {
                throw std::runtime_error(
                    "negative resources found in agent's habitat!");
            }
        }
    }

    void modify(const std::shared_ptr<AgentType> agent)
    {
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
            // //this->_log->debug("  modifying: i = {} , end = {}, ctrtsize = {}",
            //                   i, end, ctrt.size());

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
            // //this->_log->debug("  appending: i = {} , end = {}", i, end);
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
    }

    void move(const std::shared_ptr<AgentType> agent)
    {
        auto old_home = agent->state().habitat;

        std::shared_ptr<Cell> new_home = nullptr;

        if (agent->state().resources < (_offspringresources + _reproductioncost))
        {
            auto nb = neighborhoods[old_home];
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

            // update habitat pointer
            agent->state().habitat = new_home;

            // get position of agent and move it
            auto pos = new_home->position();
            move_to(pos, agent, _agentmanager);
        }
    }

    void kill(const std::shared_ptr<AgentType> agent)
    {
        if (is_equal(agent->state().resources, 0.) or _deathdist(*(this->_rng)) < _deathprobability)
        {
            agent->state().deathflag = true;
        }
    }

    void reproduce(const std::shared_ptr<AgentType> agent)
    {
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
    }

    void celltrait_decay(const std::shared_ptr<CellType> cell)
    {
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
    }

    void update_cell(const std::shared_ptr<CellType> cell)
    {
        for (std::size_t i = 0; i < cell->state().celltrait.size(); ++i)
        {
            if (is_equal(cell->state().resources[i], 0., 1e-10)) // FIXME: tolerance?
            {
                cell->state().resources[i] = cell->state().resourceinfluxes[i];
            }
            else
            {
                cell->state().resources[i] =
                    logistic_function(cell->state().resourceinfluxes[i],
                                      cell->state().resource_capacities[i],
                                      cell->state().resources[i], 1.);
            }
        }

        if constexpr (decay)
        {
            celltrait_decay(cell);
        }
    }

    void update_agent(const std::shared_ptr<AgentType> agent)
    {
        update_adaption(agent);

        move(agent);

        update_adaption(agent);

        if constexpr (construction)
        {
            modify(agent);
        }

        update_adaption(agent);

        metabolism(agent);

        reproduce(agent);

        kill(agent);
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
          _highres_interval(as_vector<std::array<unsigned, 2>>(
              this->_cfg["highresinterval"])),
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
          _dgroup_agent_statistics(
              this->_hdfgrp->open_group("Agent_statistics")),
          _dgroup_cell_statistics(this->_hdfgrp->open_group("Cell_statistics")),
          _agent_adaptors(
              {AgentAdaptortuple{"accumulated_adaption",
                                 [](const auto& agent) -> double {
                                     return std::accumulate(
                                         agent->state().adaption.begin(),
                                         agent->state().adaption.end(), 0.);
                                 }},
               AgentAdaptortuple{"sumlen",
                                 [](const auto& agent) -> double {
                                     return agent->state().sumlen;
                                 }},
               AgentAdaptortuple{"divisor",
                                 [](const auto& agent) -> double {
                                     return agent->state().divisor;
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
               AgentAdaptortuple{"cell_id",
                                 [](const auto& agent) -> double {
                                     return agent->state().habitat->id();
                                 }},
               AgentAdaptortuple{
                   "age",
                   [](const auto& agent) -> double { return agent->state().age; }},

               AgentAdaptortuple{"traitlen",
                                 [](const auto& agent) -> double {
                                     return agent->state().phenotype.size();
                                 }}}),
          _cell_adaptors(
              {CellAdaptortuple{"resources",
                                [](const auto& cell) -> double {
                                    return std::accumulate(
                                        cell->state().resources.begin(),
                                        cell->state().resources.end(), 0.);
                                }},
               CellAdaptortuple{"resourceinfluxes",
                                [](const auto& cell) -> double {
                                    return std::accumulate(
                                        cell->state().resourceinfluxes.begin(),
                                        cell->state().resourceinfluxes.end(), 0.);
                                }},
               CellAdaptortuple{"cell_traitlen",
                                [](const auto& cell) -> double {
                                    return cell->state().celltrait.size();
                                }}}),

          _idx(0)
    {
        initialize_cells();
        initialize_agents();

        _dgroup_agent_statistics->add_attribute(
            "Stored quantities", "mean, var, mode, min, q25, q50, q75, max");
        _dgroup_agent_statistics->add_attribute("Save time", _statisticstime);

        for (std::size_t i = 0; i < _agent_adaptors.size(); ++i)
        {
            _dsets_agent_statistics.push_back(_dgroup_agent_statistics->open_dataset(
                std::get<0>(_agent_adaptors[i])));

            _agent_statistics_data.push_back(std::vector<std::array<double, 7>>());
            _agent_statistics_data.back().reserve(1 + this->_time_max / _statisticstime);
        }

        _dgroup_cell_statistics->add_attribute(
            "Stored quantities", "mean, var, mode, min, q25, q50, q75, max");
        _dgroup_cell_statistics->add_attribute("Save time", _statisticstime);

        for (std::size_t i = 0; i < _cell_adaptors.size(); ++i)
        {
            _dsets_cell_statistics.push_back(_dgroup_cell_statistics->open_dataset(
                std::get<0>(_cell_adaptors[i])));

            _cell_statistics_data.push_back(std::vector<std::array<double, 7>>());
            _cell_statistics_data.back().reserve(1 + this->_time_max / _statisticstime);
        }

        // write out cell position and cell id to be able to
        // link them later for agents
        this->_hdfgrp->open_group("grid")
            ->open_dataset("cell_position", {_cellmanager.cells().size()}, {1000})
            ->write(_cellmanager.cells().begin(), _cellmanager.cells().end(),
                    [](auto cell) { return cell->position(); });
        this->_hdfgrp->open_group("grid")
            ->open_dataset("cell_id", {_cellmanager.cells().size()}, {1000})
            ->write(_cellmanager.cells().begin(), _cellmanager.cells().end(),
                    [](auto cell) { return cell->id(); });

        std::reverse(_highres_interval.begin(), _highres_interval.end());
    }

    void initialize_cells()
    {
        // Extract the mode that determines the initial state
        const auto init_celltrait_len =
            as_<std::size_t>(this->_cfg["init_cell_traitlen"]);

        // initialize resource influxes
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
                "Unknown init_cell_resourceinflux given in config, must be "
                "'given' or 'random'");
        }

        // initialize resource maximum capacities
        const auto cell_resourcecapacity_kind =
            as_str(this->_cfg["cellresourcecapacity_kind"]);

        std::vector<double> resourcecapacities;

        if (cell_resourcecapacity_kind == "random")
        {
            const auto cell_resourcecapacity_limits =
                as_vector<double>(this->_cfg["cellresourcecapacity_limits"]);
            std::uniform_real_distribution<double> capdist(
                cell_resourcecapacity_limits[0], cell_resourcecapacity_limits[1]);
            resourcecapacities.resize(init_celltrait_len);
            std::generate(resourcecapacities.begin(), resourcecapacities.end(),
                          [this, &capdist]() { return capdist(*this->_rng); });
        }
        else if (cell_resourcecapacity_kind == "given")
        {
            resourcecapacities =
                as_vector<double>(this->_cfg["cellresourcecapacities"]);
        }
        else
        {
            throw std::runtime_error(
                "Unknown cell_resourcecapacity_kind given in config, must be "
                "'given' or 'random'");
        }

        std::vector<double> init_cellresources(init_celltrait_len, 1.);

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
                Cellstate cs(init_celltrait, init_cellresources,
                             init_cellresourceinfluxes, resourcecapacities);
                return cs;
            },
            _cellmanager.cells());

        // make neighborhoods lookup table
        std::for_each(_cellmanager.cells().begin(), _cellmanager.cells().end(),
                      [this](auto& cell) {
                          _neighborhoods[cell] = MooreNeighbor::neighbors(cell, _cellmanager)
                      });
    }

    void initialize_agents()
    {
        // Extract the mode that determines the initial state
        const auto init_genotypelen =
            as_<std::size_t>(this->_cfg["init_genotypelen"]);
        const auto init_resources = as_double(this->_cfg["init_resources"]);
        const auto init_genotype_values =
            as_array<double, 2>(this->_cfg["init_genotype_values"]);

        // make adaption which is viable
        auto cell = find_cell(_agentmanager.agents()[0], _cellmanager);

        auto agent = _agentmanager.agents()[0];
        std::uniform_real_distribution<double> dist(init_genotype_values[0],
                                                    init_genotype_values[1]);

        bool found = false;

        for (std::size_t i = 0; i < 100000000; ++i)
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

            if (cum_res > _livingcost)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            throw std::runtime_error("Could not build viable organism!");
        }

        // estimate maximum population size and reserve memory accordingly,
        // with some safety margin (factor of 2)
        auto resource_influxlimits =
            as_vector<double>(this->_cfg["resourceinflux_limits"]);

        // reserve memory for a lot of agents:
        _agentmanager.agents().reserve(2 * (resource_influxlimits[1] / _upper_resourcelimit) *
                                       _cellmanager.cells().size());
    }

    void increment_time(const typename Base::Time dt = 1)
    {
        this->_time += dt;
    }

    void print_statistics(const std::vector<std::shared_ptr<AgentType>>& agents,
                          const std::vector<std::shared_ptr<CellType>>& cells)
    {
        ArithmeticMean Mean;
        Maximum Max;
        this->_log->info(
            "Current time: {}\n current populationsize: {}\n"
            " <adaption> {}\n <adaption_size> {}\n <genome_size> {}\n "
            "<phenotype_size> {}\n <resourceinfluxes> {}\n"
            " <resourceinfluxesize> {}\n <celltraitsize> {}\n"
            " max_adaptionsize {}\n max_resourceinfluxsize {}\n "
            "max_celltraitsize {}\n max_genotypesize {}\n max_phenotypesize "
            "{} ",
            this->_time, agents.size(),
            Mean(agents.begin(), agents.end(),
                 [](auto agent) {
                     return std::accumulate(agent->state().adaption.begin(),
                                            agent->state().adaption.end(), 0.);
                 }),
            Mean(agents.begin(), agents.end(),
                 [](auto agent) { return agent->state().adaption.size(); }),
            Mean(agents.begin(), agents.end(),
                 [](auto agent) { return agent->state().genotype.size(); }),
            Mean(agents.begin(), agents.end(),
                 [](auto agent) { return agent->state().phenotype.size(); }),
            Mean(cells.begin(), cells.end(),
                 [](auto cell) {
                     return std::accumulate(cell->state().resourceinfluxes.begin(),
                                            cell->state().resourceinfluxes.end(), 0.);
                 }),
            Mean(cells.begin(), cells.end(),
                 [](auto cell) { return cell->state().resourceinfluxes.size(); }),
            Mean(cells.begin(), cells.end(),
                 [](auto cell) { return cell->state().celltrait.size(); }),
            Max(agents.begin(), agents.end(),
                [](auto agent) { return agent->state().adaption.size(); }),
            Max(cells.begin(), cells.end(),
                [](auto cell) { return cell->state().resourceinfluxes.size(); }),
            Max(cells.begin(), cells.end(),
                [](auto cell) { return cell->state().celltrait.size(); }),
            Max(agents.begin(), agents.end(),
                [](auto agent) { return agent->state().genotype.size(); }),
            Max(agents.begin(), agents.end(),
                [](auto agent) { return agent->state().phenotype.size(); }));
    }

    void perform_step()
    {
        auto& agents = _agentmanager.agents();
        auto& cells = _cellmanager.cells();

        // if ((this->_time % 5000 == 0))
        // {
        //     print_statistics(agents, cells);
        // }

        if (agents.size() == 0)
        {
            return;
        }

        for (auto& agent : agents)
        {
            update_adaption(agent);
        }

        for (auto& cell : cells)
        {
            update_cell(cell);
        }

        std::vector<unsigned> indices(agents.size());
        std::iota(indices.begin(), indices.end(), 0);

        std::shuffle(indices.begin(), indices.end(), *(this->_rng));

        for (auto& idx : indices)
        {
            update_agent(agents[idx]);
        }

        _agentmanager.agents().erase(
            std::remove_if(_agentmanager.agents().begin(),
                           _agentmanager.agents().end(),
                           [](auto agent) { return agent->state().deathflag; }),
            _agentmanager.agents().end());
    }

    /**
     * @brief write out data stuff
     *
     */
    void write_data()
    {
        auto& agents = _agentmanager.agents();
        auto& cells = _cellmanager.cells();

        if (agents.size() == 0)
        {
            return;
        }

        auto curr_hi = _highres_interval.back();

        if (this->_time < curr_hi[1] and this->_time >= curr_hi[0])
        {
            std::size_t chunksize = (agents.size() < 1000) ? (agents.size()) : 1000;

            auto agrp = _dgroup_agents->open_group("t=" + std::to_string(this->_time));

            for (auto& [name, adaptor] : _agent_adaptors)
            {
                agrp->open_dataset(name, {agents.size()}, {chunksize}, 6)
                    ->write(agents.begin(), agents.end(), adaptor);
            }

            auto cgrp = _dgroup_cells->open_group("t=" + std::to_string(this->_time));

            for (auto& [name, adaptor] : _cell_adaptors)
            {
                cgrp->open_dataset(name, {cells.size()}, {256}, 6)
                    ->write(cells.begin(), cells.end(), adaptor);
            }
        }

        if (this->_time % _statisticstime == 0)
        {
            for (std::size_t i = 0; i < _agent_adaptors.size(); ++i)
            {
                _agent_statistics_data[i].push_back(Utils::Describe()(
                    agents.begin(), agents.end(), std::get<1>(_agent_adaptors[i])));
            }

            for (std::size_t i = 0; i < _cell_adaptors.size(); ++i)
            {
                _cell_statistics_data[i].push_back(Utils::Describe()(
                    cells.begin(), cells.end(), std::get<1>(_cell_adaptors[i])));
            }
        }

        if (this->_time > 0 and (this->_time % (_statisticstime * 10) == 0 or
                                 this->_time == this->_time_max))
        {
            for (std::size_t i = 0; i < _dsets_agent_statistics.size(); ++i)
            {
                _dsets_agent_statistics[i]->write(_agent_statistics_data[i]);
                _agent_statistics_data[i].clear();
            }

            for (std::size_t i = 0; i < _dsets_cell_statistics.size(); ++i)
            {
                _dsets_cell_statistics[i]->write(_cell_statistics_data[i]);
                _cell_statistics_data[i].clear();
            }
        }

        if (this->_time == curr_hi[1])
        {
            _highres_interval.pop_back();
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
