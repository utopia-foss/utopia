#ifndef NEIGHBORHOODS_HH
#define NEIGHBORHOODS_HH

namespace Citcat {

/// Find appropriate cells for a set of indices
/** \param cont Container of indices
 *  \param mngr Manager instance
 *  \return Container of shared pointers to the cells
 */
template<typename IndexContainer, typename Manager>
auto cells_from_ids (const IndexContainer& cont, const Manager& mngr)
{
	using Cell = typename Manager::Cell;
	std::vector<std::shared_ptr<Cell>> ret;
	ret.reserve(cont.size());
	const auto& cells = mngr._cells;
	for(auto id : cont){
		ret.emplace_back(std::shared_ptr<Cell>(cells.at(id)));
	}
	return ret;
}

namespace Neighborhoods {

template<class Manager>
class NextNeighbor
{
private:
	using Cell = typename Manager::Cell;
	using Index = typename Cell::Index;

	using return_type = typename std::vector<std::shared_ptr<Cell>>;

	static constexpr bool _structured = Manager::is_structured();
	static constexpr bool _periodic = Manager::is_periodic();
	static constexpr int _dim = Manager::Traits::dim;

public:

	/// Return next neighbors for any grid
	template<bool enabled = _structured>
	static std::enable_if_t<!enabled,return_type> neighbors
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

		return cells_from_ids(neighbor_ids,mngr);
	}

	/// Return next neighbors for structured grid
	template<bool enabled = _structured>
	static std::enable_if_t<enabled,return_type> neighbors
		(const Manager& mngr, const std::shared_ptr<Cell> root)
	{
		// find neighbor IDs
		const long long int root_id = root->index();
		const auto& grid_cells = mngr.grid_cells();

		std::vector<long long int> neighbor_ids;

		// 1D shift
		// front boundary
		if(root_id % grid_cells[0] == 0){
			if(_periodic){
				neighbor_ids.push_back(root_id - shift<0>(grid_cells) + shift<1>(grid_cells));
			}
		}
		else{
			neighbor_ids.push_back(root_id - shift<0>(grid_cells));
		}
		// back boundary
		if(root_id % grid_cells[0] == grid_cells[0] - 1){
			if(_periodic){
				neighbor_ids.push_back(root_id + shift<0>(grid_cells) - shift<1>(grid_cells));
			}
		}
		else{
			neighbor_ids.push_back(root_id + shift<0>(grid_cells));
		}

		// 2D shift
		// 'normalize' id to lowest height (if 3D)
		const auto root_id_nrm = root_id % shift<2>(grid_cells);
		// front boundary
		if((long long int) root_id_nrm / grid_cells[0] == 0){
			if(_periodic){
				neighbor_ids.push_back(root_id - shift<1>(grid_cells) + shift<2>(grid_cells));
			}
		}
		else{
			neighbor_ids.push_back(root_id - shift<1>(grid_cells));
		}
		// back boundary
		if((long long int) root_id_nrm / grid_cells[0] == grid_cells[1] - 1){
			if(_periodic){
				neighbor_ids.push_back(root_id + shift<1>(grid_cells) - shift<2>(grid_cells));
			}
		}
		else{
			neighbor_ids.push_back(root_id + shift<1>(grid_cells));
		}

		// 3D shift
		if(_dim == 3)
		{
			const auto id_max = shift<3>(grid_cells) - 1;
			// front boundary
			if(root_id - shift<2>(grid_cells) < 0){
				if(_periodic){
					neighbor_ids.push_back(root_id - shift<2>(grid_cells) + shift<3>(grid_cells));
				}
			}
			else{
				neighbor_ids.push_back(root_id - shift<2>(grid_cells));
			}
			// back boundary
			if(root_id + shift<2>(grid_cells) > id_max){
				if(_periodic){
					neighbor_ids.push_back(root_id + shift<2>(grid_cells) - shift<3>(grid_cells));
				}
			}
			else{
				neighbor_ids.push_back(root_id + shift<2>(grid_cells));
			}
		}

		return cells_from_ids(neighbor_ids,mngr);
	}

private:

	template<std::size_t index, typename T>
	static typename T::value_type shift (const T& cells)
	{
		if (index == 0){
			return 1;
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
	using return_type = typename std::vector<std::shared_ptr<Cell>>;

	/// Return reference to neighbor storage
	template<std::size_t i=0>
	static return_type& neighbors_nc (const std::shared_ptr<Cell> root)
	{
		return std::get<i>(root->neighborhoods());
	}

public:
	/// Return const reference to neighbor storage
	template<std::size_t i=0>
	static const return_type& neighbors (const std::shared_ptr<Cell> root)
	{
		return std::get<i>(root->neighborhoods());
	}

	/// Insert a cell into the neighborhood storage, if it is not yet contained by it.
	/** \param neighbor Cell to be inserted as neighbor
	 *  \param root Cell which receives new neighbor
	 *  \return True if cell was inserted
	 */
	template<std::size_t i=0>
	static bool add_neighbor (const std::shared_ptr<Cell> neighbor,
		const std::shared_ptr<Cell> root)
	{
		auto& nb = neighbors_nc<i>(root);
		if(std::find(nb.cbegin(),nb.cend(),neighbor) == nb.end()){
			nb.push_back(neighbor);
			return true;
		}

		return false;
	}

	/// Remove a cell from the neighborhood storage
	template<std::size_t i=0>
	static void remove_neighbor (const std::shared_ptr<Cell> neighbor,
		const std::shared_ptr<Cell> root)
	{
		auto& nb = neighbors_nc<i>(root);
		const auto it = std::find(nb.cbegin(),nb.cend(),neighbor);
		if(it == nb.end()){
			DUNE_THROW(Dune::Exception,"Trying to erase a neighbor which is not in neighborhood");
		}
		nb.erase(it);
	}

};


} // namespace Neighborhoods
} // namespace Citcat

#endif // NEIGHBORHOODS_HH