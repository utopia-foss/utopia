#include <cassert>
#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>

int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc,argv);

        Utopia::DefaultTag t_true;
        assert(!t_true.is_tagged);
        t_true.is_tagged = true;
        assert(t_true.is_tagged);
        return 0;
    }
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 2;
    }
}
