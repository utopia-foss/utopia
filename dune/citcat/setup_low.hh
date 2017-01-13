#ifndef SETUP_LOW_HH
#define SETUP_LOW_HH

namespace Citcat{
namespace Setup{

/// Low-level utility functions for Setup wrappers
namespace Low{

/// Provide helper functions for apply_periodic_boundaries() wrapper.
template<int dim, typename Cell>
struct PeriodicBoundaryApplicator{

private:

	static bool check_base (const Cell a, const Cell b)
	{
		if(!b->boundary())
			return false;
		if(a==b)
			return false;
		return true;
	}

	static decltype(auto) count_matching_coords (const Cell a, const Cell b)
	{
		const auto pos_a = a->position();
		const auto pos_b = b->position();
		std::bitset<dim> counter;
		for(int i=0; i<dim; ++i){
			if(pos_a[i]==pos_b[i])
				counter.set(i);
		}
		return counter.count();
	}

public:
	
	static bool is_corner_cell (const Cell cell)
	{
		if(dim == 2 && cell->grid_neighbors_count() == 2)
			return true;
		if(dim == 3 && cell->grid_neighbors_count() == 3)
			return true;
		return false;
	}

	
	static bool is_edge_cell (const Cell cell)
	{
		if(dim == 2 && cell->grid_neighbors_count() == 3)
			return true;
		if(dim == 3 && cell->grid_neighbors_count() == 4)
			return true;
		return false;
	}

	
	static bool is_surface_cell (const Cell cell)
	{
		if(dim == 3 && cell->grid_neighbors_count() == 5)
			return true;
		return false;
	}

	
	static bool check_corner_cell (const Cell a, const Cell b)
	{
		if(check_base(a,b) && is_corner_cell(b) &&
			count_matching_coords(a,b) == dim-1){
			return true;
		}
		return false;
	}

	
	static bool check_edge_cell (const Cell a, const Cell b)
	{
		if(check_base(a,b) && is_edge_cell(b) &&
			count_matching_coords(a,b) == dim-1){
			return true;
		}
		return false;
	}

	
	static bool check_surface_cell (const Cell a, const Cell b)
	{
		if(check_base(a,b) && is_surface_cell(b) &&
			count_matching_coords(a,b) == dim-1){
			return true;
		}
		return false;
	}

};


} // namespace Low
} // namespace Setup
} // namespace Citcat

#endif // SETUP_LOW_HH
