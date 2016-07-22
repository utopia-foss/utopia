#ifndef DATA_HH
#define DATA_HH

#ifndef OUTPUTDIR
#define OUTPUTDIR ""
#endif

#ifndef EXECUTABLE_NAME
#define EXECUTABLE_NAME "toolbox"
#endif

#define COM "# " // Comment
#define LIM "," // Separator
#define LINBR "\n" // Linebreak
#define PREC std::fixed // Floating point output precision
#define FILETYPE ".dat"

/// Generic data write interface.
class DataWriter
{
public:

	DataWriter () = default;
	virtual ~DataWriter () = default;

	/// Virtual write function. Write sequence has to be specified by derived class
	/** \param time Current time
	 */
	virtual void write (const float time) = 0;
};

/// Manage a single data file
/** \tparam Container Container object of data to be printed
 */
template<typename Container>
class ContainerDataWriter : public DataWriter
{
protected:

	const Container& data; //!< data to operate on
	std::ofstream file; //!< filestream to write into

public:

	/// Default constructor. Opens a file for writing
	/** \param data_ Container of data to write
	 *  \param filename Name of the file to open
	 */
	ContainerDataWriter (const Container& data_, const std::string filename) :
		data(data_)
	{
		file.open(filename);
		write_header_base();
		file << PREC; // set output precision
	}

	/// Virtual write header function. Will be called once by the constructor.
	virtual void write_header () = 0;

protected:

	/// Default destructor. Closes the filestream
	~ContainerDataWriter ()
	{
		file.flush();
		file.close();
	}

private:

	/// Write the basic header: date, time, stuff.
	void write_header_base ()
	{
		const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		file << COM << std::put_time(std::localtime(&time), "%F %T") << LINBR;
	}

};

template<typename Container>
class TimeStateMeanWriter final : public ContainerDataWriter<Container>
{
public:
	/// Constructor.
	/** \param data Container of data
	 *  \param filename Output file name
	 */
	TimeStateMeanWriter (const Container& data_, const std::string filename) :
		ContainerDataWriter<Container>(data_,filename)
	{
		write_header();
	}

	void write_header ()
	{
		this->file << COM << "time" << LIM << "mean" << LINBR;
	}

	void write (const float time)
	{
		float mean = .0;
		for(const auto& i : this->data)
			mean += i->state();
		mean /= this->data.size();

		this->file << time << LIM << mean << LINBR;
	}
};

template<typename Container>
class TimeStateDensityWriter : public ContainerDataWriter<Container>
{
protected:
	using State = typename Container::value_type::element_type::State;
public:
	/// Constructor.
	/** \param data Container of data
	 *  \param filename Output file name
	 *  \param range_ Array containing the two range limits of the output
	 */
	TimeStateDensityWriter (const Container& data_, const std::string filename, std::array<State,2> range_) :
		ContainerDataWriter<Container>(data_,filename),
		range(range_)
	{
		static_assert(std::is_integral<State>::value,"This data writer does not support the data type of 'State'");
		write_header();
	}

	~TimeStateDensityWriter () = default;

	void write_header ()
	{
		this->file << COM << "time";
		for(auto i=range[0]; i<=range[1]; i++)
			this->file << LIM << i;
		this->file << LINBR;
	}

	void write (const float time)
	{
		// initialize data array
		const State size = range[1]-range[0]+1;
		const State rtl = -range[0];
		std::vector<float> data_row(size,.0);

		// iterate over container
		for(const auto& i : this->data){
			data_row[i->state()+rtl] += 1.0;
		}

		// write
		this->file << time;
		for(auto&& j : data_row){
			j /= this->data.size();
			this->file << LIM << j;
		}
		this->file << LINBR;
	}

private:

	std::array<State,2> range; //!< Range of integral states is [ range[0],range[1] ]

};


namespace Output {

	/// Return string of format YYMMDDHHMMSS
	std::string get_file_timestamp ()
	{
		const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		char stamp[12];
		std::strftime(stamp,sizeof(stamp),"%y%m%d%H%M%S",std::localtime(&time));
		const std::string stamp_str(stamp);
		return stamp_str;
	}

	/// Create Output Writer: Time vs. Mean of all states
	/**
	 *  \param cont Data container
	 *  \param filename Name of output file (without extension)
	 */
	template<typename Container>
	std::shared_ptr<TimeStateMeanWriter<Container>> plot_time_state_mean (const Container& cont, const std::string filename="state_mean")
	{
		static_assert(std::is_arithmetic<typename Container::value_type::element_type::State>::value,"This data writer does not support the data type of 'State'");

		std::string filename_adj = OUTPUTDIR+filename+"-"+get_file_timestamp()+FILETYPE;
		return std::make_shared<TimeStateMeanWriter<Container>>(cont,filename_adj);
	}

	/// Create Output Writer: Time vs. Density of specific state
	/** State range will be automatically determined from
	 *  initial state distribution, if not stated specifically.
	 *  \param cont Data container
	 *  \param filename Name of output file (without extension)
	 *  \param lower Lower end of state range to plot
	 *  \param upper Upper end of state range to plot
	 */
	template<typename Container, typename StateType=int>
	std::shared_ptr<TimeStateDensityWriter<Container>> plot_time_state_density (const Container& cont, const StateType lower=0, const StateType upper=0, const std::string filename="state_density")
	{
		static_assert(std::is_integral<typename Container::value_type::element_type::State>::value,"This data writer does not support the data type of 'State'");

		std::string filename_adj = OUTPUTDIR+filename+"-"+get_file_timestamp()+FILETYPE;

		std::array<StateType,2> range({lower,upper});
		if(lower==0 && upper==0)
		{
			// read range from current state range
			for(const auto& cell : cont){
				range[0] = std::min(cell->state(),range[0]);
				range[1] = std::max(cell->state(),range[1]);
			}
		}
		return std::make_shared<TimeStateDensityWriter<Container>>(cont,filename_adj,range);
	}

}



#endif // DATA_HH