#include <cassert>
#include <dune/utopia/base.hh>
#include <dune/utopia/setup.hh>
#include <dune/utopia/data_vtk.hh>

int main(int argc, char** argv)
{
    try{
        Dune::MPIHelper::instance(argc,argv);

        // create two grids
        auto rect_2d = Utopia::Setup::create_grid({100,100},{1.0,1.0});

        // structured yasp grid
        auto agents = Utopia::Setup::create_agents_on_grid(rect_2d,100,0);
        auto m1 = Utopia::Setup::create_manager_agents<false,false>(rect_2d, agents);

        auto vtkwriter = Utopia::Output::create_vtk_writer(rect_2d._grid,"rectangular");
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
