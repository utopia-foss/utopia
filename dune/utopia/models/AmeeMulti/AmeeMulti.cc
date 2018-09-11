#include <iostream>

#include "AmeeMulti.hh"

using namespace Utopia::Models::AmeeMulti;
using Utopia::Setup::create_grid_manager_cells;

int main(int argc, char** argv)
{
    try
    {
        Dune::MPIHelper::instance(argc, argv);

        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent pp(argv[1]);

        return 0;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "Exception occured!" << std::endl;
        return 1;
    }
}
