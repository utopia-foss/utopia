#ifndef NEIGHBORHOODS_HH
#define NEIGHBORHOODS_HH

namespace Citcat {

namespace Neighborhoods {

template<class Manager>
class NextNeighbor
{
private:
	using Cell = typename Manager::Cell;
	using Index = typename Cell::Index;

	static constexpr bool _structured = Manager::is_structured();
	static constexpr bool _periodic = Manager::is_periodic();
	static constexpr int _dim = Manager::Traits::dim;

public:
	/// Return next neighbors for any grid
	static auto neighbors
		(const Manager& mngr, const std::shared_ptr<Cell> root)
	{
		// get references to grid manager members
		const auto& gv = mngr.grid_view();
		const auto& mapper = mngr.mapper();

		// get grid entity of root cell
		const auto root_id = root->index();
		const auto it = std::find_if(
			elements(gv).begin(),
			elements(gv).end(),
			[&mapper,root_id](const auto& x){ return root_id == mapper.index(x); }
		);

		// find adjacent grid entities
		std::vector<Index> neighbor_ids;
		for(auto&& is : intersections(gv,*it)){
			if(is.neighbor()){
				neighbor_ids.push_back(mapper.index(is.outside()));
			}
		}

		// find appropriate cell objects
		std::vector<std::shared_ptr<Cell>> neighbors;
		const auto& cells = mngr._cells;
		for(auto id : neighbor_ids){
			neighbors.push_back(std::shared_ptr<Cell>(cells.at(id)));
		}

		return neighbors;
	}

	/// Return next neighbors for structured grid
	static auto neighbors
		(const Manager& mngr, const std::shared_ptr<Cell> root)
	{
		// find neighbor IDs
		const auto root_id = root->index();
		const auto& grid_cells = mngr.grid_cells();

		std::vector<long long int> neighbor_ids;

		// interate over dimensions
		for(std::size_t i=0; i<grid_cells.size(); ++i){
			long long int disp = 1;
			// iterate over lower dimensions
			for(int j = i-1; j>=0; --j){
				disp *= grid_cells[j];
			}

			auto disp_plus = disp;
			auto disp_minus = disp;

			if (_periodic)
			{

				if(i == 0){
					// left boundary
					if(root_id % grid_cells[0] == 0){
						disp_minus -= shift<1>(grid_cells);
					}
					// right boundary
					else if(root_id % grid_cells[0] == grid_cells[0] - 1){
						disp_plus -= shift<1>(grid_cells);
					}
				}

				else if(i == 1){
					// 'normalize' id to lowest height (if 3D)
					const auto root_id_nrm = root_id % shift<2>(grid_cells);
					// front
					if((int) root_id_nrm / grid_cells[0] == 0){
						disp_minus -= shift<2>(grid_cells);
					}
					else if((int) root_id_nrm / grid_cells[0] == grid_cells[1] - 1){
						disp_plus -= shift<2>(grid_cells);
					}
				}

				else if(i == 2){
					const auto id_max = shift<3>(grid_cells);
					if(root_id + disp_plus >= id_max){
						disp_plus -= id_max;
					}
					else if(root_id - disp_minus < 0){
						disp_minus -= id_max;
					}
				}

			}

			neighbor_ids.push_back(root_id + disp_plus);
			neighbor_ids.push_back(root_id - disp_minus);
		}

		// find appropriate cell objects
		std::vector<std::shared_ptr<Cell>> neighbors;
		const auto& cells = mngr._cells;
		for(auto id : neighbor_ids){
			neighbors.push_back(std::shared_ptr<Cell>(cells.at(id)));
		}

		return neighbors;
	}

private:

	template<std::size_t index, typename T>
	static typename T::value_type shift (const T& cells)
	{
		if (index == 0){
			return 0;
		}
		else if (index == 1){
			return cells[0];
		}
		else if (index == 2){
			return cells[0] * cells[1];
		}
		else if (index == 3){
			return cells[0] * cells[1] * cells[2];
		}
		/*
		else{
			static_assert(false,"This only works for index = {0,1,2,3}.");
		}
		*/
	}

};


template<class Cell>
class CustomNeighborhood
{
private:
	/// container for pointers to neighbors
	std::vector<std::shared_ptr<Cell>> _neighbors;

public:
	/// Constructor. Reserve the neighborhood size.
	CustomNeighborhood (const std::size_t size)
	{
		_neighbors.reserve(size);
	}

	/// Default copy constructor
	CustomNeighborhood (const CustomNeighborhood&) = default;

	/// Default move constructor
	CustomNeighborhood (CustomNeighborhood&&) = default;

	/// Return const reference to neighbor storage
	const std::vector<std::shared_ptr<Cell>>& neighbors () const
	{
		return _neighbors;
	}

	/// Insert a cell into the storage, if it is not yet contained by it.
	/** \return True if cell was inserted
	 */
	bool add_neighbor (const std::shared_ptr<Cell> cell)
	{
		if(std::find(_neighbors.cbegin(),_neighbors.cend(),cell)
			== _neighbors.end()){
			_neighbors.push_back(cell);
			return true;
		}

		return false;
	}

};


} // namespace Neighborhoods
} // namespace Citcat

#endif // NEIGHBORHOODS_HH