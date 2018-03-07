#include <cassert>
#include <vector>
#include <iostream>

#include <dune/common/exceptions.hh>
#include <dune/common/parallel/mpihelper.hh>
#include <dune/utopia/state.hh>

//#include <dune/utopia/utopia.hh>

/// Instantiate containers, check access and contents
int main(int argc, char **argv)
{
    try{
        Dune::MPIHelper::instance(argc,argv);

        StateContainer<double, false> sc1(0.1);
        assert(!sc1.is_sync());
        auto& state = sc1.state();
        state = 0.2;
        assert(sc1.state() = 0.2);

        std::vector<double> vec({0.1, 0.2});
        StateContainer<std::vector<double>, true> sc2(vec);
        assert(sc2.is_sync());
        auto& new_state = sc2.state_new();
        new_state = std::vector<double>({0.1, 0.3});
        assert(sc2.state() == vec);
        sc2.update();
        assert(sc2.state()[1] == 0.3);

        return 0;
    }
    catch(Dune::Exception c){
        std::cerr << c << std::endl;
        return 1;
    }
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 1;
    }
}