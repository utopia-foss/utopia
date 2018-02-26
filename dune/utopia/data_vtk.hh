#ifndef DATA_VTK_HH
#define DATA_VTK_HH

namespace Citcat
{

/// Interface for wrapping data to be written by a VTKWrapper.
/** In order to stack an adaptor to the VTKWrapper, it must inherit
 *  from this abstract class
 */
class GridDataAdaptor
{
public:
    /// Default constructor
    GridDataAdaptor () = default;

    /// Update the local data before printout.
    virtual void update_data () = 0;
};

/// Manages the Dune::VTKWriter and saves instances of GridDataAdaptors
/** This class does not manage the data but only the data adaptors and the
 *  actual VTK writer.
 */
template<typename GridType>
class VTKWrapper : public DataWriter
{
protected:
    /// extract types from the type adaptor
    using GridTypes = GridTypeAdaptor<GridType>;
    using GV = typename GridTypes::GridView;
    using VTKWriter = typename GridTypes::VTKWriter;

    GV _gv; //!< Grid view
    VTKWriter _vtkwriter; //!< Dune::VTKWriter
    std::vector<std::shared_ptr<GridDataAdaptor>> _adaptors; //!< Data adaptors

public:
    /// Create a grid view and a VTK writer
    /** \param grid Shared pointer to the grid
     *  \param filename Output filename
     */
    VTKWrapper (const std::shared_ptr<GridType> grid,
        const std::string& filename) :
        _gv(grid->leafGridView()),
        _vtkwriter(_gv,filename,OUTPUTDIR,"")
    { }

    /// Add a data adaptor to the output of this wrapper
    /** \param adpt GridDataAdaptor to be added
     *  \throw CompilerError The adaptor has to be derived from GridDataAdaptor
     */
    template<typename DerivedGridDataAdaptor>
    void add_adaptor (std::shared_ptr<DerivedGridDataAdaptor> adpt)
    {
        static_assert(std::is_base_of<GridDataAdaptor,DerivedGridDataAdaptor>::value,
            "Object for writing VTK data must be derived from GridDataAdaptor!");
        adpt->add_data(_vtkwriter);
        _adaptors.push_back(adpt);
    }

    /// Update the data managed by the adaptors and call write on the VTKWriter
    void write (const float time)
    {
        for(auto&& i : _adaptors){
            i->update_data();
        }
        _vtkwriter.write(time);
    }

};


/// Write the state of all entities on a grid
template<typename CellContainer>
class CellStateGridDataAdaptor : public GridDataAdaptor
{
protected:
    using Cell = typename CellContainer::value_type::element_type;
    using State = typename Cell::State;

    const CellContainer& _cells; //!< Container of entities
    std::vector<State> _grid_data; //!< Container for VTK readout
    const std::string _label; //!< data label

public:
    /// Constructor
    /** \param cells Container of cells
     *  \param label Data label in VTK output
     */
    CellStateGridDataAdaptor (const CellContainer& cells, const std::string label) :
        _cells(cells), _grid_data(_cells.size()), _label(label)
    { }

    /// Add the data of this adaptor to the VTKWriter
    /** \param vtkwriter Dune VTKWriter managed by the VTKWrapper
     */
    template<typename VTKWriter>
    void add_data (VTKWriter& vtkwriter)
    {
        vtkwriter.addCellData(_grid_data,_label);
    }

    /// Update the data managed by this adaptor
    void update_data ()
    {
        for(const auto& cell : _cells){
            _grid_data[cell->index()] = cell->state();
        }
    }
};

/// Write data defined by a function object for every cell of the grid
/** \tparam CellContainer Type of the container of cells
 *  \tparam Func Type of the function object
 */
template<typename CellContainer, typename Func>
class FunctionalGridDataAdaptor : public GridDataAdaptor
{
protected:
    using Cell = typename CellContainer::value_type;
    using Result = typename std::result_of_t<Func(Cell)>;

    const CellContainer& _cells; //!< Container of entities
    std::vector<Result> _grid_data; //!< Container for VTK readout
    const std::string _label; //!< data label
    //! function returning the data to print
    std::function<Result(Cell)> _function;

public:
    /// Constructor
    /** \param cells Container of cells
     *  \param function Function object returing the data for each cell
     *  \param label Data label in VTK output
     */
    FunctionalGridDataAdaptor (const CellContainer& cells, Func function, const std::string label) :
        _cells(cells), _grid_data(_cells.size()), _label(label), _function(function)
    { }

    /// Add the data of this adaptor to the VTKWriter
    /** \param vtkwriter Dune VTKWriter managed by the VTKWrapper
     */
    template<typename VTKWriter>
    void add_data (VTKWriter& vtkwriter)
    {
        vtkwriter.addCellData(_grid_data,_label);
    }

    /// Update the data managed by this adaptor
    void update_data ()
    {
        for(auto cell : _cells){
            _grid_data[cell->index()] = _function(cell);
        }
    }
};


template<typename CellContainer>
class CellStateClusterGridDataAdaptor : public GridDataAdaptor
{
protected:
    using Cell = typename CellContainer::value_type::element_type;
    using State = typename Cell::State;

    const CellContainer& _cells; //!< Container of entities
    std::vector<int> _grid_data; //!< Container for VTK readout
    const std::string _label; //!< data label
    const std::array<State,2> _range; //! range of states to use

public:
    /// Constructor
    /** \param cells Container of cells
     *  \param label Data label in VTK output
     *  \param range Range of states to plot
     */
    CellStateClusterGridDataAdaptor (const CellContainer& cells, const std::string label, std::array<State,2> range) :
        _cells(cells), _grid_data(_cells.size()), _label(label), _range(range)
    { }

