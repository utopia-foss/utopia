#include <cassert>
#include <dune/citcat/citcat.hh>
#include <dune/common/exceptions.hh>

int main(int argc, char const *argv[])
{
	try{
		const int state = 0;
		const double traits = 1.2;
		const int tag = 1;
		Citcat::Entity<int,double> e1(state,traits,tag);

		assert(e1.state()==state);
		assert(e1.new_state()==state);
		assert(e1.traits()==traits);
		assert(e1.new_traits()==traits);
		assert(e1.tag()==tag);

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