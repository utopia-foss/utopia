#ifndef UTOPIA_MODELS_AMEEMULTI_HH
#define UTOPIA_MODELS_AMEEMULTI_HH

#include <cassert>
#include <dune/utopia/base.hh>
#include <dune/utopia/core/apply.hh>
#include <dune/utopia/core/model.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/types.hh>
#include <dune/utopia/models/Amee/Amee.hh>
#include <dune/utopia/models/Amee/agentpolicies/agent_updatepolicy.hh>
#include <dune/utopia/models/Amee/cell_state.hh>
#include <dune/utopia/models/Amee/utils/statistics.hh>
#include <dune/utopia/models/Amee/utils/test_utils.hh>
#include <dune/utopia/models/Amee/utils/utils.hh>
#include <dune/utopia/models/AmeeMulti/adaptionfunctions.hh>

#include <fstream>
#include <functional>

namespace Utopia
{
namespace Models
{
namespace AmeeMulti
{
/**
 * @brief  Amee model class for multiple resources per cell
 *
 * @tparam Cell
 * @tparam CellManager
 * @tparam AgentManager
 * @tparam Modeltypes
 * @tparam construction
 * @tparam decay
 */
template <typename Cell, typename Agent, typename Modeltypes, typename Adaptionfunction, typename RNG, template <typename, typename, typename, typename> class Agentupdatepolicy, bool construction, bool decay>
class AmeeMulti
    : public Model<AmeeMulti<Cell, Agent, Modeltypes, Adaptionfunction, RNG, Agentupdatepolicy, construction, decay>, Modeltypes>,
      public Agentupdatepolicy<Cell, Agent, Adaptionfunction, RNG>

{
public:
    using Gridcell = Cell;
    using Organism = Agent;
    using Cellstate = typename Gridcell::State;
    using Agentstate = typename Organism::State;

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
        Model<AmeeMulti<Cell, Agent, Modeltypes, Adaptionfunction, RNG, Agentupdatepolicy, construction, decay>, Modeltypes>;

    using Updatepolicy = Agentupdatepolicy<Cell, Agent, Adaptionfunction, RNG>;

    /// Data type that holds the configuration
    using Config = typename Base::Config;

    /// Data type of the group to write model data to, holding datasets
    using DataGroup = typename Base::DataGroup;

    /// Data type for a dataset
    using DataSet = typename Base::DataSet;

    // Alias the neighborhood classes to make access more convenient
    using NextNeighbor = Utopia::Neighborhoods::NextNeighbor;
    using MooreNeighbor = Utopia::Neighborhoods::MooreNeighbor;

private:
    using Agentcontainer = typename Updatepolicy::Agentcontainer;

    using Cellcontainer = std::vector<std::shared_ptr<Gridcell>>;

    using AgentAdaptor = std::function<double(const std::shared_ptr<Organism>&)>;
    using AgentAdaptortuple = std::tuple<std::string, AgentAdaptor>;

    using CellAdaptor = std::function<double(const std::shared_ptr<Gridcell>)>;
    using CellAdaptortuple = std::tuple<std::string, CellAdaptor>;

    //  grid
    Cellcontainer _cells;

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
    std::uniform_real_distribution<double> _deathdist;
    std::uniform_real_distribution<double> _resdist;
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

    std::chrono::time_point<std::chrono::high_resolution_clock> _begintime;
    unsigned int _infotime;

public:
    // update sub functions

    /**
     *
     * @brief logistic function for resource input, nonfunctional currently
     *         (K u0 exp(r t))/(K + u (e(r t) - 1))
     * @param r growth rate
     * @param K maximum value
     * @param u0 initial value
     * @param t time to compute the value at
     * @return double current value according to logisitc function
     */
    double logistic_function(double r, double K, double u0, double t)
    {
        return (K * u0 * std::exp(r * t)) / (K + u0 * (std::exp(r * t) - 1.));
    }

    /**
     * @brief function for updating the adaption of an organism
     *
     * @param agent
     */
    void update_adaption(Organism& agent)
    {
        agent.state().adaption = this->_check_adaption(agent);
    }

    /**
     * @brief Function for agent metabolism
     *
     * @param agent
     */
    void metabolism(Organism& agent)
    {
        std::size_t i = agent.state().start;
        std::size_t j = 0;
        double resourcecredit = 0.;
        auto& cell = agent.state().habitat;

        for (; i < unsigned(agent.state().end) && i < cell->state().resources.size();
             ++i, ++j)
        {
            resourcecredit = (cell->state().resources[i] > agent.state().adaption[j])
                                 ? agent.state().adaption[j]
                                 : cell->state().resources[i];

            resourcecredit = (resourcecredit > _upper_resourcelimit) ? _upper_resourcelimit
                                                                     : resourcecredit;

            agent.state().resources += resourcecredit;
            cell->state().resources[i] -= resourcecredit;
        }

        agent.state().resources = (agent.state().resources > _livingcost)
                                      ? (agent.state().resources - _livingcost)
                                      : 0.;
        agent.state().age += 1;
    }

    /**
     * @brief Funciton for habitat modification by agent
     *
     * @param agent
     */
    void modify(Organism& agent)
    {
        auto cell = agent.state().habitat;
        auto& trt = agent.state().phenotype;
        auto& ctrt = cell->state().celltrait;
        int start = agent.state().start_mod;
        int end = agent.state().end_mod;
        double intensity = agent.state().intensity;

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

            if (agent.state().resources < (_reproductioncost + _offspringresources))
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
                    cell->state().resourceinflux[i] = _resdist(*this->_rng);
                    cell->state().modtimes[i] = this->_time;
                }

                // replace value at locus with scaled value of organism phenotype
                double value = intensity * trt[i];
                double cost = _modifiercost * std::abs(value - ctrt[i]);
                if (cost < agent.state().resources)
                {
                    ctrt[i] = value;
                    cell->state().modtimes[i] = this->_time;
                    agent.state().resources -= cost;
                }
            }
        }

