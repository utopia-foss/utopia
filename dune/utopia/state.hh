#ifndef UTOPIA_STATE_HH
#define UTOPIA_STATE_HH

/// Container for states
/** \tparam T Type of state
 *  \tparam sync Boolean if sync or async state is implemented
 */
template<typename T, bool sync>
class StateContainer;

/// State Container specialization for async states
template<typename T>
class StateContainer<T, false>
{
public:
    /// Type of state
    using State = T;

    /// Export implementation type
    static constexpr bool is_structured () { return true; }

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

#endif // UTOPIA_STATE_HH