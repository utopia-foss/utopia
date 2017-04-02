#ifndef GRID_HH
#define GRID_HH

namespace Citcat {

template<typename GridType, bool structured, bool periodic>
class GridManager
{
public:
	using Traits = GridTypeAdaptor<GridType>;

private:
	using Grid = GridType;
	using GV = typename Traits::GridView;
	using Mapper = typename Traits::Mapper;
	using Index = typename Traits::Index;
	using Coordinate = typename Traits::Coordinate;
	using Position = typename Traits::Position;

	std::shared_ptr<Grid> _grid;
	GV _gv;
	Mapper _mapper;

	static constexpr bool _is_structured = structured;
	static constexpr bool _is_periodic = periodic;
	static constexpr int _dim = GridTypeAdaptor::dim;

	//! grid extensions in each dimension
	std::array<Coordinate,dim> _extensions;
	//! cells on the grid in each dimension
	std::array<unsigned int, dim> _cells;

public:
	GridManager (const std::shared_ptr<Grid> grid):
		_grid(grid),
		_gv(_grid->leafGridView()),
		_mapper(_gv)
	{
		std::fill(_extensions.begin(),_extensions.end(),0.0);
		for(const auto& v : vertices(_gv)){
			const auto pos = v.geometry().center();
			for(int i = 0; i<dim; ++i){
				_extensions.at(i) = std::max(pos[i],_extensions.at(i));
			}
		}
	}

	const std::array<Coordinate,dim>& extensions () { return _extensions; }
	static constexpr bool is_structured () { return _is_structured; }
	static constexpr bool is_periodic () { return _is_periodic; }

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

};

} // namespace Citcat

#endif // GRID_HH