    template<typename VTKWriter>
    void add_data (VTKWriter& vtkwriter)
    {
        vtkwriter.addCellData(_grid_data,_label);
    }

    void update_data ()
    {
        std::minstd_rand gen(1);
        std::uniform_int_distribution<int> dist(1,50000);

        std::vector<bool> visited(_cells.size(),false);
        auto cluster_id = dist(gen);
        for(const auto& cell : _cells){
            if(!visited[cell->index()] && range_check(cell)){
                _grid_data[cell->index()] = cluster_id;
                visited[cell->index()] = true;
                neighbor_clustering(cell,visited,cluster_id);
                cluster_id++;
            }
        }
    }

private:
    template<typename Cell>
    void neighbor_clustering (const Cell& cell, std::vector<bool>& visited, const int cluster_id)
    {
        for(const auto& nb : cell->neighbors()){
            if(nb->state()==cell->state() && !visited[nb->index()])
            {
                _grid_data[nb->index()] = cluster_id;
                visited[nb->index()] = true;
                neighbor_clustering(nb,visited,cluster_id);
            }
        }
    }

    template<typename Cell>
    bool range_check (const Cell& cell)
    {
        bool ret = true;
        if(cell->state() < _range[0]) ret = false;
        if(cell->state() > _range[1]) ret = false;
        return ret;
    }
};


template<class Manager>
class AgentCountGridDataAdaptor : public GridDataAdaptor
{
private:
    const Manager& _manager; //!< manager instance
    std::vector<unsigned int> _grid_data; //!< data to write
    const std::string _label; //!< data label

public:
    /// Constructor
    /** \param manager Manager containing grid, agents, cells
     *  \param label Data label in VTK output
     */
    AgentCountGridDataAdaptor (const Manager& manager, const std::string label) :
        _manager(manager),
        _grid_data(_manager.cells().size(),0),
        _label(label)
    { }

    /// Count all agents per cell
    void update_data () override
    {
        std::fill(_grid_data.begin(),_grid_data.end(),0);
        std::for_each(
            _manager.agents().begin(),
            _manager.agents().end(),
            [this](const auto agent){
                const auto cell = find_cell(agent,_manager);
                _grid_data.at(cell->index())++;
        });
    }

    template<typename VTKWriter>
    void add_data (VTKWriter& vtkwriter)
    {
        vtkwriter.addCellData(_grid_data,_label);
    }
};


namespace Output {

    /// Create wrapper object managing a Dune::VTKSequenceWriter
    /** \param grid Grid to operate on
     *  \param filename Name of output file
     *  \return Shared pointer to the wrapper
     */
    template<typename GridType>
    std::shared_ptr<VTKWrapper<GridType>> create_vtk_writer (std::shared_ptr<GridType> grid, const std::string filename=EXECUTABLE_NAME)
    {
        std::string filename_adj = filename+"-"+Output::get_file_timestamp();
        return std::make_shared<VTKWrapper<GridType>>(grid,filename_adj);
    }

    /// Create GridData output wrapper: Plot state for every cell
    /** \param cont Container of cells
     *  \param label Data layer label in VTK output
     *  \return Shared pointer to the wrapper
     */
    template<typename CellContainer>
    std::shared_ptr<CellStateGridDataAdaptor<CellContainer>> vtk_output_cell_state (const CellContainer& cont, const std::string label="state")
    {
        return std::make_shared<CellStateGridDataAdaptor<CellContainer>>(cont,label);
    }
    
    /// Create a GridData output wrapper: Plot result of a function for every cell
    /** \param cont Container of cells
     *  \param function Function returning the data to print for every cell
     *  \param label Data layer label in VTK output
     *  \return Shared pointer to the wrapper
     */
    template<typename CellContainer, typename Func>
    std::shared_ptr<FunctionalGridDataAdaptor<CellContainer, Func>> vtk_output_cell_function (const CellContainer& cont,
        Func function, const std::string label="function")
    {
        return std::make_shared<FunctionalGridDataAdaptor<CellContainer, Func>>(cont, function, label);
    }

    /// Create a GridData output wrapper: Plot cluster ID dependent on state for every cell
    /** \param cont Container of cells
     *  \param upper Upper end of state range to incorporate
     *  \param lower Lower end of state range to incorporate
     *  \param label Data layer label in VTK output
     */
    template<typename CellContainer, typename StateType=int>
    std::shared_ptr<CellStateClusterGridDataAdaptor<CellContainer>> vtk_output_cell_state_clusters (const CellContainer& cont, const StateType lower=0, const StateType upper=0, const std::string label="clusters")
    {
        std::array<StateType,2> range({lower,upper});
        return std::make_shared<CellStateClusterGridDataAdaptor<CellContainer>>(cont,label,range);
    }

    /// create GridData output wrapper: Plot the number of agents per cell
    /** \param cells Container of cells
     *  \param agents Container of agents
     *  \param label Data layer label in VTK output
     */
    template<class Manager>
    std::shared_ptr<AgentCountGridDataAdaptor<Manager>>
        vtk_output_agent_count_per_cell
        (const Manager& manager,
         const std::string label="agent_count")
    {
        return std::make_shared<AgentCountGridDataAdaptor<Manager>>(manager,label);
    }

} // namespace Output

} // namespace Citcat

#endif // DATA_VTK_HH