        for (int i = min_m; i < min_a; ++i)
        {
            // //this->_log->debug("  appending: i = {} , end = {}", i, end);
            if (agent.state().resources < (_reproductioncost + _offspringresources))
            {
                break;
            }
            else
            {
                double value = intensity * trt[i];
                double cost = _modifiercost * std::abs(value);

                if (cost < agent.state().resources)
                {
                    ctrt.emplace_back(intensity * trt[i]);
                    cell->state().modtimes.emplace_back(this->_time);
                    cell->state().resources.emplace_back(0.);
                    cell->state().resourceinflux.emplace_back(_resdist(*this->_rng));
                    agent.state().resources -= cost;
                }
            }
        }
    }

    /**
     * @brief Function for checking if an organism is to die
     *
     * @param agent
     */
    void kill(Organism& agent)
    {
        if (Amee::Utils::is_equal(agent.state().resources, 0.) or
            _deathdist(*(this->_rng)) < _deathprobability)
        {
            agent.state().deathflag = true;
        }
    }

    /**
     * @brief Function for decaying back the trait of a cell towards its original state
     *
     * @param cell
     */
    void celltrait_decay(const std::shared_ptr<Gridcell>& cell)
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
                cell->state().resourceinflux[i] = 0.;
                times[i] = std::numeric_limits<double>::quiet_NaN();

