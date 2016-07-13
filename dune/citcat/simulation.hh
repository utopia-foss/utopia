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
	using StateRule = std::function<State(const std::shared_ptr<const Cell>)>;

	SimulationWrapper data; //!< The simulation wrapper object containing references to the data containers

	std::vector<StateRule> rules;
	std::vector<StateRule> bc;

	//! Output writers. Tuple contains writer, interval for printout and time until next printout
	std::vector< std::tuple<std::shared_ptr<DataWriter>,const float,float> > output;

	float dt; //!< time step size
	float time; //!< current time

	int steps;
	Dune::Timer rule_timer;
	Dune::Timer update_timer;
	Dune::Timer data_timer;

public:

	/// Default constructor
	/** \param data_ Wrapper object to run the simulation on
	 */
	Simulation (SimulationWrapper& data_) :
		data(data_), dt(1.0), time(0.0),
		steps(0), rule_timer(false), update_timer(false), data_timer(false)
	{ }

	~Simulation ()
	{
		std::cout << std::scientific;
		std::cout << "Rule application time per step: " << rule_timer.elapsed()/steps << std::endl;
		std::cout << "Update time per step: " << update_timer.elapsed()/steps << std::endl;
		std::cout << "Data printout time per step " << data_timer.elapsed()/steps << std::endl;
	}

	//! Return current time
	float get_time () const { return time; }
	//! Return timestep size
	float get_timestep () const { return dt; }
	//! Set current time
	void set_time (const float time_) { time = time_; }
	//! Set timestep size
	void set_timestep (const float dt_) { dt = dt_; }

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
		output.push_back(std::make_tuple(writer,interval,interval));
	}

	/// Single iteration. Apply rules, print data
	void iterate ()
	{
		advance_cells();
		advance_time();
		print_data();
		steps++;
	}

	/// Run simulation until time limit is reached
	/** \param t_end Time limit
	 */
	void run (const float t_end)
	{
		print_data(); // initial printout
		while(time<t_end)
			iterate();
	}

	/// Multiple Iterations. Apply rules & print data in every step
	/** \param steps Number of time steps
	 */
	void iterate (const int steps)
	{
		for(int i=0; i<steps; i++)
			iterate();
	}

	void print_data ()
	{
		data_timer.start();
		for(auto&& i : output){
			// update timer
			auto& time_to_write = std::get<2>(i);
			time_to_write -= dt;
			if(time_to_write<=0.0){
				auto writer = std::get<0>(i);
				writer->write(time);
				// reset timer
				std::get<2>(i) = std::get<1>(i);
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
		update_cells();
	}

	/// Apply all rules to all cells
	void apply_rules_cells ()
	{
		for(auto&& cell : data.cells()){
			for(const auto& f : rules)
				cell->new_state() = f(cell);
		}
	}

	/// Apply all rules to non-boundary cells. Apply all BCs to boundary cells
	void apply_rules_bc_cells ()
	{
		for(auto&& cell : data.cells()){
			if(cell->boundary()){
				for(const auto& f : bc)
					cell->new_state() = f(cell);
			}
			else{
				for(const auto& f : rules)
					cell->new_state() = f(cell);
			}
		}
	}

	void update_cells ()
	{
		update_timer.start();
		for(auto&& cell : data.cells())
			cell->update();
		update_timer.stop();
	}

	void print_cells () const
	{
		for(const auto& cell : data.cells())
			std::cout << "Cell No. " << cell->index() << ": " << cell->state() << std::endl;
	}

};

#endif // SIMULATION_HH
