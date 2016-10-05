#include <cassert>

/** \param e Entity to test
 *  \param s State
 *  \param s_n New State
 *  \param t Traits
 *  \param t_n New Traits
 *  \param tag Tag
 */
template<typename Entity, typename State, typename Traits>
void assert_entity_members (Entity& e, const State& s, const State& s_n, const Traits& t, const Traits& t_n, const int tag)
{
	assert(e.state()==s);
	assert(e.new_state()==s_n);
	assert(e.traits()==t);
	assert(e.new_traits()==t_n);
	assert(e.tag()==tag);
}