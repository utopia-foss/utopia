#include <cassert>
#include <dune/utopia/base.hh>
#include <dune/utopia/core/setup.hh>
#include <dune/utopia/core/data_vtk.hh>

int main(int argc, char** argv)
{
    try{
        Dune::MPIHelper::instance(argc,argv);

        // create two grids
        auto gmsh_2d = Utopia::Setup::read_gmsh("square.msh",2);

        // gmsh grid
        auto agents = Utopia::Setup::create_agents_on_grid(gmsh_2d,100,0);
        auto m1 = Utopia::Setup::create_manager_agents<false,false>(gmsh_2d, agents);

        auto vtkwriter = Utopia::Output::create_vtk_writer(gmsh_2d._grid,"simplex");
        vtkwriter->add_adaptor(Utopia::Output::vtk_output_agent_count_per_cell(m1));
        vtkwriter->write(0);
        
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
