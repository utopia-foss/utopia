#ifndef ENTITY_HH
#define ENTITY_HH

namespace Citcat
{
	
/// Base class for Cells and Individuals, containing information on State and Traits
/** \tparam StateType Type of states
 *  \tparam TraitsType Type of traits
 */
template<typename StateType, typename TraitsType>
class Entity
{
public:
	using State = StateType;
	using Traits = TraitsType;

	/// Constructor. Define initial state and traits, and constant tag
	Entity(const State& state, const Traits& traits, const int tag=0) :
		_state(state), _state_c(state),
		_traits(traits), _traits_c(traits),
		_tag(tag)
	{ }

	/// Return reference to state cache
	inline State& new_state() { return _state_c; }
	/// Return const reference to state
	inline const State& state() const { return _state; }
	/// Return reference to traits cache
	inline Traits& new_traits() { return _traits_c; }
	/// Return const reference to traits
	inline const Traits& traits() const { return _traits; }
	/// Return tag
	inline int tag() const { return _tag; }

	/// Update using the cache
	void update()
	{
		_state = _state_c;
		_traits = _traits_c;
	}

private:

	State _state; //!< Current State
	State _state_c; //!< State cache
	Traits _traits; //!< Current Traits
	Traits _traits_c; //!< Traits cache
	const int _tag; //!< Fixed tag

};

} // namespace Citcat

#endif // ENTITY_HH
