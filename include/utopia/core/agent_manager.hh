#ifndef UTOPIA_CORE_AGENTMANAGER_HH
#define UTOPIA_CORE_AGENTMANAGER_HH

#include <algorithm>
#include <string_view>

#include "types.hh"
#include "exceptions.hh"
#include "agent.hh"
#include "select.hh"

namespace Utopia {
/**
 * \addtogroup AgentManager
 * \{
 */

/// The agent manager manages the agents living in a model
/** \details The agent manager holds a container with all agents that live in
 *          a model space. It provides dynamic functions such as to move models
 *          within a space and ensures that the agents move correctly and
 *          are never allowed to leave the allowed space.
 *          Further, all agents get an id that is unique among all existing
 *          agents (also with respect to multiple agent managers).
 *
 *  \tparam AgentTraits  Specialized Utopia::AgentTraits describing the kind of
 *                       agents this manager should manage
 *  \tparam Model        The model this AgentManager resides in
 */
template<class AgentTraits, class Model>
class AgentManager{
public:
    /// The type of this AgentManager
    using Self = AgentManager<AgentTraits, Model>;

    /// The type of the space
    using Space = typename Model::Space;

    /// The type of the managed agents
    using Agent = Utopia::Agent<AgentTraits, Space>;

    /// Alias for entity type; part of the shared interface of entity managers
    using Entity = Agent;

    /// The type of the agent state
    using AgentState = typename AgentTraits::State;

    /// The dimensionality of the space
    static constexpr DimType dim = Space::dim;

    /// The type of the vectors that represent physical quantities
    using SpaceVec = SpaceVecType<dim>;

    /// The type of the move function type
    using MoveFunc = std::function<void(Agent&, const SpaceVec&)>;

    /// The type of the function that prepares the position of a new agent
    using PosFunc = std::function<SpaceVec(const SpaceVec&)>;

    /// The random number generator type
    using RNG = typename Model::RNG;


private:
    // -- Members --------–––––------------------------------------------------
    /// Counts the number of agents created with this manager, used for new IDs
    IndexType _id_counter;

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

    /// Function that will be used for moving agents, called by move_* methods
    MoveFunc _move_to_func;

    /// Function that will be used to prepare positions for adding an agent
    PosFunc _prepare_pos;


public:
    // -- Constructors --------------------------------------------------------

    /// Construct an agent manager
    /** \details With the model available, the AgentManager can extract the
      *         required information from the model without the need to pass
      *         it explicitly. Furthermore, this constructor differs to the
      *         one with the `initial_state` and `num_agents` parameters such
      *         that the way the initial state of the agents is determined can
      *         be controlled via the configuration.
      *
      * \param  model           The model this AgentManager belongs to
      * \param  custom_cfg      A custom config node to use to use for grid and
      *                         agent setup. If not given, the model's
      *                         configuration is used to extract the required
      *                         entries.
     */
    AgentManager(Model& model,
                 const DataIO::Config& custom_cfg = {})
    :
        _id_counter(0),
        _log(model.get_logger()),
        _cfg(setup_cfg(model, custom_cfg)),
        _rng(model.get_rng()),
        _space(model.get_space()),
        _agents(),
        _move_to_func(setup_move_to_func()),
        _prepare_pos(setup_prepare_pos_func())
    {
        setup_agents();
        _log->info("AgentManager is all set up.");
    }

    /// Construct an agent manager, using the same initial state for all agents
    /** \details The initial state of the agents is explicitly passed to the
      *         constructor.
      *
      * \param  model           The model this AgentManager belongs to
      * \param  initial_state   The initial state of the agents
      * \param  custom_cfg      A custom config node to use to use for grid and
      *                         agent setup. If not given, the model's
      *                         configuration is used to extract the required
      *                         entries.
     */
    AgentManager(Model& model,
                 const AgentState initial_state,
                 const DataIO::Config& custom_cfg = {})
    :
        _id_counter(0),
        _log(model.get_logger()),
        _cfg(setup_cfg(model, custom_cfg)),
        _rng(model.get_rng()),
        _space(model.get_space()),
        _agents(),
        _move_to_func(setup_move_to_func()),
        _prepare_pos(setup_prepare_pos_func())
    {
        setup_agents(initial_state);
        _log->info("AgentManager is all set up.");
    }

