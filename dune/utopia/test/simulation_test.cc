#include <dune/utopia/utopia.hh>
#include <cassert>

int main(int argc, char** argv)
{
    try{
        Dune::MPIHelper::instance(argc,argv);

        constexpr unsigned int cell_count = 10;

        auto grid = Utopia::Setup::create_grid(cell_count);
        auto cells = Utopia::Setup::create_cells_on_grid<true>(grid,0);
        // structured, non-periodic manager
        auto manager = Utopia::Setup::create_manager_cells<true,false>(grid,cells);

        // create VTK writer
        auto vtkwriter = Utopia::Output::create_vtk_writer(grid._grid,"sim-test");
        vtkwriter->add_adaptor(Utopia::Output::vtk_output_cell_state(manager.cells()));

        // create simulation
        auto sim = Utopia::Setup::create_sim(manager);
        sim.add_output(vtkwriter);

        sim.add_rule([](const auto cell){
            cell->state();
            return 1;
        });
        sim.add_bc([](const auto cell){
            cell->state();
            return 2;
        });

        sim.run(1);

        for(const auto cell : manager.cells()){
            std::cout << "Cell " << std::to_string(cell->id()) << " State " << std::to_string(cell->state()) << std::endl;
        }

        // check states
        for(const auto cell : manager.cells()){
            if (cell->is_boundary()) {
                if (cell->state() != 2)
                    DUNE_THROW(Dune::Exception,"Boundary Cell State not 2");
            }
            else if (!cell->is_boundary()) {
                if (cell->state() != 1)
                DUNE_THROW(Dune::Exception,"Cell " + std::to_string(cell->id()) + " is not State 1");
            }
        }

        return 0;
    }
    catch(Dune::Exception c){
        std::cerr << c << std::endl;
        return 1;
    }
    catch(...){
        std::cerr << "Unknown exception thrown!" << std::endl;
        return 2;
    }
}
