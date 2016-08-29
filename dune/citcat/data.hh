#ifndef DATA_HH
#define DATA_HH

#ifndef OUTPUTDIR
#define OUTPUTDIR ""
#endif

#ifndef EXECUTABLE_NAME
#define EXECUTABLE_NAME "toolbox"
#endif

#define COM "# " //!< Comment
#define LIM " " //!< Separator
#define LINBR "\n" //!< Linebreak
#define PREC std::fixed //!< Floating point output precision
#define FILETYPE ".dat" //!< Data file extension

namespace Citcat
{
	/// Generic data write interface.
	/** All data writers need to inherit from this class in order
	 *  to be stacked into a Simulation object.
	 */
	class DataWriter
	{
	public:

		/// Default constructor
		DataWriter () = default;
		/// Default destructor
		virtual ~DataWriter () = default;

		/// Virtual write function. Write sequence has to be specified by derived class
		/** \param time Current time
		 */
		virtual void write (const float time) = 0;
	};

	/// Manage a single data file. Data is written from a container.
	/** \tparam Container Container object of data to be printed
	 */
	template<typename Container>
	class ContainerDataWriter : public DataWriter
	{
	protected:

		const Container& _data; //!< data to operate on
		std::ofstream _file; //!< filestream to write into

	public:

		/// Default constructor. Opens a file for writing.
		/** Print generic header and define ostream precision.
		 *  \param data Container of data to write
		 *  \param filename Name of the file to open
		 */
		ContainerDataWriter (const Container& data, const std::string filename) :
			_data(data)
		{
			_file.open(filename);
			write_header_base();
			_file << PREC;
		}

		/// Virtual write header function.
		/** Intended for use from the constructor of the derived class object.
		 *  Should contain information on the actual data printed
		 */
		virtual void write_header () = 0;

	protected:

		/// Default destructor. Flushed and closes the filestream
		~ContainerDataWriter ()
		{
			_file.flush();
			_file.close();
		}

	private:

		/// Write the default header: date, time, stuff.
		void write_header_base ()
		{
			const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			_file << COM << std::put_time(std::localtime(&time), "%F %T") << LINBR;
		}

	};

	template<typename Container>
	class TimeStateMeanWriter final : public ContainerDataWriter<Container>
	{
	public:
		/// Initialize the data writer and write the file header.
		/** \param data Container of data
		 *  \param filename Output file name
		 *  \throw CompilerError if data type is not arithmetic
		 */
		TimeStateMeanWriter (const Container& data, const std::string filename) :
			ContainerDataWriter<Container>(data,filename)
		{
			static_assert(std::is_arithmetic<typename Container::value_type::element_type::State>::value,
				"This data writer only supports arithmetic 'State' types!");
			write_header();
		}

		/// Write the data
		void write (const float time)
		{
			double mean = .0;
			for(const auto& i : this->_data)
				mean += i->state();
			mean /= this->_data.size();

			this->_file << time << LIM << mean << LINBR;
		}

	private:
		/// Write the data file header
		void write_header ()
		{
			this->_file << COM << "time" << LIM << "mean" << LINBR;
		}
	};

	template<typename Container>
	class TimeStateDensityWriter : public ContainerDataWriter<Container>
	{
	protected:
		using State = typename Container::value_type::element_type::State;

	public:
		/// Initialize data writer and write the file header
		/** \param data Container of data
		 *  \param filename Output file name
		 *  \param range Array containing the two range limits of the output
		 *  \throw CompilerError if data type is not integral
		 */
		TimeStateDensityWriter (const Container& data, const std::string filename, std::array<State,2> range) :
			ContainerDataWriter<Container>(data,filename),
			_range(range)
		{
			static_assert(std::is_integral<typename Container::value_type::element_type::State>::value,
				"This data writer only supports integral 'State' types!");
			write_header();
		}

		~TimeStateDensityWriter () = default;

		/// Write the data
		void write (const float time)
		{
			// initialize data array
			const State size = _range[1] - _range[0] + 1;
			const State transl = - _range[0];
			std::vector<float> data_row(size,.0);

			// iterate over container
			for(const auto& i : this->_data){
				data_row[i->state()+transl] += 1.0;
			}

			// write
			this->_file << time;
			for(auto&& j : data_row){
				j /= this->_data.size();
				this->_file << LIM << j;
			}
			this->_file << LINBR;
		}

	private:

		/// Write the output file header
		void write_header ()
		{
			this->_file << COM << "time";
			for(auto i=_range[0]; i<=_range[1]; i++)
				this->_file << LIM << "\"" << i << "\"";
			this->_file << LINBR;
		}

		std::array<State,2> _range; //!< Range of integral states is [ range[0],range[1] ]

	};

	/// Wrapper and helper functions for creating output classes
	namespace Output
	{
		/// Return string of format YYMMDDHHMMSS
		/** This function is called for creating unique output filenames
		 *  for each simulation run
		 */
		std::string get_file_timestamp ()
		{
			const auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			char stamp[15];
			std::strftime(stamp,sizeof(stamp),"%y%m%d%H%M%S",std::localtime(&time));
			const std::string stamp_str(stamp);
			return stamp_str;
		}

		/// Create Output Writer: Time vs. Mean of all states
		/** Designed for arithmetic states types
		 *  \param cont Data container
		 *  \param filename Name of output file (without extension)
		 *  \return Shared pointer to the data writer
		 */
		template<typename Container>
		std::shared_ptr<TimeStateMeanWriter<Container>> plot_time_state_mean (const Container& cont, const std::string filename="state_mean")
		{
			std::string filename_adj = OUTPUTDIR+filename+"-"+get_file_timestamp()+FILETYPE;
			return std::make_shared<TimeStateMeanWriter<Container>>(cont,filename_adj);
		}

		/// Create Output Writer: Time vs. Density of specific state.
		/** Designed for integral (countable) state types.
		 *  State range will be automatically determined from
		 *  initial state distribution, if not stated specifically.
		 *  \param cont Data container
		 *  \param lower Lower end of state range to plot
		 *  \param upper Upper end of state range to plot
		 *  \param filename Name of output file (without extension)
		 *  \return Shared pointer to the data writer
		 */
		template<typename Container, typename StateType=int>
		std::shared_ptr<TimeStateDensityWriter<Container>> plot_time_state_density (
			const Container& cont, const StateType lower=0, const StateType upper=0,
			const std::string filename="state_density")
		{
			std::string filename_adj = OUTPUTDIR+filename+"-"
				+get_file_timestamp()+FILETYPE;

			std::array<StateType,2> range({lower,upper});
			if(lower==0 && upper==0){
				// read range from current state range
				for(const auto& cell : cont){
					range[0] = std::min(cell->state(),range[0]);
					range[1] = std::max(cell->state(),range[1]);
				}
			}

			return std::make_shared<TimeStateDensityWriter<Container>>(cont,filename_adj,range);
		}

	}
}



#endif // DATA_HH