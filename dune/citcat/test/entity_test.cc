#include <cassert>
#include <random>
#include <dune/citcat/citcat.hh>
#include <dune/common/exceptions.hh>

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

/// Choose random states and traits. Verify Entity members before and after update
int main(int argc, char const *argv[])
{
	try{
		using State = int;
		using Traits = double;

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<State> dist_state(std::numeric_limits<State>::min(),std::numeric_limits<State>::max());
		std::uniform_real_distribution<Traits> dist_traits(std::numeric_limits<Traits>::min(),std::numeric_limits<Traits>::max());
		const State state = dist_state(gen);
		const State state_1 = dist_state(gen);
		const Traits traits = dist_traits(gen);
		const Traits traits_1 = dist_traits(gen);
		const int tag = 1;

		Citcat::Entity<State,Traits> e1(state,traits,tag);
		assert_entity_members(e1,state,state,traits,traits,tag);

		e1.new_state() = state_1;
		e1.new_traits() = traits_1;
		assert_entity_members(e1,state,state_1,traits,traits_1,tag);

		e1.update();
		assert_entity_members(e1,state_1,state_1,traits_1,traits_1,tag);

		return 0;
	}
	catch(Dune::Exception c){
		std::cerr << c << std::endl;
		return 1;
	}
	catch(...){
		std::cerr << "Unknown exception thrown!" << std::endl;
		return 2;
	}
}