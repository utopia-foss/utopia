//#include <dune/utopia/utopia.hh>
#include <dune/utopia/tags.hh>
#include <iostream>
#include <cassert>

int main(int args, char** argv)
{
    try {
        Utopia::DefaultTag t_true(true);
        assert(t_true.is_tagged());
        t_true.set_is_tagged(false);
        assert(!t_true.is_tagged());
        return 0;
    }
    //catch(Dune::Exception c){
    //    std::cerr << c << std::endl;
    //    return 1;
    //}
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 2;
    }
}
