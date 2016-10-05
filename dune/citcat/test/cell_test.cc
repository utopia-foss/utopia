#include <random>
#include <dune/citcat/citcat.hh>
#include <dune/common/exceptions.hh>
#include <dune/common/fvector.hh>

#include "entity_test.hh"
#include "cell_test.hh"

/// Choose random states and traits. Verify Entity members before and after update
int main(int argc, char const *argv[])
{
	try{
		using State = int;
		using Traits = double;
		using Position = Dune::FieldVector<double,2>;
		using Index = int;

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<State> dist_int(std::numeric_limits<State>::min(),std::numeric_limits<State>::max());
		std::uniform_real_distribution<Traits> dist_real(std::numeric_limits<Traits>::min(),std::numeric_limits<Traits>::max());
		const State state = dist_int(gen);
		const State state_1 = dist_int(gen);
		const Traits traits = dist_real(gen);
		const Traits traits_1 = dist_real(gen);
		const int tag = 1;
		const Position pos({dist_real(gen),dist_real(gen)});
		const Index index = 2;
		const bool boundary = true;

		Citcat::Cell<State,Traits,Position,Index> c1(state,traits,pos,index,boundary,tag);
		assert_entity_members(c1,state,state,traits,traits,tag);
		assert_cell_members(c1,pos,index,boundary);

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