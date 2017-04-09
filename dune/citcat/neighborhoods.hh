#ifndef NEIGHBORHOODS_HH
#define NEIGHBORHOODS_HH

namespace Citcat {

namespace Neighborhoods {

template<class Manager>
struct NextNeighbor
{

using Cell = typename Manager::Cell;
using Index = typename Cell::Index;

static constexpr bool _structured = Manager::is_structured();
static constexpr bool _periodic = Manager::is_periodic();

/// Return next neighbors for structured grid
static auto neighbors
	(const Manager& mngr, const std::shared_ptr<Cell> root)
{
	// find neighbor IDs
	const auto root_id = root->index();
	const auto& grid_cells = mngr.grid_cells();

	std::vector<Index> neighbor_ids;
	for(std::size_t i=0; i<grid_cells.size(); ++i){
		Index disp = 1;
		for(std::size_t j=i-1; j>=0; --j){
			disp *= grid_cells[j];
		}
		neighbor_ids.push_back(root_id + 1 * disp);
		neighbor_ids.push_back(root_id - 1 * disp);
	}

	// find appropriate cell objects
	std::vector<std::shared_ptr<Cell>> neighbors;
	const auto& cells = mngr.cells();
	for(auto id : neighbor_ids){
		if(id >= 0 && id < cells.size()){
			neighbors.push_back(std::shared_ptr<Cell>(cells.at(id)));
		}
		else if(_periodic){
			id = id % cells.size();
			neighbors.push_back(std::shared_ptr<Cell>(cells.at(id)));
		}
	}

	return neighbors;
}

};


} // namespace Neighborhoods
} // namespace Citcat

#endif // NEIGHBORHOODS_HH