#ifndef NEIGHBORHOODS_HH
#define NEIGHBORHOODS_HH

namespace Utopia {

/// Return 0-dimensional shift in grid cells
template<std::size_t index, typename T>
constexpr std::enable_if_t<index==0,typename T::value_type> shift (const T& cells)
{
    return 1;
}

/// Return i-dimensional shift in grid cells
template<std::size_t index, typename T>
constexpr std::enable_if_t<index!=0,typename T::value_type> shift (const T& cells)
{
    return cells[index-1] * shift<index-1>(cells);
}

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
    const auto& cells = mngr.cells();
    for(auto id : cont){
        ret.emplace_back(std::shared_ptr<Cell>(cells.at(id)));
    }
    return ret;
}

/// Fill an index container with neighbors in different directions
/** This function takes an index container and populates it with the indices of
 *  neighboring cells in different dimensions, specified by template
 *  parameter `dim_no`.
 *  This only works on structured grids!
 * 
 *  The algorithm first calculates whether the given root cell index has a
 *  front or back boundary in the chosen dimension. If so, the neighboring cell
 *  is only added if the grid is periodic.
 * 
 * \param root_id Which cell to find the agents of
 * \param neighbor_ids The container to populate with the indices
 * \param mngr The cell manager
 * 
 * \tparam dim_no The dimensions in which to add neighbors
 * \tparam structured To ensure it only works on structured grids
 * 
 * \return void
 */
template<std::size_t dim_no,
         class Manager,
         typename IndexContainer,
         bool structured = Manager::is_structured()>
auto add_neighbors_in_dim (const long root_id,
                           IndexContainer& neighbor_ids,
                           const Manager& mngr)
    -> std::enable_if_t<structured, void>
{
    // Gather the grid information needed
    constexpr bool periodic = Manager::is_periodic();
    const auto& grid_cells = mngr.grid_cells();

    std::cout << "    before (root " << root_id << ", dim " << dim_no << ")" << std::endl << "     ";
    for(auto&& id : neighbor_ids) {std::cout << " " << id;}
    std::cout << std::endl;

    // Distinguish by dimension parameter
    // TODO: make these `if constexpr` when adopting C++17 standard    
    if (dim_no == 1) {
        // check if at front boundary
        if(root_id % grid_cells[0] == 0){
            if(periodic){
                neighbor_ids.push_back(root_id
                                       - shift<0>(grid_cells)
                                       + shift<1>(grid_cells));
            }
        }
        else{
            neighbor_ids.push_back(root_id - shift<0>(grid_cells));
        }

        // check if at back boundary
        if(root_id % grid_cells[0] == grid_cells[0] - 1){
            if(periodic){
                neighbor_ids.push_back(root_id
                                       + shift<0>(grid_cells)
                                       - shift<1>(grid_cells));
            }
        }
        else{
            neighbor_ids.push_back(root_id + shift<0>(grid_cells));
        }
    }


    else if (dim_no == 2) {
        // 'normalize' id to lowest height (if 3D)
        const auto root_id_nrm = root_id % shift<2>(grid_cells);

        // check if at front boundary
        if ((long) root_id_nrm / grid_cells[0] == 0){
            if (periodic) {
                neighbor_ids.push_back(root_id
                                       - shift<1>(grid_cells)
                                       + shift<2>(grid_cells));
            }
        }
        else {
            neighbor_ids.push_back(root_id - shift<1>(grid_cells));
        }

        // check if at back boundary
        if ((long) root_id_nrm / grid_cells[0] == grid_cells[1] - 1) {
            if (periodic) {
                neighbor_ids.push_back(root_id
                                       + shift<1>(grid_cells)
                                       - shift<2>(grid_cells));
            }
        }
        else {
            neighbor_ids.push_back(root_id + shift<1>(grid_cells));
        }
    }


    else if (dim_no == 3) {
        const auto id_max = shift<3>(grid_cells) - 1;

        // check if at front boundary
        if (root_id - shift<2>(grid_cells) < 0){
            if (periodic) {
                neighbor_ids.push_back(root_id
                                       - shift<2>(grid_cells)
                                       + shift<3>(grid_cells));
            }
        }
        else {
            neighbor_ids.push_back(root_id - shift<2>(grid_cells));
        }

        // check if at back boundary
        if (root_id + shift<2>(grid_cells) > id_max) {
            if (periodic) {
                neighbor_ids.push_back(root_id
                                       + shift<2>(grid_cells)
                                       - shift<3>(grid_cells));
            }
        }
        else {
            neighbor_ids.push_back(root_id + shift<2>(grid_cells));
        }
    }


    else {
        DUNE_THROW(Dune::Exception, "Can only look for neighbors in first, second, and third dimension.");
    }


    std::cout << "    after (root " << root_id << ", dim " << dim_no << ")" << std::endl << "     ";
    for(auto&& id : neighbor_ids) {std::cout << " " << id;}
    std::cout << std::endl;
}