    /// -- Getters ------------------------------------------------------------
    /// Return the logger of this AgentManager
    const auto& log () const {
        return _log;
    }

    /// Return the configuration used for building this AgentManager
    const DataIO::Config& cfg () const {
        return _cfg;
    }

    /// Return a reference to the shared random number generator
    const std::shared_ptr<RNG>& rng () const {
        return _rng;
    }

    /// Return pointer to the space, for convenience
    const std::shared_ptr<Space>& space () const {
        return _space;
    }

    /// Return const reference to the managed agents
    const AgentContainer<Agent>& agents () const {
        return _agents;
    }

    /// Return const reference to the entities managed by this manager: agents
    const AgentContainer<Agent>& entities () const {
        return agents();
    }

    /// Return the ID counter
    IndexType id_counter () const {
        return _id_counter;
    }

    // -- Public interface ----------------------------------------------------
    /// Move an agent to a new position in the space
    void move_to(const std::shared_ptr<Agent>& agent,
                 const SpaceVec& pos) const
    {
        _move_to_func(*agent, pos);
    }

    /// Move an agent to a new position in the space
    void move_to(Agent& agent,
                 const SpaceVec& pos) const
    {
        _move_to_func(agent, pos);
    }

    /// Move an agent relative to its current position
    void move_by(const std::shared_ptr<Agent>& agent,
                 const SpaceVec& move_vec) const
    {
        _move_to_func(*agent, agent->position() + move_vec);
    }

    /// Move an agent relative to its current position
    void move_by(Agent& agent,
                 const SpaceVec& move_vec) const
    {
        _move_to_func(agent, agent.position() + move_vec);
    }


    /// Create an agent and associate it with this AgentManager
    /** \details Adds an agent with the specified state to the managed
     *          container of agents. It is checked whether the given position
     *          is valid; if space is periodic, any position outside the space
     *          is mapped back into the space.
     *
     *  \param state the state of the agent that is to be added
     *  \param pos the position of the agent that is to be added
    */
    const std::shared_ptr<Agent>&
        add_agent (const AgentState& state, SpaceVec pos)
    {
        _log->trace("Creating agent with ID {:d} ...", _id_counter);
        _agents.emplace_back(
            std::make_shared<Agent>(_id_counter, state, _prepare_pos(pos))
        );
        ++_id_counter;

        return _agents.back();
    }

    /// add_agent overload for auto-constructed agent states
    /** \details Adds an agent with a specified position and an auto-constructed
      *         AgentState depending on the setup, either default-constructed
      *         if the flag was set in the AgentTraits or config-constructed
      *         with or without use of the RNG.
      *
      * \param pos        The position of the newly created agent
      * \param custom_cfg A custom configuration. If not given, will use the
      *         configuration given at initialization.
      */
    const std::shared_ptr<Agent>&
        add_agent (SpaceVec pos, const DataIO::Config& custom_cfg = {})
    {
        // Check if flag for default constructor was set
        if constexpr (AgentTraits::use_default_state_constructor) {
            if (custom_cfg.size()) {
                throw std::runtime_error("custom_cfg was passed but "
                    "AgentTraits specified use of default constructor!");
            }

            return add_agent(AgentState(), pos);
        }
        else {
            // Determine whether to use the given custom configuration or the
            // one  passed at construction of AgentManager
            DataIO::Config cfg;

            if (custom_cfg.size()) {
                cfg = custom_cfg;
            }
            else {
                cfg = _cfg["agent_params"];
            }

            // Distinguish the two config-constructible states
            if constexpr (std::is_constructible<AgentState,
                                                const DataIO::Config&,
                                                const std::shared_ptr<RNG>&
                                                >())
            {
                return add_agent(AgentState(cfg, _rng), pos);
            }
            else {
                // Should be config-constructible
                return add_agent(AgentState(cfg), pos);
            }
        }

    }
    /// add_agent overload for auto constructed states and random position
    /** \details Add an agent with a random position and an auto-constructed
      *         AgentState depending on the setup, either default constructed
      *         if the flag was set in the AgentTraits or config constructed
      *         with or without use of the RNG.
      */
    const std::shared_ptr<Agent>&
        add_agent (const DataIO::Config& custom_cfg = {})
    {
        return add_agent(random_pos(), custom_cfg);
    }


