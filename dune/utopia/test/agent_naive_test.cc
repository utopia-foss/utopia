#include <dune/utopia/utopia.hh>
#include <iostream>
#include <cassert>

int main()
{
    try {
        Utopia::Agent<double, Utopia::DefaultTag, int, int> agent(0.2, 0, 1);
        assert(agent.state() == 0.2);
        assert(agent.id() == 0);
        assert(agent.position() == 1);
        assert(agent.is_tagged == false);
        
        //auto grid = Utopia::Setup::create_grid<2>(1000);
        
        //test of neighborhood
        Utopia::Agent<double, Utopia::DefaultTag, int, int, 2> agent_with_neighbors(0.2, 0, 1);
        
        //look at a reference of the neighborhoods, see they are build, empty
        auto& nb=agent_with_neighbors.neighborhoods();
        assert(nb.size()==2);
        assert(nb[0].size()==0);
        assert(nb[1].size()==0);
        
        //build a neighbor cell and add it to the first neighborhood
        auto neighbor= std::make_shared<Utopia::Agent<double, Utopia::DefaultTag, int, int, 2> >(0.3, 0, 42);
        nb[0].push_back(neighbor);
        
        
        //check that it was added, carries correct values 
        assert(nb[0].size()==1);
        assert(nb[0][0]->position()==42);
        assert(nb[0][0]->id()==0);
        assert(nb[0][0]->state()==0.3);
        
        //alter the state of neighbor
        neighbor->state()=666;
        
        //check the altered state of neighbor via neighborlist
        assert(nb[0][0]->state()==666); 
        
        return 0;
    }
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 2;
    }
}
