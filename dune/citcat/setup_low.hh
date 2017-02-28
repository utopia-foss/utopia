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

	std::array<double,dim> _extensions;

	bool check_max_distance (const Cell a, const Cell b, const std::bitset<dim> counter)
	{
		for(int i = 0; i<dim; ++i){
			if(!counter.test(i)){
				const auto distance = std::abs(a->position()[i] - b->position()[i]);
				if (std::abs(distance - _extensions[i]) < 1e-3)
					return true;
			}
		}
		return false;
	}

	bool check_base (const Cell a, const Cell b)
	{
		if(!b->boundary())
			return false;
		if(a==b)
			return false;
		return true;
	}

	std::bitset<dim> matching_coords (const Cell a, const Cell b)
	{
		std::bitset<dim> ret;
		const auto pos_a = a->position();
		const auto pos_b = b->position();
		for(int i=0; i<dim; ++i){
			if(pos_a[i]==pos_b[i])
				ret.set(i);
		}
		return ret;
	}

public:

	PeriodicBoundaryApplicator (const std::array<double,dim> extensions) :
		_extensions(extensions)
	{ }
	
	bool is_corner_cell (const Cell cell)
	{
		if(dim == 2 && cell->grid_neighbors_count() == 2)
			return true;
		if(dim == 3 && cell->grid_neighbors_count() == 3)
			return true;
		return false;
	}

	
	bool is_edge_cell (const Cell cell)
	{
		if(dim == 2 && cell->grid_neighbors_count() == 3)
			return true;
		if(dim == 3 && cell->grid_neighbors_count() == 4)
			return true;
		return false;
	}

	
	bool is_surface_cell (const Cell cell)
	{
		if(dim == 3 && cell->grid_neighbors_count() == 5)
			return true;
		return false;
	}

	
	bool check_corner_cell (const Cell a, const Cell b)
	{
		if(check_base(a,b) && is_corner_cell(b) &&
			matching_coords(a,b).count() == dim-1){
			return true;
		}
		return false;
	}

	
	bool check_edge_cell (const Cell a, const Cell b)
	{
		if(check_base(a,b) && is_edge_cell(b)) {
			const auto counter = matching_coords(a,b);
			if(counter.count() == dim-1){
				return check_max_distance(a,b,counter);
			}
		}
		return false;
	}

	
	bool check_surface_cell (const Cell a, const Cell b)
	{
		bool match = false;
		if(check_base(a,b) && is_surface_cell(b)) {
			const auto counter = matching_coords(a,b);
			if(counter.count() == dim-1){
				match = check_max_distance(a,b,counter);
				for(int i = 0; i<dim; ++i){
					// both matching coordinates must not be zero
					if(counter.test(i) && a->position()[i] == 0.0){
						match = false;
					}
				}
			}
		}
		return match;
	}

};


} // namespace Low
} // namespace Setup
} // namespace Citcat

#endif // SETUP_LOW_HH
