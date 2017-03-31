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
	using Coordinate = typename Traits::Coordinate;
	using Position = typename Traits::Position;

	std::shared_ptr<Grid> _grid;
	GV _gv;
	Mapper _mapper;

	static constexpr bool _is_structured = structured;
	static constexpr bool _is_periodic = periodic;
	static constexpr int _dim = GridTypeAdaptor::dim;

	Position _extensions;


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
			if(int count > 0){
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

};

} // namespace Citcat

#endif // GRID_HH