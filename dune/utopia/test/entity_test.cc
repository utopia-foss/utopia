#include <dune/utopia/entity.hh>
#include <dune/utopia/state.hh>
#include <dune/utopia/tags.hh>
#include <iostream>

int main(int argc, char **argv)
{
    try{

        StateContainer<double, false> sc1(0.1);
        Utopia::DefaultTag tag(true);
        Utopia::Entity test_entity(sc1, tag, 0);

        return 0;
    }
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 1;
    }
}