    /// Removes the given agent from the agent manager
    void remove_agent (const std::shared_ptr<Agent>& agent) {
        // Find the position in the agents container that belongs to the agent
        const auto it = std::find(_agents.cbegin(), _agents.cend(), agent);

        if (it == _agents.cend()) {
            throw std::invalid_argument("The given agent is not handled by "
                                        "this manager!");
        };

        _log->trace("Removing agent with ID {:d} ...", agent->id());
        _agents.erase(it);
    }

    /// Remove agents if the given condition is met
    /** \details Uses the erase-remove idiom
     *
     *  \tparam UnaryPredicate type of the UnaryPredicate
     *
     *  \param condition The condition under which the agent is to be removed.
     *                   This should be a callable that takes the agent shared
     *                   pointer as argument and returns bool.
     */
    template<typename UnaryPredicate>
    void erase_agent_if (UnaryPredicate&& condition) {
        _agents.erase(
            std::remove_if(_agents.begin(), _agents.end(), condition),
            _agents.cend()
        );
    }


    /// Update the agents
    /** \details This function is needed in the case of synchronous update.
     *          It updates the state and position from the agent's state
     *          and position cache variables.
     *
     * \note In the case of asynchronous agent update this function will throw
     *       an error. There is no need to update these agent traits because
     *       there is no cached trait.
     */
    void update_agents() {
        // Assert that the agents update synchronously
        static_assert(AgentTraits::mode == Update::sync,
            "The update_agents method only makes sense to call when agents "
            "are set to be updated synchronously, which is not the case! "
            "Either adapt the AgentTraits to that update mode or remove the "
            "call to the update_agents method.");

        // Go through all agents and update them
        for (const auto& agent : _agents){
            agent->update();
        }
    }

    // .. Agent Selection .....................................................
    /// Select agents using the \ref Utopia::select_entities interface
    /** Returns a container of agents that were selected according to a certain
      * selection mode. This is done via the \ref Utopia::select_entities
      * interface.
      *
      * \tparam  mode    The selection mode
      *
      * \args    args    Forwarded to \ref Utopia::select_entities
      */
    template<SelectionMode mode, class... Args>
    AgentContainer<Agent> select_agents(Args&&... args) {
        return select_entities<mode>(*this, std::forward<Args>(args)...);
    }

    /// Select entities according to parameters specified in a configuration
    /** Via the ``mode`` key, one of the selection modes can be chosen; for
      * available oens, see \ref Utopia::SelectionMode.
      *
      * Depending on that mode, the other parameters are extracted from the
      * configuration. See \ref Utopia::select_entities for more info.
      *
      * \param  sel_cfg  The configuration node containing the expected
      *                  key-value pairs specifying the selection.
      */
    AgentContainer<Agent> select_agents(const DataIO::Config& sel_cfg) {
        return select_entities(*this, sel_cfg);
    }


private:
    // -- Helper functions ----------------------------------------------------
    /// Returns a valid (uniformly) random position in space
    SpaceVec random_pos() {
        // Create a space vector with random relative positions [0, 1), and
        // calculate the absolute position by multiplying element-wise with the
        // extent of the space
        std::uniform_real_distribution<double> dist(0., 1.);
        return (  this->_space->extent
                % SpaceVec().imbue([this,&dist](){return dist(*this->_rng);}));
    }

