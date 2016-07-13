#ifndef DATA_VTK_HH
#define DATA_VTK_HH

/// Interface for wrapping data to be written by a VTKWrapper.
class GridDataAdaptor
{
protected:
	using VTKWriter = StaticTypes::VTKWriter;

public:
	GridDataAdaptor () = default;

	virtual void add_data (VTKWriter& vtkwriter) = 0;

	/// Update the local data before printout.
	virtual void update_data () = 0;
};

/// Manages the Dune::VTKWriter. Data Adaptors manage the data written by it.
template<typename GridType>
class VTKWrapper : public DataWriter
{
protected:
	using GV = StaticTypes::Grid::GridView;
	using VTKWriter = StaticTypes::VTKWriter;

	GV gv;
	VTKWriter vtkwriter;
	std::vector<std::shared_ptr<GridDataAdaptor>> adaptors;

public:
	VTKWrapper (const std::shared_ptr<GridType> grid, const std::string& filename) :
		gv(grid->leafGridView()),
		vtkwriter(gv,filename,"","")
	{ }

	/// Add a data adaptor to the output of this wrapper
	template<typename DerivedGridDataAdaptor>
	void add_adaptor (std::shared_ptr<DerivedGridDataAdaptor> adpt)
	{
		static_assert(std::is_base_of<GridDataAdaptor,DerivedGridDataAdaptor>::value,"Object for writing VTK data must be derived from GridDataAdaptor!");
		adpt->add_data(vtkwriter);
		adaptors.push_back(adpt);
	}

	void write (const float time)
	{
		for(auto&& i : adaptors){
			i->update_data();
		}
		vtkwriter.write(time);
	}

};


/// Write the state all entities on a grid
template<typename CellContainer>
class CellStateGridDataAdaptor : public GridDataAdaptor
{
protected:
	using Cell = typename CellContainer::value_type::element_type;
	using State = typename Cell::State;

	const CellContainer& cells; //!< Container of entities
	std::vector<State> grid_data; //!< Container for VTK readout
	const std::string label; //!< data label

public:
	/// Constructor
	/** \param cells_ Container of cells
	 *  \param label_ Data label in VTK output
	 */
	CellStateGridDataAdaptor (const CellContainer& cells_, const std::string label_) :
		cells(cells_), grid_data(cells.size()), label(label_)
	{ }

	void add_data (VTKWriter& vtkwriter)
	{
		vtkwriter.addCellData(grid_data,label);
	}

	void update_data ()
	{
		for(const auto& cell : cells){
			grid_data[cell->index()] = cell->state();
		}
	}
};
/*
template<typename SimulationWrapper>
class CellStateGridDataWriter final : public GridDataWriter<SimulationWrapper>
{
protected:
	using Cell = typename SimulationWrapper::CellContainer::value_type::element_type;
	using State = typename Cell::State;

	using Mapper = StaticTypes::Grid::Mapper;
	using Index = StaticTypes::Grid::Index;

	std::vector<State> grid_data; //!< data to be written

public:
	/// Constructor for scalar valued States
	CellStateGridDataWriter (const SimulationWrapper& data, const std::string& filename) :
		GridDataWriter<SimulationWrapper>(data,filename), grid_data(data.cells().size())
	{
		this->vtkwriter.addCellData(grid_data,"State");
	}

	/// Constructor for vector valued States
	CellStateGridDataWriter (const SimulationWrapper& data, const int dim, const std::string& filename) :
		GridDataWriter<SimulationWrapper>(data,filename), grid_data(data.cells().size())
	{
		static_assert(std::is_array<State>::value,"State type is not an array!");
		static_assert(dim<=grid_data[0].size(),"State has less components than specified for output!");
		this->vtkwriter.addCellData(grid_data,"State",dim);
	}

	void write (const float time)
	{
		// loop over all cells
		for(const auto& cell : this->data.cells()){
			grid_data[cell->index()] = cell->state();
		}
		this->vtkwriter.write(time);
	}
};
*/
namespace Output {

	/// Create a VTKWrapper and return a pointer to it.
	/** \param grid Grid to operate on
	 *  \param filename Name of output file
	 */
	template<typename GridType>
	std::shared_ptr<VTKWrapper<GridType>> create_vtk_writer (std::shared_ptr<GridType> grid, std::string filename="grid")
	{
		return std::make_shared<VTKWrapper<GridType>>(grid,filename);
	}

	/// Create a GridData output wrapper: Plot state for every cell
	/** \param cont Container of cells
	 *  \param label Data label in VTK output
	 */
	template<typename CellContainer>
	std::shared_ptr<CellStateGridDataAdaptor<CellContainer>> vtk_output_cell_state (const CellContainer& cont, const std::string label="state")
	{
		return std::make_shared<CellStateGridDataAdaptor<CellContainer>>(cont,label);
	}

} // Output

#endif // DATA_VTK_HH
