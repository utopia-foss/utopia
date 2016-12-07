#include <random>
#include <dune/citcat/citcat.hh>
#include <dune/common/exceptions.hh>

#include "entity_test.hh"

/// Choose random states and traits. Check member access and update functions.
int main(int argc, char const *argv[])
{
	try{
		using State = int;
		using Traits = double;

		/// get random values for states and traits
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<State> dist_state(std::numeric_limits<State>::min(),std::numeric_limits<State>::max());
		std::uniform_real_distribution<Traits> dist_traits(std::numeric_limits<Traits>::min(),std::numeric_limits<Traits>::max());
		const State state = dist_state(gen);
		const State state_1 = dist_state(gen);
		const State state_2 = dist_state(gen);
		const Traits traits = dist_traits(gen);
		const Traits traits_1 = dist_traits(gen);
		const Traits traits_2 = dist_traits(gen);
		const int tag = 1;

		/// test initialization
		Citcat::Entity<State,Traits> e1(state,traits,tag);
		assert_entity_members(e1,state,state,traits,traits,tag);

		/// test accessing state and traits cache
		e1.new_state() = state_1;
		e1.new_traits() = traits_1;
		assert_entity_members(e1,state,state_1,traits,traits_1,tag);

		/// test general update
		e1.update();
		assert_entity_members(e1,state_1,state_1,traits_1,traits_1,tag);

		/// test separate updates
		e1.new_state() = state_2;
		e1.update_state();
		assert_entity_members(e1,state_2,state_2,traits_1,traits_1,tag);

		e1.new_traits() = traits_2;
		e1.update_traits();
		assert_entity_members(e1,state_2,state_2,traits_2,traits_2,tag);

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