#include <PSgraf312.h>

namespace Utopia
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

    //add_info: [0] nbr of states, [1] min state, [2] max state
    template<typename DataContainer, typename DataFunction>
    double** convert_data_to_cell_data(const DataContainer &data, DataFunction function, double *add_info = NULL) {
        int size = data.size();
        auto max = function(data[0]);
        auto min = function(data[0]);
        for (int it = 0; it < size; it++) {
            max = std::max(max, function(data[it])); 
            min = std::min(min, function(data[it]));
        }
        add_info[1] = min, add_info[2] = max;

        double **rgb = 0;
        rgb = new double*[3];
        for (int c : {0,1,2}) 
            rgb[c] = new double[size];
        using StateType = typename DataContainer::value_type::element_type::State;
        if(std::is_same<StateType,int>::value && max - min < 5) { //discrete data points
            int states = max - min + 1;
            add_info[0] = states;

            //define coloring of states
            std::vector<std::vector<double> > color_map = {};
            color_map = {{1,1,1}, {0,0,0}, {1,0,0}, {0,1,0}, {0,0,1}};

            //define colour of each datapoint
            for (int it_data = 0; it_data < size; it_data++) {
                int state = function(data[it_data]);
                rgb[0][it_data] = color_map[state-min][0];
                rgb[1][it_data] = color_map[state-min][1];
                rgb[2][it_data] = color_map[state-min][2];
            }
        }
        else { //define grayscale map
            add_info[0]= 0;
            for (int it_data = 0; it_data < size; it_data++) {
                double gray = (function(data[it_data]) - min) / (max - min);  //rescale values between 0 ... 1
                rgb[0][it_data] = gray;
                rgb[1][it_data] = gray;
                rgb[2][it_data] = gray;
            }
        }
        return rgb;
    }
};

template<typename Container>
class CellStateEPSDataWriter : public EPSDataWriter<Container>
{
protected:
    using CellPtr = typename Container::value_type;
    using StateType = typename CellPtr::element_type::State;
    using Func = typename std::function<StateType(CellPtr)>;

    Func _function;

public:
    CellStateEPSDataWriter(const Container& data, const std::string label, const std::string filepath) :
        EPSDataWriter<Container>(data, label, filepath)
    { 
        static_assert(std::is_convertible<StateType,double>::value,
            "Data to write with EPS Writer must be converitble to double!");

        _function = std::function<StateType(CellPtr)>([](CellPtr cell){ return cell->state(); });
    }
    template<typename Function>
    CellStateEPSDataWriter(const Container& data, Function function, const std::string label, const std::string filepath) :
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
        double add_info[3]; //nbr_states, min/max state, min/max extension
        double **data_rgb = this->convert_data_to_cell_data(this->_data, _function, add_info);

        //define output path
        std::string filename_adj = this->_filepath+"-"+this->_label+"-"+std::to_string((int)time);
        char * writable = new char[filename_adj.size() + 1];
        std::copy(filename_adj.begin(), filename_adj.end(), writable);
        writable[filename_adj.size()] = '\0';

        gPaper(writable);
            //setting		
            int Nx = sqrt(size), Ny=sqrt(size);
            int w = 150; int h = 150;
            int DX = 30;
            int NCTR = 65;
            int b_off = 15; //bottom offset

            //coordinate system
            sXWorldCoord(0,Nx,0,w); sYWorldCoord(0,Ny,b_off,h+b_off);
            sXIntervals(Nx/4,Nx/16,0,1); sYIntervals(Ny/4,Ny/16,0,1);
            dXAxis(0,0,Nx,1); dYAxis(0,0,Ny,1);

            //labels
            movea('P',5,0);
            double states = add_info[0];
            if (states > 0 && states <= 5) {
                std::string text = "cells: white (state 0), black (state 1)";
                if (states >= 3) {
                    text += ", red (state 2)";	}
                if (states >= 4) {
                    text += ", green (state 3)"; }
                if (states >= 5) {
                    text += ", blue (state 4)"; }
                dText(text.c_str());	}
            else {
                std::string text = "cells: black (lowest value: " + std::to_string(add_info[1]) + ") to white (highest values: " + std::to_string(add_info[2]) + ")";
                dText(text.c_str()); }

            //plot data
               sColorSpace("RGB");
            dBitMap(0,b_off,w,h,Nx,Ny, data_rgb[0],data_rgb[1],data_rgb[2]);

        endPS();

        for (int c : {0,1,2}) 
            delete [] data_rgb[c];
        delete [] data_rgb;

        return;
    }
};



namespace Output {

    template<typename Container>
    std::shared_ptr<CellStateEPSDataWriter<Container> > eps_plot_cell_state (
        const Container& cont, const std::string label="state", 
        const std::string filename = EXECUTABLE_NAME, const std::string outputdir = OUTPUTDIR)
    {
        std::string filename_adj = OUTPUTDIR+filename+"-"+get_file_timestamp();
        return std::make_shared<CellStateEPSDataWriter<Container>>(cont,label,filename_adj);
    }

    template<typename Container, typename Func>
    std::shared_ptr<CellStateEPSDataWriter<Container> > eps_plot_cell_function (
        const Container& cont, Func function, const std::string label="state", 
        const std::string filename = EXECUTABLE_NAME, const std::string outputdir = OUTPUTDIR)
    {
        std::string filename_adj = OUTPUTDIR+filename+"-"+get_file_timestamp();
        return std::make_shared<CellStateEPSDataWriter<Container>>(cont,function,label,filename_adj);
    }

} //Namespace Output

} //Namespace Utopia
