#ifndef UTOPIA_STATE_HH
#define UTOPIA_STATE_HH

#include <type_traits>

/// Container for states
/** \tparam StateType Type of state
 *  \tparam sync Boolean if sync or async state is implemented
 */
template<typename StateType, bool sync>
class StateContainer;

/// State Container specialization for async states
template<typename StateType>
class StateContainer<StateType, false>
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
template<typename StateType>
class StateContainer<StateType, true>
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

#endif // UTOPIA_STATE_HH
