#include <cassert>
#include <dune/citcat/citcat.hh>
#include <dune/common/exceptions.hh>

template<typename GridType>
void assert_grid_elements (std::shared_ptr<GridType> grid, const int cells, const int cells_boundary, const int vertices)
{
	int count_cells(0), count_cells_boundary(0), count_vertices(0);
	auto gv = grid->leafGridView();

	// iterate over cells
	for(auto it=gv.template begin<0>(); it!=gv.template end<0>(); ++it)
	{
		++count_cells;
		// iterate over intersections
		for(auto iit=gv.ibegin(*it); iit!=gv.iend(*it); ++iit){
			if(iit->boundary()){
				++count_cells_boundary;
				break;
			}
		}
	}

	// iterate over vertices
	for(auto it=gv.template begin<GridType::dimension>(); it!=gv.template end<GridType::dimension>(); ++it)
	{
		++count_vertices;
	}

	assert(count_cells == cells);
	assert(count_vertices == vertices);
	assert(count_cells_boundary == cells_boundary);
}

int main(int argc, char** argv)
{
	try{
		Dune::MPIHelper::instance(argc,argv);

		auto gmsh_2d = Citcat::Setup::read_gmsh("square.msh");
		assert_grid_elements(gmsh_2d,1042,80,562);

		auto gmsh_3d = Citcat::Setup::read_gmsh<3>("cube.msh");
		assert_grid_elements(gmsh_3d,4461,1372,1117);

		auto rect_2d = Citcat::Setup::create_grid(100);
		assert_grid_elements(rect_2d,1E4,396,10201);

		auto rect_3d = Citcat::Setup::create_grid<3>(100);
		assert_grid_elements(rect_3d,1000000,58808,1030301);

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