namespace Neighborhoods {

template<typename CellType>
struct NBTraits
{
    using Cell = CellType;
    using Index = typename Cell::Index;
    using return_type = typename std::vector<std::shared_ptr<Cell>>;
};


class NextNeighbor
{

public:

    /// Return next neighbors for any grid
    template<class Manager, class Cell,
        bool structured = Manager::is_structured()>
    static auto neighbors (const std::shared_ptr<Cell> root, const Manager& mngr)
        -> std::enable_if_t<!structured,typename NBTraits<Cell>::return_type>
    {
        // get references to grid manager members
        using Index = typename NBTraits<Cell>::Index;
        const auto& gv = mngr.grid_view();
        const auto& mapper = mngr.mapper();

        // get grid entity of root cell
        const auto root_id = root->index();
        auto it = elements(gv).begin();
        std::advance(it,root_id);

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
    template<class Manager, class Cell,
        bool structured = Manager::is_structured()>
    static auto neighbors (const std::shared_ptr<Cell> root, const Manager& mngr)
        -> std::enable_if_t<structured,typename NBTraits<Cell>::return_type>
    {
        constexpr bool periodic = Manager::is_periodic();

        // find neighbor IDs
        const long root_id = root->index();
        const auto& grid_cells = mngr.grid_cells();

        std::vector<long> neighbor_ids;

        // 1D shift
        // front boundary
        if(root_id % grid_cells[0] == 0){
            if(periodic){
                neighbor_ids.push_back(root_id - shift<0>(grid_cells) + shift<1>(grid_cells));
            }
        }
        else{
            neighbor_ids.push_back(root_id - shift<0>(grid_cells));
        }
        // back boundary
        if(root_id % grid_cells[0] == grid_cells[0] - 1){
            if(periodic){
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
        if((long) root_id_nrm / grid_cells[0] == 0){
            if(periodic){
                neighbor_ids.push_back(root_id - shift<1>(grid_cells) + shift<2>(grid_cells));
            }
        }
        else{
            neighbor_ids.push_back(root_id - shift<1>(grid_cells));
        }
        // back boundary
        if((long) root_id_nrm / grid_cells[0] == grid_cells[1] - 1){
            if(periodic){
                neighbor_ids.push_back(root_id + shift<1>(grid_cells) - shift<2>(grid_cells));
            }
        }
        else{
            neighbor_ids.push_back(root_id + shift<1>(grid_cells));
        }

        // 3D shift
        if(Manager::Traits::dim == 3)
        {
            const auto id_max = shift<3>(grid_cells) - 1;
            // front boundary
            if(root_id - shift<2>(grid_cells) < 0){
                if(periodic){
                    neighbor_ids.push_back(root_id - shift<2>(grid_cells) + shift<3>(grid_cells));
                }
            }
            else{
                neighbor_ids.push_back(root_id - shift<2>(grid_cells));
            }
            // back boundary
            if(root_id + shift<2>(grid_cells) > id_max){
                if(periodic){
                    neighbor_ids.push_back(root_id + shift<2>(grid_cells) - shift<3>(grid_cells));
                }
            }
            else{
                neighbor_ids.push_back(root_id + shift<2>(grid_cells));
            }
        }

        return cells_from_ids(neighbor_ids,mngr);
    }

};

/// New, faster neighborhood function
/** This class utilizes the add_neighbors_in_dim function to generalize the
 *  finding of neighbors.
 */
// TODO after testing: make this `NextNeighbor` and adapt documentation
class NextNeighborNew
{

public:

    /// Return next neighbors for any grid
    template<class Manager, class Cell,
        bool structured = Manager::is_structured()>
    static auto neighbors (const std::shared_ptr<Cell> root, const Manager& mngr)
        -> std::enable_if_t<!structured, typename NBTraits<Cell>::return_type>
    {
        // get references to grid manager members
        using Index = typename NBTraits<Cell>::Index;
        const auto& gv = mngr.grid_view();
        const auto& mapper = mngr.mapper();

        // get grid entity of root cell
        const auto root_id = root->index();
        auto it = elements(gv).begin();
        std::advance(it,root_id);

        // find adjacent grid entities
        std::vector<Index> neighbor_ids;
        for(auto&& is : intersections(gv,*it)){
            if(is.neighbor()){
                neighbor_ids.push_back(mapper.index(is.outside()));
            }
        }

        return cells_from_ids(neighbor_ids, mngr);
    }

    /// Return next neighbors for structured grid
    template<class Manager, class Cell,
        bool structured = Manager::is_structured()>
    static auto neighbors (const std::shared_ptr<Cell> root, const Manager& mngr)
        -> std::enable_if_t<structured, typename NBTraits<Cell>::return_type>
    {
        // Generate vector in which to store the neighbors
        std::vector<long> neighbor_ids;
        neighbor_ids.reserve(2 * Manager::Traits::dim);
        // Brings about 2x speed improvement
        // valid only for square grids (implied by the grid being structured)

        // Work with the id to reduce lookups
        const long root_id = root->index();

        // Add neighbors in first two dimensions (assuming there are at least 2)
        add_neighbors_in_dim<1>(root_id, neighbor_ids, mngr);
        add_neighbors_in_dim<2>(root_id, neighbor_ids, mngr);

        // And third ...
        if (Manager::Traits::dim >= 3) {
            add_neighbors_in_dim<3>(root_id, neighbor_ids, mngr);

            // Neigbors in even higher dimensions __could__ be added here
        }

        return cells_from_ids(neighbor_ids, mngr);
    }

};


/// Moore neighborhood on structured 2D and 3D lattices and unstructured grids
/** Classicaly, this is only defined on a 2D square lattice and only for
 *  structured grids. The implementation here adds a 3D version, by using the
 *  three-dimensional cube of side length 3 around the desired cell as the
 *  neighborhood.
 * 
 *  The algorithm for the structured grids works basically by finding neighbors
 *  in one dimension and then, in turn, adding the neighbors neighbors _in
 *  another dimension_.
 */
class MooreNeighbor
{

public:
    /// Return Moore neighbors for structured and periodic 2D grid
    template<class Manager, class Cell,
             bool structured = Manager::is_structured(),
             bool periodic = Manager::is_periodic()>
    static auto neighbors (const std::shared_ptr<Cell> root,
                           const Manager& mngr)
        -> std::enable_if_t<structured
                           && periodic
                           && (Manager::Traits::dim == 2),
                           typename NBTraits<Cell>::return_type>
    {
        // Generate vector in which to store the neighbors
        std::vector<long> neighbor_ids;
        neighbor_ids.reserve(8); // is known and fixed for 2D lattice

        // Get the ID of the root cell here; faster than doing multiple lookups
        const long root_id = root->index();

        // Get the neighbors in the second dimension
        add_neighbors_in_dim<2>(root_id, neighbor_ids, mngr);
        // ...have these neighbors at indices 0 and 1 now.

        // For these neighbors and the root, add neighbors in the first dimension
        add_neighbors_in_dim<1>(root_id,         neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[0], neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[1], neighbor_ids, mngr);

        return cells_from_ids(neighbor_ids, mngr);
    }

    /// Return Moore neighbors for structured and non-periodic 2D grid
    template<class Manager, class Cell,
             bool structured = Manager::is_structured(),
             bool periodic = Manager::is_periodic()>
    static auto neighbors (const std::shared_ptr<Cell> root,
                           const Manager& mngr)
        -> std::enable_if_t<structured
                           && !periodic
                           && (Manager::Traits::dim == 2),
                           typename NBTraits<Cell>::return_type>
    {
        // Generate vector in which to store the neighbors
        std::vector<long> neighbor_ids;
        neighbor_ids.reserve(8); // fewer at boundary of course

        // Get the ID of the root cell here; faster than doing multiple lookups
        const long root_id = root->index();

        // Get the neighbors in the second dimension
        add_neighbors_in_dim<2>(root_id, neighbor_ids, mngr);
        // if root not at border: these neighbors are at indices 0 and 1 now
        // if root at border: only one neighbor was added, index 0

        // Before adding the neighbors' neighbors, need to check if the root was at a boundary
        if (neighbor_ids.size() == 2) {
            // Was not at a boundary
            add_neighbors_in_dim<1>(neighbor_ids[0], neighbor_ids, mngr);
            add_neighbors_in_dim<1>(neighbor_ids[1], neighbor_ids, mngr);
        }
        else if (neighbor_ids.size() == 1) {
            // Was at a front OR back boundary in dimension 2 -> only one neighbor available
            add_neighbors_in_dim<1>(neighbor_ids[0], neighbor_ids, mngr);
        }
        // else: was at front AND back boundary (single row of cells in dim 2)

        // Finally, add the root's neighbors in the first dimension
        add_neighbors_in_dim<1>(root_id, neighbor_ids, mngr);

        return cells_from_ids(neighbor_ids, mngr);
    }

    /// Return Moore neighbors for structured and periodic 3D grid
    template<class Manager, class Cell,
             bool structured = Manager::is_structured(),
             bool periodic = Manager::is_periodic()>
    static auto neighbors (const std::shared_ptr<Cell> root,
                           const Manager& mngr)
        -> std::enable_if_t<structured
                            && periodic
                            && (Manager::Traits::dim == 3),
                            typename NBTraits<Cell>::return_type>
    {
        // Generate vector in which to store the neighbors
        std::vector<long> neighbor_ids;
        neighbor_ids.reserve(26); // is known and fixed for 3D square grids

        // Use the ID of the root cell; faster than doing multiple lookups
        const long root_id = root->index();

        // Get the neighbors in the third dimension
        add_neighbors_in_dim<3>(root_id, neighbor_ids, mngr);
        // ...have them at indices 0 and 1 now.

        // For these neighbors and the root, add their neighbors in the 2nd dimension
        add_neighbors_in_dim<2>(root_id,         neighbor_ids, mngr);
        add_neighbors_in_dim<2>(neighbor_ids[0], neighbor_ids, mngr);
        add_neighbors_in_dim<2>(neighbor_ids[1], neighbor_ids, mngr);
        // ...have them at indices 2, 3, 4, 5, 6, 7 now.

        // And finally, add all neighbors in the first dimension
        add_neighbors_in_dim<1>(root_id,         neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[0], neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[1], neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[2], neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[3], neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[4], neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[5], neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[6], neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[7], neighbor_ids, mngr);

        return cells_from_ids(neighbor_ids, mngr);
    }

    /// Return Moore neighbors for structured and non-periodic 3D grid
    // FIXME
    template<class Manager, class Cell,
             bool structured = Manager::is_structured(),
             bool periodic = Manager::is_periodic()>
    static auto neighbors (const std::shared_ptr<Cell> root,
                           const Manager& mngr)
        -> std::enable_if_t<structured
                            && !periodic
                            && (Manager::Traits::dim == 3),
                            typename NBTraits<Cell>::return_type>
    {
        // Generate vector in which to store the neighbors
        std::vector<long> neighbor_ids;
        neighbor_ids.reserve(26); // is known and fixed for 3D square grids

        // Use the ID of the root cell; faster than doing multiple lookups
        const long root_id = root->index();

        // Get the neighbors in the third dimension
        add_neighbors_in_dim<3>(root_id, neighbor_ids, mngr);
        // ...have them at indices 0 and 1 now.

        // For these neighbors and the root, add their neighbors in the 2nd dimension
        add_neighbors_in_dim<2>(root_id,         neighbor_ids, mngr);
        add_neighbors_in_dim<2>(neighbor_ids[0], neighbor_ids, mngr);
        add_neighbors_in_dim<2>(neighbor_ids[1], neighbor_ids, mngr);
        // ...have them at indices 2, 3, 4, 5, 6, 7 now.

        // And finally, add all neighbors in the first dimension
        add_neighbors_in_dim<1>(root_id,         neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[0], neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[1], neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[2], neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[3], neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[4], neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[5], neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[6], neighbor_ids, mngr);
        add_neighbors_in_dim<1>(neighbor_ids[7], neighbor_ids, mngr);

        return cells_from_ids(neighbor_ids, mngr);
    }

    /// Return Moore neighbors for unstructured grid
    template<class Manager, class Cell,
             bool structured = Manager::is_structured(),
             bool periodic = Manager::is_structured()>
    static auto neighbors (const std::shared_ptr<Cell> root,
        const Manager& mngr)
        -> std::enable_if_t<!structured, typename NBTraits<Cell>::return_type>
    {
        // get regular neighbors first
        const auto next_neighbors = NextNeighborNew::neighbors(root, mngr);

        // get their neighbors, i.e. the next next neighbors
        typename NBTraits<Cell>::return_type ret;
        for (auto&& nb : next_neighbors) {
            auto nn_neighbors = NextNeighborNew::neighbors(nb, mngr);
            std::move(nn_neighbors.begin(), nn_neighbors.end(),
                std::back_inserter(ret));
        }

        // remove root
        ret.erase(std::remove_if(ret.begin(), ret.end(),
                [&root](const auto cell){ return cell == root; }),
            ret.end());

        // only keep duplicates -- those are the ones that are part of the Moore neighborhood
        ret.erase(std::remove_if(ret.begin(), ret.end(),
                [&ret](const auto cell){
                    return std::count(ret.begin(), ret.end(), cell) == 1;}),
            ret.end());

        // now only have the duplicates left; but keep only one of each
        std::sort(ret.begin(), ret.end()); // needed for std::unique
        ret.erase(std::unique(ret.begin(), ret.end()), ret.end());

        // add regular neighbors to container
        std::move(next_neighbors.begin(), next_neighbors.end(),
                  std::back_inserter(ret));

        return ret;
    }

};


template<std::size_t i=0>
class Custom
{
private:

    /// Return reference to neighbor storage
    template<class Cell>
    static auto neighbors_nc (const std::shared_ptr<Cell> root)
        -> typename NBTraits<Cell>::return_type&
    {
        return std::get<i>(root->neighborhoods());
    }

public:
    /// Return const reference to neighbor storage
    template<class Cell>
    static auto neighbors (const std::shared_ptr<Cell> root)
        -> const typename NBTraits<Cell>::return_type&
    {
        return std::get<i>(root->neighborhoods());
    }

    /// Insert a cell into the neighborhood storage, if it is not yet contained by it.
    /** \param neighbor Cell to be inserted as neighbor
     *  \param root Cell which receives new neighbor
     *  \return True if cell was inserted
     */
    template<class Cell>
    static bool add_neighbor (const std::shared_ptr<Cell> neighbor,
        const std::shared_ptr<Cell> root)
    {
        auto& nb = neighbors_nc(root);
        if(std::find(nb.cbegin(),nb.cend(),neighbor) == nb.end()){
            nb.push_back(neighbor);
            return true;
        }

        return false;
    }

    /// Remove a cell from the neighborhood storage
    template<class Cell>
    static void remove_neighbor (const std::shared_ptr<Cell> neighbor,
        const std::shared_ptr<Cell> root)
    {
        auto& nb = neighbors_nc(root);
        const auto it = std::find(nb.cbegin(),nb.cend(),neighbor);
        if(it == nb.end()){
            DUNE_THROW(Dune::Exception,"Trying to erase a neighbor which is not in neighborhood");
        }
        nb.erase(it);
    }

};


} // namespace Neighborhoods
} // namespace Utopia

#endif // NEIGHBORHOODS_HH