#include <PSgraf311.h>  //on LINUX use PSgraf3.h


namespace Citcat
{

template<typename CellContainer>
class EPSWriter {

protected:
	//const CellContainer& _cells;
	const std::string _filepath;
	std::vector<std::tuple<std::vector<double>*, const std::string > > _data;

public:
	EPSWriter(const std::string filename, 
		const std::string outputdir = OUTPUTDIR) :
		_filepath(outputdir+"/"+filename)
		{ }

	addCellData(std::vector<double> &grid_data, const std::string label) { 
		_data.push_back(std::make_tuple(&grid_data, label));
	}

	~EPSWriter() = default;

	void write(const float time) {
		int Nx = 32; int Ny = 32;
		int DX = 30;
		for (int i = 0; i < _data.size(); i++) {
			auto data_tuple_set = _data[i];
			auto data_set = *(std::get<0>(data_tuple_set));
			auto label = std::get<1>(data_tuple_set);
			//std::tie(data_set, label) = data_tuple_set; 
			
			double max = 0;
			double min = 0;
			for (int i = 0; i < Nx*Ny; i++) {				
				max = std::max(max, data_set[i]); 
				min = std::min(min, data_set[i]);
			}
			double map[Nx*Ny];
			for (int i = 0; i < Nx*Ny; i++) {
				double value = data_set[i];
				if (min < 0) { value += min; max += min; }
				if (max > 0) value = value / max; 
				map[i] = value;
			}

			std::string filename_adj = _filepath+"-"+label+"-"+std::to_string((int)time);
			char * writable = new char[filename_adj.size() + 1];
			std::copy(filename_adj.begin(), filename_adj.end(), writable);
			writable[filename_adj.size()] = '\0';
		  	
		  	gPaper(writable);
	    	//sColorSpace("HSB");
		  		dBitMap(0,0,32,32,32,32, map);
			endPS();

			//delete[] map;
		}
	}

};


template<typename CellContainer> //aim: pass shared_ptr<GridType> grid...
class EPSDataWriter : public DataWriter {
	/*using GridTypes = GridTypeAdaptor<GridType>;
	using GridView = typename GridTypes::GridView;

	typedef typename GridView::template Codim< 0 >::Entity Cell;
	typedef Cell Entity;
   	typedef typename GridCellIterator::Reference EntityReference;*/

protected:
	//const std::shared_ptr<GridType> _grid;
	const CellContainer& _cells;
	const std::string _filepath;

public:
	EPSDataWriter(const CellContainer& cells,
		const std::string filename, 
		const std::string outputdir) :
		_cells(cells),
		_filepath(outputdir+"/"+filename+"-"+Output::get_file_timestamp())
	{ 
		//change string to char

		//rearrange _cells to data readable by PSgraf
	}

	~EPSDataWriter() = default;

	template<typename State_3d, typename State=double>
	void write(const float time, std::_Mem_fn<State (State_3d::*)() const> &stateValue, const std::string label="state"){
		int Nx = 32; int Ny = 32;
		int DX = 30;
		double map[Nx*Ny];
		double maxH = 0;
		for (auto it_cells : _cells) {
			maxH = std::max(maxH, stateValue(it_cells->state())); }
		for (int i = 0; i < Nx*Ny; i++) {
			map[i] = stateValue(_cells[i]->state()) / maxH; }

		std::string filename_adj = _filepath+"-"+std::to_string((int)time);
		char * writable = new char[filename_adj.size() + 1];
		std::copy(filename_adj.begin(), filename_adj.end(), writable);
		writable[filename_adj.size()] = '\0';
	  	
	  	gPaper(writable);
    	//sColorSpace("HSB");
	  		dBitMap(0,0,32,32,32,32, map);
		endPS();
	}
};

template<typename CellContainer>
class EPSWrapper : public DataWriter
{
protected:
	EPSWriter<CellContainer> _epswriter;
	std::vector<std::shared_ptr<GridDataAdaptor>> _adaptors;

public:
	EPSWrapper(const std::string filename) :
		_epswriter(filename, OUTPUTDIR)
		{  }

	template<typename DerivedGridDataAdaptor>
	void add_adaptor (std::shared_ptr<DerivedGridDataAdaptor> adpt)
	{
		static_assert(std::is_base_of<GridDataAdaptor,DerivedGridDataAdaptor>::value,
			"Object for writing VTK data must be derived from GridDataAdaptor!");
		adpt->add_data(_epswriter);
		_adaptors.push_back(adpt);
	}

	void write (const float time)
	{
		for(auto&& i : _adaptors){
			i->update_data();
		}
		_epswriter.write(time);
	}
};

template<typename CellContainer, typename State_3d, typename State>
class EPSMemberCellStateGridDataAdaptor : public GridDataAdaptor
{
protected:
	using Cell = typename CellContainer::value_type;

	const CellContainer& _cells; //!< Container of entities
	std::vector<double> _grid_data; //!< Container for VTK readout
	const std::string _label; //!< data label
	std::_Mem_fn<State (State_3d::*)() const> _stateValue;

public:
	/// Constructor
	/** \param cells Container of cells
	 *  \param label Data label in VTK output
	 */
	EPSMemberCellStateGridDataAdaptor (const CellContainer& cells, std::_Mem_fn<State (State_3d::*)() const> &stateValue, const std::string label) :
		_cells(cells), _grid_data(_cells.size()), _stateValue(stateValue), _label(label)
	{ }

	/// Add the data of this adaptor to the VTKWriter
	/** \param vtkwriter Dune VTKWriter managed by the VTKWrapper
	 */
	template<typename EPSWriter>
	void add_data (EPSWriter& epswriter)
	{
		epswriter.addCellData(_grid_data,_label);
	}

	/// Update the data managed by this adaptor
	void update_data ()
	{
		for(const auto& cell : _cells){
			_grid_data[cell->index()] = double(_stateValue(cell->state()));
		}
	}
};
	

namespace Output {

		template<typename CellContainer>
	std::shared_ptr<EPSWrapper<CellContainer>> create_eps_writer (const std::string filename=EXECUTABLE_NAME)
	{
		//TEST GRID FOR REGULAR
		std::string filename_adj = filename+"-"+Output::get_file_timestamp();
		return std::make_shared<EPSWrapper<CellContainer>>(filename_adj);
	}

	template<typename CellContainer, typename State_3d, typename State=double>
	std::shared_ptr< MemberCellStateGridDataAdaptor<CellContainer, State_3d, State> > eps_output_cell_state_member (const CellContainer& cont, std::_Mem_fn<State (State_3d::*)() const> &stateValue, const std::string label="state")
	{
		return std::make_shared<MemberCellStateGridDataAdaptor<CellContainer, State_3d, State>>(cont, stateValue, label);
	}

} //Namespace Output

} //Namespace CitCat
