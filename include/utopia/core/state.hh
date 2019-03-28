#ifndef UTOPIA_CORE_STATE_HH
#define UTOPIA_CORE_STATE_HH

namespace Utopia {

/**
 *  \addtogroup Rules
 *  \{
 */

/// Update modes when applying rules
/** It is recommended to use Update::manual in the EntityTraits because
 *  this gives full flexibility when applying the rules.
 *
 *  \note To retain compatiblity with older implementations, this switch
 *        is used **twice**: For specializing the StateContainer, and for
 *        setting the update type in apply_rule() if said specialization
 *        is for Update::manual.
 */
enum class Update {
    manual, //!< User chooses update type when calling apply_rule()
    sync, //!< Synchronous update
    async //!< Asynchronous update
};

/// Container for states
/** \tparam StateType Type of state
 *  \tparam mode The update mode implemented by the state
 *
 *  This class specializes depending on the template parameter #Update.
 *
 *  \see StateContainer<StateType, Update::manual>
 *  \see StateContainer<StateType, Update::sync>
 *  \see StateContainer<StateType, Update::async>
 */
template<typename StateType, Update mode>
class StateContainer;

/// A very simple, default constructible container with public members
template<typename StateType>
class StateContainer<StateType, Update::manual>
{
public:
    using State = StateType;
    State state;

    /// Construct state container with specific state
    /** This is required to support initialization without braces,
     *  as used for the other state containers.
     */
    StateContainer (const State state_initial) :
        state(state_initial)
    { }
};

/// State Container specialization for async states
/**
 *  \warning Using this specialization is discouraged because it determines
 *           the type of update used in apply_rule(). See \ref Rules for
 *           details.
 */
template<typename StateType>
class StateContainer<StateType, Update::async>
{
public:
    /// Type of state
    using State = StateType;

    /// Export implementation type
    static constexpr bool is_sync () { return false; }

    /// Construct state container with specific state
    StateContainer (const State state):
        _state(state)
    { }

    /// Return reference to state
    State& state () { return _state; }
    /// Return const reference to state
    const State& state () const { return _state; } 

private:
    State _state;
};

/// State Container specialization for sync states
/**
 *  \warning Using this specialization is discouraged because it determines
 *           the type of update used in apply_rule(). See \ref Rules for
 *           details.
 */
template<typename StateType>
class StateContainer<StateType, Update::sync>
{
public:
    /// Type of state
    using State = StateType;

    /// Export implementation type
    static constexpr bool is_sync () { return true; }

    /// Construct state container with specific state
    StateContainer (const State state):
        _state(state),
        _state_new(state)
    { }

    /// Return reference to state cache
    State& state_new () { return _state_new; }
    /// Return const reference to state
    const State& state () const { return _state; }
    /// Overwrite state with state cache
    void update () noexcept { _state = _state_new; }

private:
    State _state;
    State _state_new;
};

/**
 *  \} // endgroup Rules
 */

} // namespace Utopia

#endif // UTOPIA_CORE_STATE_HH
