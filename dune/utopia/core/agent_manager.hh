#ifndef UTOPIA_CORE_AGENTMANAGER_HH
#define UTOPIA_CORE_AGENTMANAGER_HH

#include "types.hh"
#include "agent_new.hh"         // NOTE Use agent.hh eventually

namespace Utopia {
/**
 * \addtogroup AgentManager
 * \{
 */

template<class AgentTraits, class Model>
class AgentManager {
public:
    /// The type of this AgentManager
    using Self = AgentManager<AgentTraits, Model>;

    /// The type of the managed agents
    using Agent = __Agent<AgentTraits>; // NOTE Use Agent eventually

    /// The type of the agent state
    using AgentStateType = typename AgentTrait::State;

    /// The type of the vectors that represent physical quantities
    using SpaceVec = SpaceVecType<dim>;

    /// The type of the move function type
    using MoveFunc = std::function<void(const Agent&, const SpaceVec&)>;

    /// The random number generator type
    using RNG = typename Model::RNG;

private:
    // -- Members --------–––––-------------------------------------------------
    /// The logger (same as the model this manager resides in)
    const std::shared_ptr<spdlog::logger> _log;

    /// Agent manager configuration node
    const DataIO::Config _cfg;

    /// The model's random number generator
    const std::shared_ptr<RNG> _rng;

    /// The physical space the agents are to reside in
    const std::shared_ptr<Space> _space;

    /// Storage container for agents
    AgentContainer<Agent> _agents;

    /// ID counter: ID of the globally latest created agent
    static IndexType _id_counter;

    /// The move_to function that will be used for moving agents
    const MoveFunc _mv_func;


public:
    // -- Constructors ---------------------------------------------------------

    AgentManager (Model& model,
                  const DataIO::Config& custom_cfg) = {})
    :
        _log(model.get_logger()),
        _cfg(setup_cfg(model, custom_cfg)),
        _rng(model.get_rng()),
        _space(model.get_space()),
        _id_counter(0),
        _agents(setup_agents()),
        _mv_func(setup_mv_func())
    {
        _log->info("AgentManager is all set up.");
    }


    AgentManager(Model& model,
                 const AgentStateType initial_state,
                 const DataIO::Config& custom_cfg) = {})
    :
        _log(model.get_logger()),
        _cfg(setup_cfg(model, custom_cfg)),
        _rng(model.get_rng()),
        _space(model.get_space()),
        _id_counter(0),
        _agents(setup_agents()),
        _mv_func(setup_mv_func())
    {
        _log->info("AgentManager is all set up.");
    }

    /// -- Getters -------------------------------------------------------------
    /// Return pointer to the space, for convenience
    const std::shared_ptr<Space>& space () const {
        return _space;
    }

    /// Return const reference to the managed agents
    const AgentContainer<Agent>& agents () const {
        return _agents;
    }

    const IndexType id_counter () const {
        return _id_counter;
    }

    // -- Public interface -----------------------------------------------------
    // ...

