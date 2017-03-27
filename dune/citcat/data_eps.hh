#include <PSgraf311.h>  //on LINUX use PSgraf3.h


namespace Citcat
{

template<typename CellContainer, typename State>
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
		for (int i = 0; i < _data.size(); i++) {
			auto data_tuple_set = _data[i];
			auto data_set = *(std::get<0>(data_tuple_set));
			auto label = std::get<1>(data_tuple_set);
			int size = data_set.size();

			double max = data_set[0];
			double min = data_set[0];
			for (int i = 0; i < size; i++) {		
				max = std::max(max, data_set[i]); 
				min = std::min(min, data_set[i]);
			}

			bool grayscale = true;
			double r[size], g[size], b[size];			
			if(std::is_same<State,int>::value) {
				int states = max - min + 1;
				
				std::vector<std::vector<double> > color_map = {};
				if (states <= 5) {
					color_map = {{1,1,1}, {0,0,0}, {0,0,1}, {0,1,0}, {1,0,0}};
					grayscale = false;
				}
				else {
					for (int i = 0; i < states; i++) {
						double gray = i/double(states - 1);
						color_map.push_back({gray, gray, gray});
					}
				}

				for (int it_data = 0; it_data < size; it_data++) {
					for (int it_states = 0; it_states < states; it_states++) {
						if (data_set[it_data] == it_states + min) {
							r[it_data] = color_map[it_states][0];
							g[it_data] = color_map[it_states][1];
							b[it_data] = color_map[it_states][2];
							break;
						}
					}
				}
			}
			else {
				for (int it_data = 0; it_data < size; it_data++) {
					double gray = data_set[it_data];
					if (min < 0) { gray += min; max += min; }
					if (max > 0) gray = gray / max; 
					r[it_data] = gray;
					g[it_data] = gray;
					b[it_data] = gray;
				}
			}

			std::string filename_adj = _filepath+"-"+label+"-"+std::to_string((int)time);
			char * writable = new char[filename_adj.size() + 1];
			std::copy(filename_adj.begin(), filename_adj.end(), writable);
			writable[filename_adj.size()] = '\0';

			int Nx = std::sqrt(size); int Ny = std::sqrt(size);
			int w = 150; int h = 150;
			int DX = 30;
			int NCTR = 65;

			gPaper(writable);
    			sXWorldCoord(0,Nx,0,w); sYWorldCoord(0,Ny,0,w);
				sXIntervals(Nx/4,Nx/16,0,1); sYIntervals(Ny/4,Ny/16,0,1);
				dXAxis(0,0,Nx,1); dYAxis(0,0,Ny,1);

	   			sColorSpace("RGB");
				dBitMap(0,0,w,h,Nx,Ny, r,g,b);
				//dBitMap(0,0,w,h,Nx,Ny, map);

			endPS();
		}
	}
};

template<typename CellContainer, typename State>
class EPSWrapper : public DataWriter
{
protected:
	EPSWriter<CellContainer, State> _epswriter;
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

template<typename CellContainer>
class EPSCellStateGridDataAdaptor : public GridDataAdaptor
{
protected:
	using Cell = typename CellContainer::value_type::element_type;
	using State = typename Cell::State;

	const CellContainer& _cells; //!< Container of entities
	std::vector<double> _grid_data; //!< Container for VTK readout
	const std::string _label; //!< data label

public:
	/// Constructor
	/** \param cells Container of cells
	 *  \param label Data label in VTK output
	 */
	EPSCellStateGridDataAdaptor (const CellContainer& cells, const std::string label) :
		_cells(cells), _grid_data(_cells.size()), _label(label)
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
			_grid_data[cell->index()] = double(cell->state());
		}
	}
};

template<typename CellContainer, typename Cell, typename Result>
class EPSDerivedCellStateGridDataAdaptor : public GridDataAdaptor
{
protected:
	const CellContainer& _cells; //!< Container of entities
	std::vector<double> _grid_data; //!< Container for VTK readout
	const std::string _label; //!< data label
	std::function<Result(Cell)> _result;

public:
	/// Constructor
	/** \param cells Container of cells
	 *  \param label Data label in VTK output
	 */
	EPSDerivedCellStateGridDataAdaptor (const CellContainer& cells, std::function<Result(Cell)> result, const std::string label) :
		_cells(cells), _grid_data(_cells.size()), _result(result), _label(label)
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
			_grid_data[cell->index()] = double(_result(cell));
		}
	}
};

template<typename CellContainer, typename State_3d, typename Result>
class EPSMemberCellStateGridDataAdaptor : public GridDataAdaptor
{
protected:
	using Cell = typename CellContainer::value_type;

	const CellContainer& _cells; //!< Container of entities
	std::vector<double> _grid_data; //!< Container for VTK readout
	const std::string _label; //!< data label
	std::_Mem_fn<Result (State_3d::*)() const> _stateValue;

public:
	/// Constructor
	/** \param cells Container of cells
	 *  \param label Data label in VTK output
	 */
	EPSMemberCellStateGridDataAdaptor (const CellContainer& cells, std::_Mem_fn<Result (State_3d::*)() const> &stateValue, const std::string label) :
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

	template<typename CellContainer, typename State = int>
	std::shared_ptr<EPSWrapper<CellContainer, State>> create_eps_writer (const std::string filename=EXECUTABLE_NAME)
	{
		//TEST GRID FOR REGULAR
		std::string filename_adj = filename+"-"+Output::get_file_timestamp();
		return std::make_shared<EPSWrapper<CellContainer, State>>(filename_adj);
	}

	template<typename CellContainer>
	std::shared_ptr<EPSCellStateGridDataAdaptor<CellContainer>> eps_output_cell_state (const CellContainer& cont, const std::string label="state")
	{
		return std::make_shared<EPSCellStateGridDataAdaptor<CellContainer>>(cont,label);
	}

	template<typename CellContainer, typename State_3d, typename Result=double>
	std::shared_ptr< EPSMemberCellStateGridDataAdaptor<CellContainer, State_3d, Result> > eps_output_cell_state_member (const CellContainer& cont, std::_Mem_fn<Result (State_3d::*)() const> &stateValue, const std::string label="state")
	{
		return std::make_shared<EPSMemberCellStateGridDataAdaptor<CellContainer, State_3d, Result>>(cont, stateValue, label);
	}

	template<typename CellContainer, typename Cell, typename Result=double>
	std::shared_ptr<EPSDerivedCellStateGridDataAdaptor<CellContainer, Cell, Result>> eps_output_derived_cell_state (const CellContainer& cont, 
		std::function<Result(Cell)> result, const std::string label="state")
	{
		return std::make_shared<EPSDerivedCellStateGridDataAdaptor<CellContainer, Cell, Result>>(cont, result, label);
	}

} //Namespace Output

} //Namespace CitCat
