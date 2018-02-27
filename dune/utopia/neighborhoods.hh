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