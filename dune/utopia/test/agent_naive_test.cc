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
        return 0;
    }
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 2;
    }
}
