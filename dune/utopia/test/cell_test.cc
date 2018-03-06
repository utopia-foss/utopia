#include <random>
#include <dune/utopia/state.hh>
#include <dune/utopia/tags.hh>
#include <dune/utopia/entity.hh>
#include <dune/utopia/cell.hh>
#include <dune/common/exceptions.hh>
#include <dune/common/fvector.hh>
#include <dune/common/parallel/mpihelper.hh>

//#include "entity_test.hh"
#include "cell_test.hh"

#include <iostream>
#include <cassert>

// template<typename T>
class Value
{
public:
    double value;
    const bool operator==(const Value a) const {if(value==a.value) return true;} 
};

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
        const Position pos({dist_real(gen),dist_real(gen)});
        const Index index = 2;
        const bool boundary = true;
        Value A;
        A.value=dist_int(gen);;
        
        Utopia::Cell<Value,true,Position,Utopia::DefaultTag,Index> c1(A,pos,boundary,index);
        Utopia::DefaultTag tag;
        assert(c1.position()==pos);
        assert(c1.is_boundary()==boundary);
        assert(c1.state()==A);
        //Prüfe ob der DefaultTag() gesetzt ist
        assert(c1.is_tagged==tag.is_tagged);
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