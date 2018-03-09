#ifndef SIMULATION_HH
#define SIMULATION_HH

#include <dune/utopia/data.hh>

namespace Utopia
{

/// Manage state propagation and data printout
/** This class serves as an interface for running simulations.
 *  It saves a SimulationWrapper element referencing the data to operate on,
 *  as well as the CA rules and boundary conditions to be applied.
 *  Output writers can be stacked for writing output in arbitrary intervals.
 *
 *  \tparam GridManager Type of manager object
 */
template<class GridManager>
class Simulation
{
public:

    /// Save a reference to the GridManager and start the timers.
    /** \param manager GridManager to run the simulation on
     */
    Simulation (GridManager& manager) :
        _manager(manager), _update_always(true), _dt(1.0), _time(0.0), _steps(0),
        _timer_sim(true), _timer_cout(true), _cout_interval(10.0),
        _timer_rule(false), _timer_update(false), _timer_data(false)
    { }

protected:

    using Cell = typename GridManager::Cell;
    using State = typename Cell::State;
    using StateRule = std::function<State(const std::shared_ptr<Cell>)>;

    GridManager& _manager; //!< The simulation wrapper object containing references to the data containers

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
            for(auto&& cell : _manager.cells())
                cell->state_new() = f(cell);
            if(_update_always)
                update_cells();
        }
    }

    /// Apply all rules to non-boundary cells. Apply all BCs to boundary cells
    void apply_rules_bc_cells ()
    {
        const auto iter = std::max(_rules.size(),_bc.size());
        for(unsigned int i=0; i<iter; i++)
        {
            const bool do_bc = (i<_bc.size());
            const bool do_rule = (i<_rules.size());

            for(auto&& cell : _manager.cells()){
                if(do_bc && cell->is_boundary()){
                    cell->state_new() = _bc[i](cell);
                }
                else if(do_rule){
                    cell->state_new() = _rules[i](cell);
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
        for(auto&& cell : _manager.cells())
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

} // namespace Utopia

#endif // SIMULATION_HH
