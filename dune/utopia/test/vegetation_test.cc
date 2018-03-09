#include "model_core_test.hh"


int main(int argc, char *argv[])
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        auto vegetation = Utopia::Setup::vegetation(100);

        vegetation.iterate();

        return 0;
    }
    catch(...){
        std::cerr << "Exception thrown!" << std::endl;
        return 1;
    }
}