    // -- Setup functions -----------------------------------------------------

    /// Set up the agent manager configuration member
    /** \details This function determines whether to use a custom configuration
      *         or the one provided by the model this AgentManager belongs to
      */
    DataIO::Config setup_cfg(Model& model, const DataIO::Config& custom_cfg) {
        DataIO::Config cfg;

        if (custom_cfg.size() > 0) {
            _log->debug("Using custom config for agent manager setup ...");
            cfg = custom_cfg;
        }
        else {
            _log->debug("Using '{}' model's configuration for agent manager "
                        "setup ... ", model.get_name());

            if (not model.get_cfg()["agent_manager"]) {
                throw std::invalid_argument("Missing config entry "
                    "'agent_manager' in model configuration! Either specify "
                    "that key or pass a custom configuration node to the "
                    "AgentManager constructor.");
            }
            cfg = model.get_cfg()["agent_manager"];
        }
        return cfg;
    }


    /// Setup helper used to determine a single agent's initial position
    SpaceVec initial_agent_pos() {
        // By default, use a random initial position
        std::string initial_pos_mode = "random";

        // If given, extract the initial_position mode from the configuration
        if (_cfg["initial_position"]) {
            initial_pos_mode = get_as<std::string>("initial_position", _cfg);
        }

        // Return the agent position depending on the mode
        if (initial_pos_mode == "random") {
            return random_pos();
        }
        else {
            throw std::invalid_argument("AgentManager got an invalid "
                "configuration entry for 'initial_position': '"
                + initial_pos_mode + "'. Valid options are: 'random'");
        }
    }


    /// Set up the agents container with initial states
    /** \details This function creates a container with agents that get an
     *          initial state and are set up with a random position.
     *
     * \param initial_state  The initial state of the agents
     * \param num_agents     The number of agents

     */
    void setup_agents(const AgentState initial_state) {

        // Extract parameters from the configuration
        if (not _cfg["initial_num_agents"]) {
            throw std::invalid_argument("AgentManager is missing the "
                "configuration entry 'initial_num_agents' that specifies the "
                "number of agents to set up!");
        }
        const auto num_agents = get_as<IndexType>("initial_num_agents", _cfg);

        // Construct all the agents with incremented IDs, the initial state
        // and a random position
        for (IndexType i=0; i<num_agents; ++i){
            add_agent(initial_state, initial_agent_pos());
        }

        // Done. Shrink it.
        _agents.shrink_to_fit();
        _log->info("Populated agent container with {:d} agents.",
                   _agents.size());
    }


