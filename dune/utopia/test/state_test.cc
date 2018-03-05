#include <cassert>

#include <dune/common/exceptions.hh>
#include <dune/utopia/utopia.hh>
#include <dune/utopia/state.hh>

/// Instantiate containers, check access and contents
int main(int argc, char **argv)
{
    try{
        Dune::MPIHelper::instance(argc,argv);

        StateContainer<double, false> sc(0.1);
        auto& state = sc.state();
        state = 0.2;

        assert(sc.state() = 0.2);

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