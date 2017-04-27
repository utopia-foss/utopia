#ifndef GRID_HH
#define GRID_HH

namespace Citcat {

/// Struct for returning relevant grid data from Setup functions
template<typename GridType>
struct GridWrapper
{
private:
	using Coordinate = typename GridTypeAdaptor<GridType>::Coordinate;
	static constexpr int dim = GridTypeAdaptor<GridType>::dim;
public:
	//! pointer to the grid
	std::shared_ptr<GridType> _grid;
	//! grid extensions in each dimension
	std::array<Coordinate,dim> _extensions;
	//! cells on the grid in each dimension
	std::array<unsigned int,dim> _grid_cells;
};


template<typename GridType, bool structured, bool periodic,
	typename CellType>
class GridManager
{
public:
	using Traits = GridTypeAdaptor<GridType>;
	using Cell = CellType;

private:
	using Grid = GridType;
	using GV = typename Traits::GridView;
	using Mapper = typename Traits::Mapper;
	using Index = typename Traits::Index;
	using Coordinate = typename Traits::Coordinate;
	using Position = typename Traits::Position;
	static constexpr int dim = Traits::dim;

	static constexpr bool _is_structured = structured;
	static constexpr bool _is_periodic = periodic;

	//! pointer to the Dune grid
	std::shared_ptr<Grid> _grid;
	//! cells on the grid in each dimension
	std::array<unsigned int,dim> _grid_cells;
	//! grid extensions in each dimension
	std::array<Coordinate,dim> _extensions;
	//! Dune GridView object for access to grid entities
	const GV _gv;
	//! Dune Mapper for grid entities
	const Mapper _mapper;

	//! container for CA cells
	std::vector<std::shared_ptr<Cell>> _cells;

public:

	explicit GridManager (
		const GridWrapper<GridType>& wrapper,
		const std::vector<std::shared_ptr<Cell>> cells ) :
		_grid(wrapper._grid),
		_grid_cells(wrapper._grid_cells),
		_extensions(wrapper._extensions),
		_gv(_grid->leafGridView()),
		_mapper(_gv),
		_cells(cells)
	{ }

	// queries on static information
	static constexpr bool is_structured () { return _is_structured; }
	static constexpr bool is_periodic () { return _is_periodic; }

	// queries on members
	std::shared_ptr<Grid> grid () const { return _grid; }
	const GV& grid_view () const { return _gv; }
	const Mapper& mapper () const { return _mapper; }

	const std::array<unsigned int,dim>& grid_cells () const { return _grid_cells; }
	const std::array<Coordinate,dim>& extensions () const { return _extensions; }

	const std::vector<std::shared_ptr<Cell>>& cells () const { return _cells; }
/*
	/// Check if coordinates are outside grid
	template<bool active = _is_periodic>
	std::enable_if_t<active,Position> check_inside (const Position& pos)
	{
		return true;
	}

	/// Check if coordinates are outside grid
	template<bool active = _is_periodic>
	std::enable_if_t<!active,Position> check_inside (const Position& pos)
	{
		for(int i = 0; i<dim; ++i){
			if(pos[i] < 0.0 || pos[i] > _extensions[i]){
				return false;
			}
		}

		return true;
	}


	/// Transform position out of the grid into a position inside the grid
	template<bool active = _is_periodic>
	std::enable_if_t<active,Position> transform_position_periodic (const Position& pos)
	{
		Position pos_new;
		for(int i = 0; i<dim; ++i){
			int count = pos[i] / _extensions[i];
			if(count > 0){
				pos_new[i] = pos[i] - count * _extensions[i];
			}
		}

		return pos_new;
	}

	/// Transform position out of the grid into a position inside the grid
	template<bool active = _is_periodic>
	std::enable_if_t<!active,Position> transform_position_periodic (const Position& pos)
	{
		if (!check_inside(pos)){
			DUNE_THROW(Dune::Exception,"This grid is not periodic");
		}
		return pos;
	}

	/// Return the entity index for a given position on the grid
	template<bool active = _is_structured>
	std::enable_if_t<active,Index> entity_id_at (const Position& pos)
	{
		if (!check_inside(pos)){
			DUNE_THROW(Dune::Exception,"This position is not inside the grid");
		}

		std::array<Index,dim> id_matrix;

		for(int i = 0; i<dim; ++i){
			id_matrix[i] = _cells[i] * pos[i] / _extensions[i];
		}

		return id_matrix_to_id(id_matrix)
	}

	/// Return the entity index for a given position on the grid
	template<bool active = _is_structured>
	std::enable_if_t<!active,Index> entity_id_at (const Position& pos)
	{
		for (const auto& e : elements(gv)){
			const auto geo = e.geometry();
			const auto& ref = Dune::ReferenceElements<Coordinate,dim>::general(geo.type());
			const auto pos_local = geo.local(_position);

			if (ref.checkInside(pos_local)){
				return id = mapper.index(e);
			}	
		}

		DUNE_THROW(Dune::Exception,"This position is not inside the grid");
	}

	template<int dim>
	Index id_matrix_to_id (std::array<Index,dim>& mat);

	template<>
	Index id_matrix_to_id (std::array<Index,2>& mat);
	{
		return mat[1] * _extensions[0]
			+ mat[0];
	}

	template<>
	Index id_matrix_to_id (std::array<Index,3>& mat)
	{
		return mat[2] * _extensions[1] * _extensions[0]
			+ mat[1] * _extensions[0]
			+ mat[0];
	}
*/
};

} // namespace Citcat

#endif // GRID_HH