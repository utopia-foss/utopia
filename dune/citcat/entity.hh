#ifndef ENTITY_HH
#define ENTITY_HH

/// Base class for Cells and Individuals, containing information on State and Traits
/** \tparam State Type of states
 *  \tparam Traits Type of traits
 */
template<typename State, typename Traits>
class Entity
{
public:

	/// Constructor. Define initial state and traits, and constant tag
	Entity(const State& state_, const Traits& traits_, const int tag_=0) :
		s(state_), s_(state_),
		t(traits_), t_(traits_),
		i(tag_)
	{ }

	/// Return reference to state cache
	State& new_state() { return s_; }
	/// Return const reference to state
	const State& state() const { return s; }
	/// Return reference to traits cache
	Traits& new_traits() { return t_; }
	/// Return const reference to traits
	const Traits& traits() const { return t; }
	/// Return tag
	int tag() const { return i; }

	/// Update using the cache
	void update()	{ s=s_; t=t_; }

private:

	State s; //!< Current State
	State s_; //!< State cache
	Traits t; //!< Current Traits
	Traits t_; //!< Traits cache
	const int i; //!< Fixed tag

};

#endif // ENTITY_HH