    /// Set up agents container via config or default constructor
    /** \details If no explicit initial state is given, this setup function is
      *         called.
      *         There are three modes: If the Utopia::AgentTraits are set such
      *         that the default constructor of the agent state is to be used,
      *         that constructor is required and is called for each agent.
      *         Otherwise, the AgentState needs to be constructible via a
      *         `const DataIO::Config&` argument, which gets passed the config
      *         entry `agent_params` from the AgentManager's configuration.
      *         If a constructor with the signature
      *         `(const DataIO::Config&, const std::shared_ptr<RNG>&)` is
      *         supported, that constructor is called instead.
      *
      * \note   If the constructor for the agent state has an RNG available
      *         it is called anew for _each_ agent; otherwise, an initial state
      *         is constructed _once_ and used for all agents.
      */
    void setup_agents() {
        // Distinguish depending on constructor.
        // Is the default constructor to be used?
        if constexpr (AgentTraits::use_default_state_constructor) {
            static_assert(std::is_default_constructible<AgentState>(),
                "AgentTraits were configured to use the default constructor to "
                "create agent states, but the AgentState is not "
                "default-constructible! Either implement such a constructor, "
                "unset the flag in the AgentTraits, or pass an explicit "
                "initial agent state to the AgentManager.");

            _log->info("Setting up agents using default constructor ...");

            // Create the initial state (same for all agents)
            setup_agents(AgentState());
        }

        // Is there a constructor available that allows passing the RNG?
        else if constexpr (std::is_constructible<AgentState,
                                                 const DataIO::Config&,
                                                 const std::shared_ptr<RNG>&
                                                 >())
        {
            _log->info("Setting up agents using config constructor (with RNG) "
                       "...");

            // Extract the configuration parameter
            if (not _cfg["agent_params"]) {
                throw std::invalid_argument("AgentManager is missing the "
                    "configuration entry 'agent_params' to set up the agents' "
                    "initial states!");
            }
            const auto agent_params = _cfg["agent_params"];

            if (not _cfg["initial_num_agents"]){
                throw std::invalid_argument("AgentManager is missing the "
                    "configuration entry 'initial_num_agents' to set up the "
                    "agents!"
                    );
            }
            const auto initial_num_agents = get_as<IndexType>("initial_num_agents", _cfg);

            // Populate the container, creating the agent state anew each time
            for (IndexType i=0; i<initial_num_agents; i++) {
                add_agent(AgentState(agent_params, _rng),
                          initial_agent_pos());
            }

            // Done. Shrink it.
            _agents.shrink_to_fit();
            _log->info("Populated agent container with {:d} agents.",
                       _agents.size());
        }

        // As default, require a Config constructor
        else {
            static_assert(std::is_constructible<AgentState,
                                                const DataIO::Config&>(),
                "AgentManager::AgentState needs to be constructible using "
                "const DataIO::Config& as only argument. Either implement "
                "such a constructor, pass an explicit initial agent state to "
                "the AgentManager, or set the AgentTraits such that a default "
                "constructor is to be used.");

            _log->info("Setting up agents using config constructor ...");

            // Extract the configuration parameter
            if (not _cfg["agent_params"]) {
                throw std::invalid_argument("AgentManager is missing the "
                    "configuration entry 'agent_params' to set up the agents' "
                    "initial states!");
            }

            // Create the initial state (same for all agents)
            return setup_agents(AgentState(_cfg["agent_params"]));
        }
        // This point is never reached.
    }


    /// Depending on periodicity, return the function to move agents in space
    /** \details The function that is used to move an agent to a new position in
     *          space depends on whether the space is periodic or not.
     *          In the case of a periodic space the position is automatically
     *          mapped into space again across the borders. For a nonperiodic
     *          space a position outside of the borders will throw an error.
     */
    MoveFunc setup_move_to_func() const {
        // Need to distinguish by periodicity of the space the agents live in
        if (_space->periodic) {
            // Can simply use the space's mapping function
            return [this](Agent& agent, const SpaceVec& pos){
                agent.set_pos(this->_space->map_into_space(pos));
            };
        }
        else {
            // For nonperiodic space, need to make sure the position is valid
            return [this](Agent& agent, const SpaceVec& pos){
                if (not this->_space->contains(pos)) {
                    throw OutOfSpace(pos, this->_space,
                                     "Could not move agent!");
                }

                // Set the new agent position
                agent.set_pos(pos);
            };
        }
    }

    /// Depending on periodicity, return the function to prepare positions of
    /// agents before they are added
    /** \details The function that is used to prepare a position before an
     *          agent is added (if passed explicitly) depends on whether the
     *          space is periodic or not.
     *          In the case of a periodic space the position is automatically
     *          mapped into space again across the borders.
     *          For a nonperiodic space a position outside of the borders will
     *          throw an error.
     */
    PosFunc setup_prepare_pos_func() const {
        // If periodic, map the position back into space
        if (_space->periodic) {
            return
                [this](const SpaceVec& pos){
                    return this->_space->map_into_space(pos);
                };
        }
        // If non-periodic, check wether the position is valid
        else {
            return
                [this](const SpaceVec& pos) {
                    if (not _space->contains(pos)) {
                        throw OutOfSpace(pos, _space,
                                         "Given position is out of space!");
                    }
                    return pos;
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
