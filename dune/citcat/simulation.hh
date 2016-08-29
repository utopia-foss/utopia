#ifndef SIMULATION_HH
#define SIMULATION_HH

namespace Citcat
{
	/// Wrap the data to run a simulation on. Export the associated data types
	/**
	 *  \tparam GridType Type of grid
	 *  \tparam CellContainerType Type of the container for cells
	 *  \tparam IndividualContainerType Type of the container for individuals
	 */
	template<typename GridType, typename CellContainerType, typename IndividualContainerType>
	class SimulationWrapper
	{
	public:
		using Grid = GridType;
		using CellContainer = CellContainerType;
		using IndividualContainer = IndividualContainerType;

		using CellPtr = typename CellContainer::value_type;
		using Cell = typename CellPtr::element_type;

		using IndividualPtr = typename IndividualContainer::value_type;
		using Individual = typename IndividualPtr::element_type;

		using State = typename Cell::State;

	private:
		//! shared pointer to the grid
		std::shared_ptr<Grid> _grid;
		//! reference to the cell container
		CellContainer& _cell_container;
		//! referemce to the individuals container
		IndividualContainer& _indv_container;

	public:
		/// Construct Wrapper
		/** \param grid Shared pointer to the grid
		 *  \param cells Cell container object
		 *  \param individuals Individuals container object
		 */
		SimulationWrapper (std::shared_ptr<Grid> grid, CellContainer& cells, IndividualContainer& individuals) :
			_grid(grid), _cell_container(cells), _indv_container(individuals)
		{ }

		/// Return reference to cell container
		CellContainer& cells () { return _cell_container; }
		/// Return const reference to cell container
		const CellContainer& cells () const { return _cell_container; }

		/// Return reference to individuals container
		IndividualContainer& individuals () { return _indv_container; }
		/// Return const reference to individuals container
		const IndividualContainer& individuals () const { return _indv_container; }

		/// Return shared pointer to grid
		std::shared_ptr<Grid> grid () const { return _grid; }

	};

	/// Manage state propagation and data printout
	/** This class serves as an interface for running simulations.
	 *  It saves a SimulationWrapper element referencing the data to operate on,
	 *  as well as the CA rules and boundary conditions to be applied.
	 *  Output writers can be stacked for writing output in arbitrary intervals.
	 *
	 *  \tparam SimulationWrapper Type of wrapper object
	 */
	template<typename SimulationWrapper>
	class Simulation
	{
	protected:

		using CellContainer = typename SimulationWrapper::CellContainer;
		using Cell = typename SimulationWrapper::Cell;
		using CellPtr = typename SimulationWrapper::CellPtr;

		using IndividualContainer = typename SimulationWrapper::IndividualContainer;
		using Individual = typename SimulationWrapper::Individual;

		using State = typename SimulationWrapper::State;
		using StateRule = std::function<State(const std::shared_ptr<Cell>)>;

		SimulationWrapper _data; //!< The simulation wrapper object containing references to the data containers

		//! vector containing all rules. rules will be applied in order
		std::vector<StateRule> _rules;
		//! vector containing all BCs - or none. BCs will be applied in order
		std::vector<StateRule> _bc;
		//! bool whether update is called after every single rule is applied
		bool _update_always;

		//! Output writers. Tuple contains writer, interval for printout and time of next printout
		std::vector< std::tuple<std::shared_ptr<DataWriter>,const float,float> > _output;

		float _dt; //!< time step size
		float _time; //!< current time

		int _steps; //!< count of completed iterations
		Dune::Timer _timer_sim; //!< timer for lifetime of this object
		Dune::Timer _timer_cout; //!< timer for cout info printer
		float _cout_interval; //!< interval of cout info printout in seconds
		Dune::Timer _timer_rule; //!< timer for rule application (cumulative)
		Dune::Timer _timer_update; //!< timer for cell update (cumulative)
		Dune::Timer _timer_data; //!< timer for data printout (cumulative)

	public:

		/// Save the SimulationWrapper and start the timers.
		/** \param data Wrapper object to run the simulation on
		 */
		Simulation (SimulationWrapper& data) :
			_data(data), _update_always(true), _dt(1.0), _time(0.0), _steps(0),
			_timer_sim(true), _timer_cout(true), _cout_interval(10.0),
			_timer_rule(false), _timer_update(false), _timer_data(false)
		{ }

		/// Destructor. Displays time statistics of this simulation object
		~Simulation ()
		{
			std::cout << "------" << std::endl;
			std::cout << std::scientific;
			std::cout << "Simulation runtime: " << _timer_sim.elapsed() << std::endl;
			std::cout << "Rule application time per step: " << _timer_rule.elapsed()/_steps << std::endl;
			std::cout << "Update time per step: " << _timer_update.elapsed()/_steps << std::endl;
			std::cout << "Data printout time per step " << _timer_data.elapsed()/_steps << std::endl;
		}

		//! Return current time
		float get_time () const { return _time; }
		//! Return timestep size
		float get_timestep () const { return _dt; }
		//! Set current time
		void set_time (const float time) { _time = time; }
		//! Set timestep size
		void set_timestep (const float dt) { _dt = dt; }
		//! Set to true if all cells should be updated after every rule
		void set_update_after_every_rule (const bool update) { _update_always = update; }

