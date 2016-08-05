#ifndef SIMULATION_HH
#define SIMULATION_HH

template<typename GridType, typename CellContainerType, typename IndividualContainerType>
class SimulationWrapper
{
public:
	// export data types
	// todo: more!!
	using Grid = GridType;
	using CellContainer = CellContainerType;
	using IndividualContainer = IndividualContainerType;

	using CellPtr = typename CellContainer::value_type;
	using Cell = typename CellPtr::element_type;

	using IndividualPtr = typename IndividualContainer::value_type;
	using Individual = typename IndividualPtr::element_type;

	using State = typename Cell::State;

private:
	std::shared_ptr<Grid> g;
	CellContainer& cc;
	IndividualContainer& ic;

public:
	/// Construct Wrapper
	/** \param grid Shared pointer to the grid
	 */
	SimulationWrapper (std::shared_ptr<Grid> grid, CellContainer& cells, IndividualContainer& individuals) :
		g(grid), cc(cells), ic(individuals)
	{ }

	/// Return reference to cell container
	CellContainer& cells () { return cc; }
	/// Return const reference to cell container
	const CellContainer& cells () const { return cc; }

	/// Return reference to individuals container
	IndividualContainer& individuals () { return ic; }
	/// Return const reference to individuals container
	const IndividualContainer& individuals () const { return ic; }

	/// Return shared pointer to grid
	std::shared_ptr<Grid> grid () const { return g; }

};

