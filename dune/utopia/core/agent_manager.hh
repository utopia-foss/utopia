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

    /// The type of the space
    using Space = typename Model::Space;

    /// The type of the managed agents
    using Agent = __Agent<AgentTraits, Space>; // NOTE Use Agent eventually

    /// The type of the agent state
    using AgentStateType = typename AgentTraits::State;

    /// The dimensionality of the space
    static constexpr DimType dim = Space::dim;

    /// The type of the vectors that represent physical quantities
    using SpaceVec = SpaceVecType<dim>;

    /// The type of the move function type
    using MoveFunc = std::function<void(const Agent&, const SpaceVec&)>;

    /// The random number generator type
    using RNG = typename Model::RNG;

    /// Whether the agents are updated synchronously
    static constexpr bool sync = AgentTraits::sync;

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
    IndexType _id_counter;

    /// The move_to function that will be used for moving agents
    MoveFunc _move_to_func;


public:
    // -- Constructors ---------------------------------------------------------

    AgentManager(Model& model,
                 const DataIO::Config& custom_cfg = {})
    :
        _log(model.get_logger()),
        _cfg(setup_cfg(model, custom_cfg)),
        _rng(model.get_rng()),
        _space(model.get_space()),
        _agents(setup_agents()),
        _id_counter(0),
        _move_to_func(setup_move_to_func())
    {
        _log->info("AgentManager is all set up.");
    }


    AgentManager(Model& model,
                 const AgentStateType initial_state,
                 const DataIO::Config& custom_cfg = {})
    :
        _log(model.get_logger()),
        _cfg(setup_cfg(model, custom_cfg)),
        _rng(model.get_rng()),
        _space(model.get_space()),
        _agents(setup_agents()),
        _id_counter(0),
        _move_to_func(setup_move_to_func())
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

    /// Return the ID counter
    IndexType id_counter () const {
        return _id_counter;
    }

    // -- Public interface -----------------------------------------------------
    /// Move an agent to a new position in the space
    void move_to(const std::shared_ptr<Agent>& agent, const SpaceVec& pos){
        _move_to_func(*agent, pos);
    }

    /// Move an agent by a position vector
    /** \detail The agent's new position is the vector sum of the old position
     *          vector and the move_pos vector. Thus, the movement is relative
     *          to the current position.
     */
    void move_by(const std::shared_ptr<Agent>& agent, const SpaceVec& pos_vec){
        _move_to_func(*agent, agent->position() + pos_vec);
    }

    /// Update the agents
    /** \detail This function is needed in the case of synchroneous update.
     *          It updates the state and position from the agent's state
     *          and position cache varibles.
     * 
     * \note In the case of asynchroneous agent update this function will not 
     *       do anything because states and positions are not cached.
     *       There is no need to update these agent traits.
     */
    void update_agents(){
        if constexpr (sync == UpdateMode::sync){
            // Do nothing because there is no cache variable
            return;
        }
        else{
            // Go through all agents and update them
            for (const auto& agent : _agents){
                agent->update();
            }
        }
    }


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

        return agents;
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
                        "with the key 'agent_initial_number' was not provided! "
                        "The number of agents to create is not known!");
                }

                // Everything ok. Create state object and pass it on ...
                return setup_agents(AgentStateType(_cfg["agent_initial_state"]),
                                    as_<IndexType>(_cfg["agent_initial_number"]));
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


    /// Setup the move_to function, which changes an agents position in space
    /** \detail The function that is used to move an agent to a new position in
     *          space depends on whether the space is periodic or not.
     *          In the case of a periodic space the position is automatically
     *          mapped into space again across the borders. For a nonperiodic
     *          space a position outside of the borders will throw an error.
     * 
     * \note The new agent position is either set directly in the case of
     *       synchroneous update or set in the cache variable pos_new for
     *       asynchroneous update. In the latter case, you need to call
     *       the update method in the AgentManager such that the new position
     *       is actually set.
     */
    MoveFunc setup_move_to_func(){
        // periodic
        if (_space->periodic == true) {
            return [this](const Agent& agent, const SpaceVec& pos){
                // Set the new agent position
                agent.set_pos(this->_space->map_into_space(pos));
            };
        }

        // nonperiodic
        else{
            return [this](const Agent& agent, const SpaceVec& pos){
                // Check that the position is contained in the space
                this->_space->contains(pos);

                // Set the new agent position
                agent.set_pos(pos);
            };
        }
    }
};

// end group AgentManager
/**
 *  \}
 */

} //namespace Utopia

#endif // UTOPIA_CORE_AGENTMANAGER_HH