                // cellresources are left alone, can still be used, but nothing else anymore is done
            }
        }
    }

    /**
     * @brief Function for carrying out the update algorithm for a cell
     *
     * @param cell
     */
    void update_cell(const std::shared_ptr<Gridcell>& cell)
    {
        for (std::size_t i = 0; i < cell->state().celltrait.size(); ++i)
        {
            // cell->state().resources[i] += cell->state().resourceinflux[i];

            // if (cell->state().resources[i] > cell->state().resource_capacities[i])
            // {
            //     cell->state().resources[i] = cell->state().resource_capacities[i];
            // }

            if (Amee::Utils::is_equal(cell->state().resources[i], 0., 1e-7))
            {
                cell->state().resources[i] = cell->state().resourceinflux[i];
            }
            else
            {
                cell->state().resources[i] =
                    logistic_function(cell->state().resourceinflux[i],    // r
                                      cell->state().resource_capacity[i], // K
                                      cell->state().resources[i], 1); // u0, t
            }
        }

        if constexpr (decay)
        {
            celltrait_decay(cell);
        }
    }

    /**
     * @brief Function for carrying out the update algorithm for an agent
     *
     * @param agent
     */
    void update_agent(std::shared_ptr<Organism>& agent)
    {
        update_adaption(*agent);

        this->move(agent);

        update_adaption(*agent);

        if constexpr (construction)
        {
            modify(*agent);
        }

        update_adaption(*agent);

        metabolism(*agent);

        this->reproduce(*agent);

        kill(*agent);
    };

    /**
     * @brief Construct a new Amee Multi object
     *
     * @tparam ParentModel
     * @param name name of the model instance
     * @param parent pseudoparent object
     * @param cells  cell vector which can be obtained from a cellmanager object
     */
    template <class ParentModel>
    AmeeMulti(const std::string name,
              ParentModel& parent,
              const Cellcontainer& cells,
              Adaptionfunction adaptionfunc,
              std::vector<AgentAdaptortuple> agentadaptors,
              std::vector<CellAdaptortuple> celladaptors)
        : Base(name, parent),
          Updatepolicy(
              this->_rng,
              adaptionfunc,
              as_double(this->_cfg["reproductioncost"]) +
                  as_double(this->_cfg["offspringresources"]),
              as_<std::size_t>(this->_cfg["threadnum"]),
              as_double(this->_cfg["reproductioncost"]),
              as_double(this->_cfg["offspringresources"]),
              std::vector<double>{as_double(this->_cfg["substitutionrate"]),
                                  as_double(this->_cfg["insertionrate"]),
                                  as_double(this->_cfg["substitution_std"])}),
          _cells(cells),
          _decayintensity(as_double(this->_cfg["decayintensity"])),
          _removethreshold(as_double(this->_cfg["removethreshold"])),
          _livingcost(as_double(this->_cfg["livingcost"])),
          _deathprobability(as_double(this->_cfg["deathprobability"])),
          _modifiercost(as_double(this->_cfg["modifiercost"])),
          _upper_resourcelimit(as_double(this->_cfg["upper_resourcelimit"])),
          _highres_interval(as_vector<std::array<unsigned, 2>>(
              this->_cfg["highresinterval"])),
          _statisticstime(as_<unsigned>(this->_cfg["statisticstime"])),
          _deathdist(std::uniform_real_distribution<double>(0., 1.)),
          _resdist(std::uniform_real_distribution<double>(
              as_vector<double>(this->_cfg["resourceinflux_limits"])[0],
              as_vector<double>(this->_cfg["resourceinflux_limits"])[1])),
          _dgroup_agents(this->_hdfgrp->open_group("Agents")),
          _dgroup_cells(this->_hdfgrp->open_group("Cells")),
          _dgroup_agent_statistics(
              this->_hdfgrp->open_group("Agent_statistics")),
          _dgroup_cell_statistics(this->_hdfgrp->open_group("Cell_statistics")),
          _agent_adaptors(agentadaptors),
          _cell_adaptors(celladaptors),
          _idx(0),
          _begintime(std::chrono::high_resolution_clock::now()),
          _infotime(as_<unsigned>(this->_cfg["infotime"]))
    {
        this->_log->info(" initializing cells");
        initialize_cells();
        this->adjust_grid(_cells);

        this->_log->info(" initialize agents");
        // Extract the mode that determines the initial state
        const auto init_genotypelen =
            as_<std::size_t>(this->_cfg["init_genotypelen"]);
        const auto init_resources = as_double(this->_cfg["init_resources"]);
        const auto init_genotype_values =
            as_array<double, 2>(this->_cfg["init_genotype_values"]);

        std::shared_ptr<Cell> eden =
            _cells[std::uniform_int_distribution<std::size_t>(0, _cells.size() - 1)(*this->_rng)];

        this->initialize_agents(eden, init_genotypelen, init_resources,
                                init_genotype_values, _livingcost);

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
            ->open_dataset("cell_position", {_cells.size()}, {1000})
            ->write(_cells.begin(), _cells.end(),
                    [](auto cell) { return cell->position(); });
        this->_hdfgrp->open_group("grid")
            ->open_dataset("cell_id", {_cells.size()}, {1000})
            ->write(_cells.begin(), _cells.end(),
                    [](auto cell) { return cell->id(); });

        std::reverse(_highres_interval.begin(), _highres_interval.end());
    }

    /**
     * @brief Initialize the cell with parameters from config.
     *
     */
    void initialize_cells()
    {
        // Extract the mode that determines the initial state
        const auto init_celltrait_len =
            as_<std::size_t>(this->_cfg["init_cell_traitlen"]);

        // initialize resource influxes
        const auto init_cell_resourceinflux_kind =
            as_str(this->_cfg["init_cellresourceinflux_kind"]);

        std::vector<double> init_cellresourceinflux;
        if (init_cell_resourceinflux_kind == "random")
        {
            init_cellresourceinflux.resize(init_celltrait_len);
            std::generate(init_cellresourceinflux.begin(),
                          init_cellresourceinflux.end(),
                          [this]() { return _resdist(*this->_rng); });
        }
        else if (init_cell_resourceinflux_kind == "given")
        {
            init_cellresourceinflux =
                as_vector<double>(this->_cfg["init_cell_influxvalues"]);
            if (init_celltrait_len != init_cellresourceinflux.size())
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

        std::vector<double> resourcecapacity;

        if (cell_resourcecapacity_kind == "random")
        {
            const auto cell_resourcecapacity_limits =
                as_vector<double>(this->_cfg["cellresourcecapacity_limits"]);
            std::uniform_real_distribution<double> capdist(
                cell_resourcecapacity_limits[0], cell_resourcecapacity_limits[1]);
            resourcecapacity.resize(init_celltrait_len);
            std::generate(resourcecapacity.begin(), resourcecapacity.end(),
                          [this, &capdist]() { return capdist(*this->_rng); });
        }
        else if (cell_resourcecapacity_kind == "given")
        {
            resourcecapacity =
                as_vector<double>(this->_cfg["cellresourcecapacity"]);
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

        std::for_each(_cells.begin(), _cells.end(), [&](auto cell) {
            cell->state() = Cellstate(init_celltrait, init_cellresources,
                                      init_cellresourceinflux, resourcecapacity, 0);
        });
    }

    /**
     * @brief overload of increment_time which is exposed here
     *
     * @param dt
     */
    void increment_time(const typename Base::Time dt = 1)
    {
        this->_time += dt;
    }

    /**
     * @brief Print statistics of agents and cells, mean and max of quantities.
     *
     */
    void print_statistics()
    {
        Amee::ArithmeticMean Mean;
        Amee::Maximum Max;
        this->_log->info("Current time: {}\n current populationsize: {}\n",
                         this->_time, this->_population.size());

        this->_log->info(
            "Agents: \n"
            "\n <cum_adaption> {}\n <adaption_size> {}\n <genome_size> {}\n "
            "<phenotype_size> {}\n <resources> {}\n",
            Mean(this->_population.begin(), this->_population.end(),
                 [](auto agent) {
                     return std::accumulate(agent->state().adaption.begin(),
                                            agent->state().adaption.end(), 0.);
                 }),
            Mean(this->_population.begin(), this->_population.end(),
                 [](auto agent) { return agent->state().adaption.size(); }),
            Mean(this->_population.begin(), this->_population.end(),
                 [](auto agent) { return agent->state().genotype.size(); }),
            Mean(this->_population.begin(), this->_population.end(),
                 [](auto agent) { return agent->state().phenotype.size(); }),
            Mean(this->_population.begin(), this->_population.end(),
                 [](auto agent) { return agent->state().resources; }));

        this->_log->info(
            "\n MAX(cum_adaption) {}\n MAX(adaption_size) {}\n "
            "MAX(genome_size) "
            "{}\n "
            "MAX(phenotype_size) {}\n MAX(resources) {}\n",
            Max(this->_population.begin(), this->_population.end(),
                [](auto agent) {
                    return std::accumulate(agent->state().adaption.begin(),
                                           agent->state().adaption.end(), 0.);
                }),
            Max(this->_population.begin(), this->_population.end(),
                [](auto agent) { return agent->state().adaption.size(); }),
            Max(this->_population.begin(), this->_population.end(),
                [](auto agent) { return agent->state().genotype.size(); }),
            Max(this->_population.begin(), this->_population.end(),
                [](auto agent) { return agent->state().phenotype.size(); }),
            Max(this->_population.begin(), this->_population.end(),
                [](auto agent) { return agent->state().resources; }));

        this->_log->info(
            "\n Cells: "
            "\n <cum_resourceinflux> {}\n"
            " <resourceinfluxize> {}\n <celltraitsize> {}\n <resources> {}",
            Mean(_cells.begin(), _cells.end(),
                 [](auto cell) {
                     return std::accumulate(cell->state().resourceinflux.begin(),
                                            cell->state().resourceinflux.end(), 0.);
                 }),
            Mean(_cells.begin(), _cells.end(),
                 [](auto cell) { return cell->state().resourceinflux.size(); }),
            Mean(_cells.begin(), _cells.end(),
                 [](auto cell) { return cell->state().celltrait.size(); }),
            Mean(_cells.begin(), _cells.end(), [](auto cell) {
                return std::accumulate(cell->state().resources.begin(),
                                       cell->state().resources.end(), 0.);
            }));

        this->_log->info(
            "\n MAX(cum_resourceinflux) {}"
            "\n MAX(resourceinfluxize) {} \n MAX(celltraitsize) {}"
            "\n MAX(resources) {}",
            Max(_cells.begin(), _cells.end(),
                [](auto cell) {
                    return std::accumulate(cell->state().resourceinflux.begin(),
                                           cell->state().resourceinflux.end(), 0.);
                }),
            Max(_cells.begin(), _cells.end(),
                [](auto cell) { return cell->state().resourceinflux.size(); }),
            Max(_cells.begin(), _cells.end(),
                [](auto cell) { return cell->state().celltrait.size(); }),
            Max(_cells.begin(), _cells.end(), [](auto cell) {
                return std::accumulate(cell->state().resources.begin(),
                                       cell->state().resources.end(), 0.);
            }));

        this->_log->info(
            "##################################################\n");
    }

    /**
     * @brief Perform a single timestep
     *
     */
    void perform_step()
    {
        if ((this->_time % _infotime == 0))
        {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                               std::chrono::high_resolution_clock::now() - _begintime)
                               .count();

            auto remaining =
                this->_time == 0
                    ? 1
                    : (((static_cast<double>(elapsed) / this->_time) * this->_time_max) -
                       static_cast<double>(elapsed));

            this->_log->info(
                "T {}, N {}, elapsed time {} s, estimated remaining time {} s",
                this->_time, _population.size(), elapsed, remaining);
        }

            if (this->_population.size() == 0)
            {
                return;
            }

            for (auto& agent : this->_population)
            {
                update_adaption(*agent);
            }

            for (auto& cell : _cells)
            {
                update_cell(cell);
            }

            std::shuffle(this->_population.begin(), this->_population.end(), *(this->_rng));
            std::size_t size = this->_population.size();

            for (std::size_t i = 0; i < size; ++i)
            {
                update_agent(this->_population[i]);
            }

            this->_population.erase(
                std::remove_if(
                    this->_population.begin(), this->_population.end(),
                    [](auto agent) { return agent->state().deathflag; }),
                this->_population.end());

            if (this->_time == this->_time_max - 1)
            {
                print_statistics();
            }
        }
        else
        {
            if ((this->_time % 1 == 0))
            {
                this->_log->info("T {}, N {}, elapsed time {} s", this->_time,
                                 std::accumulate(this->_population.begin(),
                                                 this->_population.end(), 0,
                                                 [](std::size_t N, auto& pair) {
                                                     return N + pair.second.size();
                                                 }),
                                 std::chrono::duration_cast<std::chrono::seconds>(
                                     std::chrono::high_resolution_clock::now() - _begintime)
                                     .count());
            }

            for (auto& [id, local_population] : this->_population)
            {
                for (auto& agent : local_population)
                {
                    update_adaption(*agent);
                }
            }

            for (auto& cell : _cells)
            {
                update_cell(cell);
            }

            for (auto& [id, local_population] : this->_population)
            {
                std::shuffle(local_population.begin(), local_population.end(), *this->_rng);
                std::size_t size = local_population.size();

                for (std::size_t i = 0; i < size; ++i)
                {
                    update_agent(local_population[i]);
                }

                local_population.erase(
                    std::remove_if(local_population.begin(), local_population.end(),
                                   [](const auto& agent) {
                                       return agent->state().deathflag;
                                   }),
                    local_population.end());
            }
        }
    }

    /**
     * @brief write out data stuff
     *
     */
    void write_data()
    {
        if constexpr (!Updatepolicy::parallel)
        {
            if (this->_population.size() == 0)
            {
                return;
            }
            if (_highres_interval.size() != 0)
            {
                auto curr_hi = _highres_interval.back();

                if (this->_time < curr_hi[1] and this->_time >= curr_hi[0])
                {
                    std::size_t chunksize = (this->_population.size() < 1000)
                                                ? (this->_population.size())
                                                : 1000;

                    auto agrp =
                        _dgroup_agents->open_group("t=" + std::to_string(this->_time));

                    for (auto& [name, adaptor] : _agent_adaptors)
                    {
                        agrp->open_dataset(name, {this->_population.size()}, {chunksize}, 6)
                            ->write(this->_population.begin(),
                                    this->_population.end(), adaptor);
                    }

                    auto cgrp =
                        _dgroup_cells->open_group("t=" + std::to_string(this->_time));

                    for (auto& [name, adaptor] : _cell_adaptors)
                    {
                        cgrp->open_dataset(name, {_cells.size()}, {256}, 6)
                            ->write(_cells.begin(), _cells.end(), adaptor);
                    }
                }

                if (this->_time == curr_hi[1])
                {
                    _highres_interval.pop_back();
                }
            }

            if (this->_time % _statisticstime == 0)
            {
                for (std::size_t i = 0; i < _agent_adaptors.size(); ++i)
                {
                    _agent_statistics_data[i].push_back(Amee::Utils::Describe()(
                        this->_population.begin(), this->_population.end(),
                        std::get<1>(_agent_adaptors[i])));
                }

                for (std::size_t i = 0; i < _cell_adaptors.size(); ++i)
                {
                    _cell_statistics_data[i].push_back(Amee::Utils::Describe()(
                        _cells.begin(), _cells.end(), std::get<1>(_cell_adaptors[i])));
                }
            }
        }
        else
        {
            auto N = std::accumulate(
                this->_population.begin(), this->_population.end(), 0,
                [](std::size_t N, auto& pair) { return N + pair.second.size(); });
            if (N == 0)
            {
                return;
            }
            else
            {
                if (_highres_interval.size() != 0)
                {
                    auto curr_hi = _highres_interval.back();

                    if (this->_time < curr_hi[1] and this->_time >= curr_hi[0])
                    {
                        std::size_t chunksize = (N < 1000) ? N : 1000;

                        auto agrp = _dgroup_agents->open_group(
                            "t=" + std::to_string(this->_time));

                        for (auto& [name, adaptor] : _agent_adaptors)
                        {
                            auto ds = agrp->open_dataset(name, {H5S_UNLIMITED},
                                                         {chunksize}, 6);
                            for (auto& [id, local_population] : this->_population)
                            {
                                ds->write(local_population.begin(),
                                          local_population.end(), adaptor);
                            }
                        }

                        auto cgrp =
                            _dgroup_cells->open_group("t=" + std::to_string(this->_time));

                        for (auto& [name, adaptor] : _cell_adaptors)
                        {
                            cgrp->open_dataset(name, {_cells.size()}, {256}, 6)
                                ->write(_cells.begin(), _cells.end(), adaptor);
                        }
                    }

                    if (this->_time == curr_hi[1])
                    {
                        _highres_interval.pop_back();
                    }
                }

                if (this->_time % _statisticstime == 0)
                {
                    for (std::size_t i = 0; i < _agent_adaptors.size(); ++i)
                    {
                        Amee::Utils::DescribeOnline agent_descriptor(
                            std::get<1>(_agent_adaptors[i]));

                        for (auto& [id, local_population] : this->_population)
                        {
                            agent_descriptor.update(local_population.begin(),
                                                    local_population.end());
                        }
                        _agent_statistics_data[i].push_back(agent_descriptor.get_result());
                    }

                    for (std::size_t i = 0; i < _cell_adaptors.size(); ++i)
                    {
                        _cell_statistics_data[i].push_back(Amee::Utils::Describe()(
                            _cells.begin(), _cells.end(), std::get<1>(_cell_adaptors[i])));
                    }
                }
            }
        }

        // save statistics stuff
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
    }

    const auto& population()
    {
        return this->_population;
    }

    const auto& cells()
    {
        return _cells;
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

    virtual ~AmeeMulti() = default;
}; // namespace AmeeMulti

} // namespace AmeeMulti
} // namespace Models
} // namespace Utopia

#endif // UTOPIA_MODELS_COPYME_HH
