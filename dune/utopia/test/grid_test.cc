#include <cassert>
#include <dune/utopia/utopia.hh>
#include <dune/common/exceptions.hh>

template<typename GridType>
void assert_grid_elements (std::shared_ptr<GridType> grid, const int cells, const int cells_boundary, const int verts)
{
    int count_cells(0), count_cells_boundary(0), count_vertices(0);
    auto gv = grid->leafGridView();

    // iterate over cells
    for(const auto& e : elements(gv))
    {
        ++count_cells;
        for(const auto& is : intersections(gv,e)){
            if(is.boundary()){
                ++count_cells_boundary;
                break;
            }
        }
    }

    // count vertices
    auto vert_range = vertices(gv);
    count_vertices = std::distance(vert_range.begin(),vert_range.end());

    assert(count_cells == cells);
    assert(count_vertices == verts);
    assert(count_cells_boundary == cells_boundary);
}

int main(int argc, char** argv)
{
    try{
        Dune::MPIHelper::instance(argc,argv);

        auto gmsh_2d = Citcat::Setup::read_gmsh("square.msh");
        assert_grid_elements(gmsh_2d._grid,1042,80,562);

        auto gmsh_3d = Citcat::Setup::read_gmsh<3>("cube.msh");
        assert_grid_elements(gmsh_3d._grid,4461,1372,1117);

        auto rect_2d = Citcat::Setup::create_grid(100);
        assert_grid_elements(rect_2d._grid,1E4,396,10201);

        auto rect_3d = Citcat::Setup::create_grid<3>(100);
        assert_grid_elements(rect_3d._grid,1000000,58808,1030301);

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