#include <iostream>

#include "PredatorPrey.hh"

using namespace Utopia::Models::PredatorPrey;
using Utopia::Setup::create_grid_manager_cells;

int main (int argc, char** argv)
{   
    
    constexpr unsigned short dim = 2;
    constexpr bool structured = true;
    constexpr bool sync = false;

    try {
        Dune::MPIHelper::instance(argc, argv);
        

        // Initialize the PseudoParent from config file path
        Utopia::PseudoParent pp(argv[1]);

        // Initialize the main model instance with different template arguments
        if (Utopia::as_bool(pp.get_cfg()["PredatorPrey"]["periodic"])) {
            // Periodic grid
            constexpr bool periodic = true;
            
            PredatorPreyModel model("PredatorPrey", pp,
                create_grid_manager_cells<State, periodic, dim, structured, 
                                          sync>("PredatorPrey", pp));

            model.run();

        }
        else {
            // Non-periodic grid
            constexpr bool periodic = false;
            
            PredatorPreyModel model("PredatorPrey", pp,
                create_grid_manager_cells<State, periodic, dim, structured, 
                                          sync>("PredatorPrey", pp));


            model.run();
        }

        return 0;
    }
    catch (Utopia::Exception& e) {
        return Utopia::handle_exception(e);
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "Exception occurred!" << std::endl;
        return 1;
    }
}
