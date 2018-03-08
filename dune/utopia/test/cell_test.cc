#include <random>
#include <iostream>
#include <cassert>

#include <dune/utopia/utopia.hh>
#include <dune/common/exceptions.hh>
#include <dune/common/fvector.hh>
#include <dune/common/parallel/mpihelper.hh>

#include "cell_test.hh"

/// Choose random states and traits. Verify Entity members before and after update
int main(int argc, char *argv[])
{
    try{
        Dune::MPIHelper::instance(argc, argv);

        //some typedefinitions
        using State = int;
        using Position = Dune::FieldVector<double,2>;
        using Index = int;

        //create random devices
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<State> dist_int(std::numeric_limits<State>::min(),std::numeric_limits<State>::max());
        std::uniform_real_distribution<double> dist_real(std::numeric_limits<double>::min(),std::numeric_limits<double>::max());

        //Create Objects that are handed to the constructor
        const Position pos({dist_real(gen),dist_real(gen)});
        const Index index = dist_int(gen);
        const bool boundary = true;
        const State state(dist_int(gen));
        
        //build a cell
        Utopia::Cell<State,true,Utopia::DefaultTag,Position,Index> c1(state,pos,boundary,index);

        //assert that the content of the cell is correct
        assert(c1.state()==state);
        assert_cell_members(c1, pos, index, boundary);

        //test whether the default flag is set correctly
        //it was created by the default constructor and not set explicitly here
        Utopia::DefaultTag tag;
        assert(c1.is_tagged==tag.is_tagged);
        
        
        // Test async Cell with doubles
        Utopia::Cell<double, false, Utopia::DefaultTag, Position, int> test_cell(0.1,pos,false,0);
        assert(!test_cell.is_sync());
        auto& c_state = test_cell.state();
        c_state = 0.2;
        assert(test_cell.state() = 0.2);
        // Test members inherited from Tag
        assert(!test_cell.is_tagged);
        test_cell.is_tagged = true;
        assert(test_cell.is_tagged);
        // Test entity member variables
        assert(test_cell.id() == 0);

        // Test sync Cell with vector 
        std::vector<double> vec({0.1, 0.2});
        Utopia::Cell<std::vector<double>, true,Utopia::DefaultTag, Position,  int> test_cell2(vec,pos,false, 987654321);
        assert(test_cell2.id() == 987654321);
        assert(test_cell2.is_sync());
        auto& new_state = test_cell2.state_new();
        new_state = std::vector<double>({0.1, 0.3});
        assert(test_cell2.state() == vec);
        test_cell2.update();
        assert(test_cell2.state()[1] == 0.3);


        //test cell neighborhood
        //create a cell with two neighborhoods
        Utopia::Cell<std::vector<double>, true, Utopia::DefaultTag, Position,  int,2 > cell_with_neighbors(vec,pos,false, 987654321);
        
        //look at a reference of the neighborhoods, see they are build, empty
        auto& nb=cell_with_neighbors.neighborhoods();
        assert(nb.size()==2);
        assert(nb[0].size()==0);
        assert(nb[1].size()==0);
        
        //build a neighbor cell and add it to the first neighborhood
        auto neighbor= std::make_shared<Utopia::Cell<std::vector<double>, true, Utopia::DefaultTag, Position, int,2 > >(vec,pos,false, 42);
        nb[0].push_back(neighbor);
        
        //check that it was added, carries correct values 
        assert(nb[0].size()==1);
        assert(nb[0][0]->id()==42);
        assert(nb[0][0]->state()==vec);
        assert(nb[0][0]->position()==pos);
        assert(nb[0][0]->is_boundary()==false);
        
        //alter the state of neighbor
        std::vector<double> vec2({43, 0.9});
        neighbor->state_new()=vec2;
        neighbor->update();
        
        //check the altered state of neighbor via neighborlist
        assert(nb[0][0]->state()==vec2); 
        
        
        //Check neighborhood functions:
        //test reference of neighbor + add function
        auto new_cell_with_neighbors=std::make_shared<Utopia::Cell<std::vector<double>, true, Utopia::DefaultTag, Position, int,2 > >(vec,pos,false, 41);
        auto& nn=Utopia::Neighborhoods::Custom<0>::neighbors(new_cell_with_neighbors);
        assert(nn.size()==0);
        Utopia::Neighborhoods::Custom<0>::add_neighbor(neighbor, new_cell_with_neighbors);
        assert(nn.size()==1);
        assert(nn.back()==neighbor);
        
        
        //test different template parameters
        auto& nnn=Utopia::Neighborhoods::Custom<1>::neighbors(new_cell_with_neighbors);
        assert(nnn.size()==0);
        auto yet_another_neighbor=std::make_shared<Utopia::Cell<std::vector<double>, true, Utopia::DefaultTag, Position, int,2 > >(vec,pos,false, 99);
        Utopia::Neighborhoods::Custom<1>::add_neighbor(yet_another_neighbor, new_cell_with_neighbors);
        Utopia::Neighborhoods::Custom<0>::add_neighbor(yet_another_neighbor, new_cell_with_neighbors);
        assert(nnn.size()==1);
        assert(nn.size()==2);
        assert(nn.back()==nnn.back());
        assert(nn.front()!=nnn.front());
        
        //test remove
        Utopia::Neighborhoods::Custom<0>::remove_neighbor(neighbor, new_cell_with_neighbors);
        assert(nn.size()==1);
        assert(nn.front()==nnn.front());
        Utopia::Neighborhoods::Custom<1>::remove_neighbor(yet_another_neighbor, new_cell_with_neighbors);
        assert(nnn.size()==0);
        assert(nn.size()==1);
        
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