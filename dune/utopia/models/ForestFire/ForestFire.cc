#include <iostream>

#include "ForestFire.hh"

using namespace Utopia::Models::ForestFire;
using Utopia::Setup::create_grid_manager_cells;

int main (int argc, char** argv)
{
    try {
        Dune::MPIHelper::instance(argc, argv);

        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent pp(argv[1]);

        // Initialize the main model instance with different template arguments
        // and then iterate it ... Need separate cases for this.
        // distinction of periodic / non-periodic grid
        if (Utopia::as_bool(pp.get_cfg()["ForestFire"]["periodic"])) 
        {
            // Periodic grid and async update
            ForestFire model("ForestFire", pp,
                /* template arguments: State, periodicity=true, 
                 * dimensionality=2, structured grid=true, synchronous update=false
                 */ 
                create_grid_manager_cells<State,true,2,true,false>("ForestFire", pp)
            );
            model.run();
        }
        else if (!Utopia::as_bool(pp.get_cfg()["ForestFire"]["periodic"])) 
        {
                /* template arguments: State, periodicity=false, 
                 * dimensionality=2, structured grid=true, synchronous update=false
                 */ 
            ForestFire model("ForestFire", pp,
                create_grid_manager_cells<State,false,2,true,false>("ForestFire", pp)
            );
            model.run();
        }

        return 0;
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Exception occured!" << std::endl;
        return 1;
    }
}
