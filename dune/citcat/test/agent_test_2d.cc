#include <random>
#include <cassert>
#include <dune/citcat/citcat.hh>

#include "agent_test.hh"

int main(int argc, char** argv)
{
    try{
        Dune::MPIHelper::instance(argc,argv);

        constexpr size_t agent_count = 1000;
        constexpr size_t grid_size = 50;
        test_agents_on_grid<2>(agent_count,grid_size);

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