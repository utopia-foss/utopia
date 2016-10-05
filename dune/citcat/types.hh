#ifndef TYPES_HH
#define TYPES_HH

namespace Citcat
{
	
/// Type of default grid: Rectangular, lower left cell center has coordinates (0,0)
using DefaultGrid = Dune::YaspGrid<2,Dune::EquidistantOffsetCoordinates<double,2>>;

/// Extrct data types dependent on the grid data type
/** \tparam GridType Type of the grid
 */
template<typename GridType>
struct GridTypeAdaptor
{
	//! spatial dimensions of the grid
	static constexpr int dim = GridType::dimension;
	//! Coordinate type
	using Coordinate = typename GridType::ctype;
	//! Position vector
	using Position = typename Dune::FieldVector<Coordinate,dim>;
	//! Type of GridView implemented by Dune
	using GridView = typename GridType::LeafGridView;
	//! Type of VTKSequenceWriter
	using VTKWriter = typename Dune::VTKSequenceWriter<GridView>;
	//! Type of Grid Index Mapper
	using Mapper = typename Dune::MultipleCodimMultipleGeomTypeMapper<GridView,Dune::MCMGElementLayout>;
	//! Type of grid index
	using Index = typename Mapper::Index;
};

/// Type of the variably sized container for cells
template<typename CellType>
using CellContainer = std::vector<std::shared_ptr<CellType>>;
/// Container dummy if no cells or individuals are used
using EmptyContainer = std::array<std::shared_ptr<int>,0>;

/// Neighborhood adaptors
namespace Neighborhood
{
	/// Implement von Neumann (5-)neighborhood for rectangular grid
	struct vonNeumann
	{
		/// Number of cells in the neighborhood
		static const int size = 5;

		/// Apply the neighborhood on a single cell based on its grid neighbors
		/** \param c Cell to apply to
		 */
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
		/// Number of cells in the neighborhood
		static const int size = 9;

		/// Apply the neighborhood on a single cell based on its grid neighbors
		/** \param c Cell to apply to
		 */
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

} // namespace Neighborhood

} // namespace Citcat

#endif // TYPES_HH
