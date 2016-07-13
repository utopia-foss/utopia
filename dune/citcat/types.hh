#ifndef TYPES_HH
#define TYPES_HH

struct StaticTypes
{
	struct Grid
	{
		using Type = Dune::YaspGrid<2,Dune::EquidistantOffsetCoordinates<double,2>>;
		static const int dim = Type::dimension;
		using DomainField = Type::ctype;
		using Domain = Dune::FieldVector<DomainField,dim>;
		using GridView = Type::Partition<Dune::All_Partition>::LeafGridView;
		//using Mapper = Dune::GlobalUniversalMapper<Type>;
		using Mapper = Dune::SingleCodimSingleGeomTypeMapper<GridView,0>;
		using Index = Mapper::Index;
		using ElementIterator = GridView::template Codim<0>::Iterator;
	};
	using EmptyContainer = std::array<std::shared_ptr<int>,0>;
	using VTKWriter = Dune::VTKSequenceWriter<Grid::GridView>;

};

template<typename CellType>
using CellContainer = std::vector<std::shared_ptr<CellType>>;

/// Neighborhood adaptors
namespace Neighborhood
{
	/// Implement von Neumann (5-)neighborhood for rectangular grid
	struct vonNeumann
	{
		static const int size = 5;

		template<typename CellPtr>
		static void apply (const CellPtr& c)
		{
			for(const auto& nb : c->grid_neighbors())
				c->add_neighbor(nb);
		}
	};

	/// Implement Moore (9-)neighborhood for rectangular grid
	struct Moore
	{
		static const int size = 9;

		template<typename CellPtr>
		static void apply (const CellPtr& c)
		{
			// create list of secondary neighbors
			std::vector<CellPtr> snb_list;
			for(const auto& nb : c->grid_neighbors()){
				// add all grid neighbors
				c->add_neighbor(nb);
				for(const auto& snb : nb->grid_neighbors()){
					if(snb==c) continue;
					snb_list.push_back(snb);
				}
			}
			// add duplicates in this list as neighbors
			for(auto it=snb_list.begin(); it!=snb_list.end(); ++it){
				for(auto iit=it+1; iit!=snb_list.end(); ++iit){
					if(*it==*iit)
						c->add_neighbor(*it);
				}
			}
		}
	};
}

#endif // TYPES_HH
