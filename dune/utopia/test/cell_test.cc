#include <random>
#include <dune/utopia/cell.hh>
#include <dune/common/exceptions.hh>
#include <dune/common/fvector.hh>
#include <dune/common/parallel/mpihelper.hh>

#include "entity_test.hh"
#include "cell_test.hh"

#include <iostream>
#include <cassert>

/// Choose random states and traits. Verify Entity members before and after update
int main(int argc, char *argv[])
{
    try{
        Dune::MPIHelper::instance(argc, argv);

        using State = int;
        using Traits = double;
        using Position = Dune::FieldVector<double,2>;
        using Index = int;
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<State> dist_int(std::numeric_limits<State>::min(),std::numeric_limits<State>::max());
        std::uniform_real_distribution<Traits> dist_real(std::numeric_limits<Traits>::min(),std::numeric_limits<Traits>::max());
        const State state = dist_int(gen);
        //const Traits traits = dist_real(gen);
        const int tag = 1;
        const Position pos({dist_real(gen),dist_real(gen)});
        const Index index = 2;
        const bool boundary = true;
        
        Utopia::Cell<State,Position,int,Index> c1(state,pos,boundary,tag,index);
        std::cout<<"I am actually doing this"<<std::endl;
        //assert(c1);
        //assert(c1.nonsense==pos);
        assert(c1.position()==pos);
        assert(c1.is_boundary()==boundary);
        assert(c1._state==state);
        assert(c1._tag==tag);
        assert(c1.id()==index);
        //assert_cell_members(c1,pos,index,boundary);

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