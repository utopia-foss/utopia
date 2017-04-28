#include <PSgraf311.h>  //on LINUX use PSgraf3.h


namespace Citcat
{

template<typename Container>
class EPSDataWriter : public DataWriter {
protected:
	const Container& _data;
	const std::string _filepath;
	const std::string _label;

public:
	EPSDataWriter (const Container& data, const std::string label, const std::string filepath) :
		_data(data), _label(label), _filepath(filepath)
	{  }
};

template<typename Container>
class StateEPSDataWriter : public EPSDataWriter<Container>
{
protected:
	using CellPtr = typename Container::value_type;
	using StateType = typename CellPtr::element_type::State;
	using Func = typename std::function<StateType(CellPtr)>;

	Func _function;

public:
	StateEPSDataWriter(const Container& data, const std::string label, const std::string filepath) :
		EPSDataWriter<Container>(data, label, filepath)
	{ 
		static_assert(std::is_convertible<StateType,double>::value,
			"Data to write with EPS Writer must be converitble to double!");

		_function = std::function<StateType(CellPtr)>([](CellPtr cell){ return cell->state(); });
	}
	template<typename Function>
	StateEPSDataWriter(const Container& data, Function function, const std::string label, const std::string filepath) :
		EPSDataWriter<Container>(data, label, filepath)
	{ 		
		using Func = Function;
		using StateType = typename std::result_of_t<Func(CellPtr)>;

		static_assert(std::is_convertible<StateType,double>::value,
			"Data to write with EPS Writer must be converitble to double! ");

		_function = function;
	}
	void write(const float time) {
		int size = this->_data.size();
		double data[size];
		double max = _function(this->_data[0]);
		double min = _function(this->_data[0]);
		for (int it = 0; it < size; it++) {
			data[it] = _function(this->_data[it]);
			max = std::max(max, data[it]); 
			min = std::min(min, data[it]);
		}

		bool grayscale = true;
		double r[size], g[size], b[size];
		if(std::is_same<StateType,int>::value && max - min < 5) { //discrete data points
			int states = max - min + 1;
			
			//define coloring of states
			std::vector<std::vector<double> > color_map = {};
			color_map = {{1,1,1}, {0,0,0}, {0,0,1}, {0,1,0}, {1,0,0}};
			grayscale = false;

			//define colour of each datapoint
			for (int it_data = 0; it_data < size; it_data++) {
				r[it_data] = color_map[data[it_data]-min][0];
				g[it_data] = color_map[data[it_data]-min][1];
				b[it_data] = color_map[data[it_data]-min][2];
			}
		}
		else { //define grayscale map
			for (int it_data = 0; it_data < size; it_data++) {
				double gray = (data[it_data] - min) / (max - min);  //rescale values between 0 ... 1
				r[it_data] = gray;
				g[it_data] = gray;
				b[it_data] = gray;
			}
		}

		//define output path
		std::string filename_adj = this->_filepath+"-"+this->_label+"-"+std::to_string((int)time);
		char * writable = new char[filename_adj.size() + 1];
		std::copy(filename_adj.begin(), filename_adj.end(), writable);
		writable[filename_adj.size()] = '\0';

		int Nx = std::sqrt(this->_data.size()); int Ny = std::sqrt(this->_data.size());
		int w = 150; int h = 150;
		int DX = 30;
		int NCTR = 65;
		int b_off = 15; //bottom offset

		gPaper(writable);
			sXWorldCoord(0,Nx,0,w); sYWorldCoord(0,Ny,b_off,h+b_off);
			sXIntervals(Nx/4,Nx/16,0,1); sYIntervals(Ny/4,Ny/16,0,1);
			dXAxis(0,0,Nx,1); dYAxis(0,0,Ny,1);

			movea('P',5,0);
			if (std::is_same<StateType,int>::value && max - min < 5) {
				int states = max - min + 1;
				std::string text = "white (state 0), black (state 1)";
				if (states >= 3) {
					text += ", blue (state 2)";	}
				if (states >= 4) {
					text += ", yellow (state 3)"; }
				if (states >= 5) {
					text += ", red (state 4)"; }
				dText(text.c_str());	}
			else {
				std::string text = "black (lowest value: " + std::to_string(min) + ") to white (highest values: " + std::to_string(max) + ")";
				dText(text.c_str()); }

   			sColorSpace("RGB");
			dBitMap(0,b_off,w,h,Nx,Ny, r,g,b);

		endPS();

		return;
	}

};

namespace Output {

	template<typename Container>
	std::shared_ptr<StateEPSDataWriter<Container> > eps_plot_cell_state (
		const Container& cont, const std::string label="state", 
		const std::string filename = EXECUTABLE_NAME, const std::string outputdir = OUTPUTDIR)
	{
		std::string filename_adj = OUTPUTDIR+filename+"-"+get_file_timestamp();
		return std::make_shared<StateEPSDataWriter<Container>>(cont,label,filename_adj);
	}

	template<typename Container, typename Func>
	std::shared_ptr<StateEPSDataWriter<Container> > eps_plot_cell_function (
		const Container& cont, Func function, const std::string label="state", 
		const std::string filename = EXECUTABLE_NAME, const std::string outputdir = OUTPUTDIR)
	{
		std::string filename_adj = OUTPUTDIR+filename+"-"+get_file_timestamp();
		return std::make_shared<StateEPSDataWriter<Container>>(cont,function,label,filename_adj);
	}

} //Namespace Output

} //Namespace CitCat