		/// Add a function object as rule
		/** \param f Function to be applied to a cell, returning the new state
		 *     of the cell
		 *  \throw CompilerError if rule applied does not match syntax
		 */
		template<typename F>
		void add_rule (F f)
		{
			static_assert(std::is_same<State,typename std::result_of<F(std::shared_ptr<Cell>)>::type>::value,"This rule does not return a variable of type 'State'!");
			_rules.push_back(f);
		}

		/// Add a function object as boundary condition.
		/** BC rules will be applied to all cells whose boundary() query returns true.
		 *  \param f Function to be applied to a cell, returning the new state
		 *     of the cell
		 *  \throw CompilerError if rule applied does not match syntax
		 */
		template<typename F>
		void add_bc (F f)
		{
			static_assert(std::is_same<State,typename std::result_of<F(std::shared_ptr<Cell>)>::type>::value,"This rule does not return a variable of type 'State'!");
			_bc.push_back(f);
		}

		/// Add an output writer which will print after each interval stated
		/** The writer will print immediately.
		 *  \param writer Shared pointer to the output writer
		 *  \param interval Interval of data printout
		 */
		template<typename DerivedDataWriter>
		void add_output (std::shared_ptr<DerivedDataWriter> writer, const float interval=1.0)
		{
			static_assert(std::is_base_of<DataWriter,DerivedDataWriter>::value,
				"Object for writing output must be derived from DataWriter interface");
			_output.push_back(std::make_tuple(writer,interval,_time));
		}

		/// Single iteration. Apply rules, print data
		void iterate ()
		{
			advance_cells();
			advance_time();
			write_data();
			_steps++;
		}

		/// Run simulation until time limit is reached
		/** Write data (initial condition) before first iteration
		 *  \param t_end Time limit
		 */
		void run (const float t_end)
		{
			std::cout << "------" << std::endl;
			std::cout <<"[  0\%] Commencing simulation run until time "<<t_end<<std::endl;

			write_data();
			while(_time<t_end){
				print_info(_time,t_end);
				iterate();
			}

			std::cout <<"[100\%] Finished computation until time "<<t_end<<std::endl;
		}

		/// Multiple Iterations. Apply rules & print data in every step
		/** \param steps Number of time steps
		 */
		void iterate (const int steps)
		{
			std::cout << "------" << std::endl;
			std::cout <<"[  0\%] Commencing simulation run of "<<steps<<" steps"<<std::endl;

			for(int i=0; i<steps; i++){
				print_info(i,steps);
				iterate();
			}

			std::cout <<"[100\%] Finished computation of "<<steps<<" steps"<<std::endl;
		}

		/// Call data printout on added output writers, considering the output intervals
		void write_data ()
		{
			_timer_data.start();
			for(auto&& i : _output){
				auto& print_time = std::get<2>(i);
				if(print_time<=_time){
					auto writer = std::get<0>(i);
					writer->write(_time);
					print_time += std::get<1>(i);
				}
			}
			_timer_data.stop();
		}

	private:

		/// Advance time by saved time step interval
		void advance_time () { _time += _dt; }

		/// Calculate new states and update all cells
		/** If now boundary conditions are stated, an optimized application function
		 *  is called
		 */
		void advance_cells ()
		{
			_timer_rule.start();
			if(_bc.empty())
				apply_rules_cells();
			else
				apply_rules_bc_cells();
			_timer_rule.stop();

			if(!_update_always)
				update_cells();
		}

		/// Apply the rules to all cells, including boundary cells
		void apply_rules_cells ()
		{
			for(const auto& f : _rules){
				for(auto&& cell : _data.cells())
					cell->new_state() = f(cell);
				if(_update_always)
					update_cells();
			}
		}

		/// Apply all rules to non-boundary cells. Apply all BCs to boundary cells
		void apply_rules_bc_cells ()
		{
			const auto iter = std::max(_rules.size(),_bc.size());
			for(int i=0; i<iter; i++)
			{
				const bool do_bc = (i<_bc.size());
				const bool do_rule = (i<_rules.size());

				for(auto&& cell : _data.cells()){
					if(do_bc && cell->boundary()){
						cell->new_state() = _bc[i](cell);
					}
					else if(do_rule){
						cell->new_state() = _rules[i](cell);
					}
				}

				if(_update_always)
					update_cells();
			}
		}

		/// Call update() on all cells
		void update_cells ()
		{
			_timer_update.start();
			for(auto&& cell : _data.cells())
				cell->update();
			_timer_update.stop();
		}

		/// Print info into the shell if cout_timer passed cout_interval
		/** \param current Time or number of steps computed
		 *  \param finish Finish time or target number of steps
		 */
		void print_info (const float current, const float finish)
		{
			if(_timer_cout.elapsed()>_cout_interval){
				const std::string perc = std::to_string(
					static_cast<int>(std::ceil(current*100/finish)));
				std::cout <<"[";
				for(int i=perc.size(); i<3; i++)
					std::cout<<" ";
				std::cout <<perc<<"\%] Simulation at step "<< _steps <<std::endl;
				_timer_cout.reset();
			}
		}

	};
}

#endif // SIMULATION_HH