/// Manage state propagation and data printout
/** \tparam SimulationWrapper Type of wrapper object
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

	SimulationWrapper data; //!< The simulation wrapper object containing references to the data containers

	//! vector containing all rules. rules will be applied in order
	std::vector<StateRule> rules;
	//! vector containing all BCs - or none. BCs will be applied in order
	std::vector<StateRule> bc;
	//! bool whether update is called after every single rule is applied
	bool update_always;

	//! Output writers. Tuple contains writer, interval for printout and time of next printout
	std::vector< std::tuple<std::shared_ptr<DataWriter>,const float,float> > output;

	float dt; //!< time step size
	float time; //!< current time

	int _steps; //!< count of completed iterations
	Dune::Timer sim_timer; //!< timer for lifetime of this object
	Dune::Timer cout_timer; //!< timer for cout info printer
	float cout_interval; //!< interval of cout info printout in seconds
	Dune::Timer rule_timer; //!< timer for rule application (cumulative)
	Dune::Timer update_timer; //!< timer for cell update (cumulative)
	Dune::Timer data_timer; //!< timer for data printout (cumulative)

public:

	/// Default constructor
	/** \param data_ Wrapper object to run the simulation on
	 */
	Simulation (SimulationWrapper& data_) :
		data(data_), update_always(true), dt(1.0), time(0.0), _steps(0),
		sim_timer(true), cout_timer(true), cout_interval(10.0),
		rule_timer(false), update_timer(false), data_timer(false)
	{ }

	/// Destructor. Displays statistics of this simulation object
	~Simulation ()
	{
		std::cout << "------" << std::endl;
		std::cout << std::scientific;
		std::cout << "Simulation runtime: " << sim_timer.elapsed() << std::endl;
		std::cout << "Rule application time per step: " << rule_timer.elapsed()/_steps << std::endl;
		std::cout << "Update time per step: " << update_timer.elapsed()/_steps << std::endl;
		std::cout << "Data printout time per step " << data_timer.elapsed()/_steps << std::endl;
	}

	//! Return current time
	float get_time () const { return time; }
	//! Return timestep size
	float get_timestep () const { return dt; }
	//! Set current time
	void set_time (const float time_) { time = time_; }
	//! Set timestep size
	void set_timestep (const float dt_) { dt = dt_; }
	//! Set to true if all cells should be updated after every rule
	void set_update_after_every_rule (const bool update) { update_always = update; }

	/// Add a rule
	template<typename F>
	void add_rule (F f)
	{
		//static_assert(std::is_function<F>::value,"This expression is not a function!");
		// assert that pointer to cell inserted into function object results in StateType
		static_assert(std::is_same<State,typename std::result_of<F(std::shared_ptr<Cell>)>::type>::value,"This rule does not return a variable of type 'State'!");
		rules.push_back(f);
	}

	/// Add a boundary condition
	template<typename F>
	void add_bc (F f)
	{
		static_assert(std::is_same<State,typename std::result_of<F(std::shared_ptr<Cell>)>::type>::value,"This rule does not return a variable of type 'State'!");
		bc.push_back(f);
	}

	/// Add an output writer to the Simulation.
	/** \param writer Shared pointer to the output writer
	 *  \param interval Interval of data printout
	 */
	template<typename DerivedDataWriter>
	void add_output (std::shared_ptr<DerivedDataWriter> writer, const float interval=1.0)
	{
		static_assert(std::is_base_of<DataWriter,DerivedDataWriter>::value,
			"Object for writing output must be derived from DataWriter interface");
		output.push_back(std::make_tuple(writer,interval,time));
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
	/** \param t_end Time limit
	 */
	void run (const float t_end)
	{
		std::cout << "------" << std::endl;
		std::cout <<"[  0\%] Commencing simulation run until time "<<t_end<<std::endl;
		write_data(); // initial printout
		while(time<t_end){
			print_info(time,t_end);
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

	/// Call data printout on added output writers. Considers the applied output interval
	void write_data ()
	{
		data_timer.start();
		for(auto&& i : output){
			// check if print time has been passed
			auto& print_time = std::get<2>(i);
			if(print_time<=time){
				auto writer = std::get<0>(i);
				writer->write(time);
				// advance print time by interval
				print_time += std::get<1>(i);
			}
		}
		data_timer.stop();
	}

private:

	void advance_time () { time += dt; }

	/// Calculate new states and update all cells
	void advance_cells ()
	{
		rule_timer.start();
		if(bc.empty())
			apply_rules_cells();
		else
			apply_rules_bc_cells();
		rule_timer.stop();

		if(!update_always)
			update_cells();
	}

	/// Apply all rules to all cells
	void apply_rules_cells ()
	{
		for(const auto& f : rules){
			for(auto&& cell : data.cells())
				cell->new_state() = f(cell);
			if(update_always)
				update_cells();
		}
	}

	/// Apply all rules to non-boundary cells. Apply all BCs to boundary cells
	void apply_rules_bc_cells ()
	{
		const auto iter = std::max(rules.size(),bc.size());
		for(int i=0; i<iter; i++)
		{
			// only apply rules if there are any left
			const bool do_bc = (i<bc.size());
			const bool do_rule = (i<rules.size());

			for(auto&& cell : data.cells()){
				if(do_bc && cell->boundary()){
					cell->new_state() = bc[i](cell);
				}
				else if(do_rule){
					cell->new_state() = rules[i](cell);
				}
			}
			if(update_always)
				update_cells();
		}
	}

	/// Call update() on all cells
	void update_cells ()
	{
		update_timer.start();
		for(auto&& cell : data.cells())
			cell->update();
		update_timer.stop();
	}

	/// Print info into the shell if cout_timer passed cout_interval
	/** \param current Time or number of steps computed
	 *  \param finish Finish time or target number of steps
	 */
	void print_info (const float current, const float finish)
	{
		if(cout_timer.elapsed()>cout_interval){
			const std::string perc =  std::to_string(static_cast<int>(std::ceil(current*100/finish)));
			std::cout <<"[";
			for(int i=perc.size(); i<3; i++)
				std::cout<<" ";
			std::cout <<perc<<"\%] Simulation at step "<< _steps <<std::endl;
			cout_timer.reset();
		}
	}

};

#endif // SIMULATION_HH