private:
    // -- Helper functions -----------------------------------------------------
    // ...

    // -- Setup functions ------------------------------------------------------

    /// Set up the agent manager configuration member
    /** \detail This function determines whether to use a custom configuration
      *         or the one provided by the model this AgentManager belongs to
      */
    DataIO::Config setup_cfg(Model& model, const DataIO::Config& custom_cfg)
    {
        auto cfg = model.get_cfg();

        if (custom_cfg.size() > 0) {
            _log->debug("Using custom config for cell manager setup ...");
            cfg = custom_cfg;
        }
        else {
            _log->debug("Using '{}' model's configuration for cell manager "
                        "setup ... ", model.get_name());
        }
        return cfg;        
    }


    /// Set up the agents container with initial states
    /** \detail This function creates a container with agents that get an
     *          initial state.
     * 
     * \param initial_state The initial state of the agents
     * \param num_agents The number of agents
     * \return AgentContainer<Agent> The agent container
     */
    AgentContainer<Agent> setup_agents(const AgentStateType initial_state,
                                       const IndexType num_agents)
    {
        AgentContainer<Agent> agents;

        // Construct all the agents
        for (IndexType i=0; i<num_agents; ++i){
            agents.emplace_back(std::make_shared<Agent>(_id_counter, 
                                                        initial_state));

            // Increase the id count
            ++_id_counter;
        }
    }


    /// Set up agents container via initial state from config or default constr
    /** \detail This function creates an initial state object and then passes it
      *         over to setup_agents(initial_state, num_agents). It checks 
      *         whether the AgentStateType is constructible via a config node 
      *         and if the config entries to construct it are available. 
      *         It can fall back to try the default constructor to construct 
      *         the object. If both are not possible or the configuration 
      *         was invalid, a run time error message is emitted.
      */
    AgentContainer<Agent> setup_agents() {
        // Find out the agent initialization mode
        if (not _cfg["agent_initialize_from"]) {
            throw std::invalid_argument("Missing required configuration key "
                "'agent_initialize_from' for setting up agents via a "
                "DataIO::Config& constructor or default constructor.");
        }
        const auto agent_init_from = as_str(_cfg["agent_initialize_from"]);

        _log->info("Creating initial agent state using '{}' constructor ...",
                   agent_init_from);

        // Find out if the initial state is constructible via a config node and
        // setup the agents with that information, if configured to do so.
        if constexpr (std::is_constructible<AgentStateType, DataIO::Config&>()){
            // Find out if this constructor was set to be used
            if (agent_init_from == "config") {
                // Yes. Should now check if the required config parameters were
                // also provided and add helpful error message
                if (not _cfg["agent_initial_state"]) {
                    throw std::invalid_argument("Was configured to create the "
                        "initial agent state from a config node but a node "
                        "with the key 'agent_initial_state' was not provided!");
                }

                // Extract the number of agents that need to be set up
                if (not _cfg["agent_initial_number"]) {
                    throw std::invalid_argument("Was configured to create the "
                        "initial agents from a config node but a node "
                        "with the key 'agent_initial_number' was not provided!
                        The number of agents to create is not known!");
                }

                // Everything ok. Create state object and pass it on ...
                return setup_agents(AgentStateType(_cfg["agent_initial_state"]),
                                    as_<Indextype>(_cfg["agent_initial_number"]));
            }
            // else: do not return but continue with the rest of the function,
            // i.e. trying the other constructors
        }
        
        // Either not Config-constructible or not configured to do so.

        // TODO could add a case here where the agent state constructor takes
        //      care of setting up each _individual_ agent such that agents can
        //      have varying initial states.

        // Last resort: Can and should the default constructor be used?
        if constexpr (std::is_default_constructible<AgentStateType>()) {
            if (agent_init_from == "default") {
                return setup_agents(AgentStateType{}, 0);
            }
        }
        
        // If we reached this point, construction does not work.
        throw std::invalid_argument("No valid constructor for the agents' "
            "initial state was available! Check that the config parameter "
            "'agent_initialize_from' is valid (was: '" + agent_init_from + "', "
            "may be 'config' or 'default') and make sure AgentTraits::State is "
            "constructible via the chosen way: "
            "This requires either `const Utopia::DataIO::Config&` as argument "
            "or being default-constructible, respectively. Alternatively, "
            "pass the initial state directly to the AgentManager constructor.");
    }


    void setup_mv_func(){
        // periodic
        if (_space.periodic == true) {
            // synchronoeous update
            if (AgentTraits::is_sync == UpdateMode::sync){
                _mv_func = [](const Agent& agent, const SpaceVec& pos){
                    // Implement
                };
            }
            // asynchroneous update
            else{
                _mv_func = [](const Agent& agent, const SpaceVec& pos){
                    // Implement
                }
            }
        }

        // nonperiodic
        else{
            if (AgentTraits::is_sync == UpdateMode::sync){
                _mv_func = [](const Agent& agent, const SpaceVec& pos){
                    // Implement
                };
            }
            // asynchroneous update
            else{
                _mv_func = [](const Agent& agent, const SpaceVec& pos){
                    // Implement
                }
            }
        }
    }

};

// end group AgentManager
/**
 *  \}
 */

} //namespace Utopia

#endif // UTOPIA_CORE_AGENTMANAGER_HH