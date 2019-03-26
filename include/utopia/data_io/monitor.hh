#ifndef UTOPIA_DATAIO_MONITOR_HH
#define UTOPIA_DATAIO_MONITOR_HH

#include <vector>
#include <chrono>
#include <iostream>
#include <utility>

#include <yaml-cpp/yaml.h>

namespace Utopia {
namespace DataIO {

/// Test whether an object is callable. 
/** is_callable returns true for callable functions such as std::functions or
 * lambdas. However, it returns false if an object only provides an operator().
 * This is necessary because otherwise types such as int or double would
 * also return true (for this case use std::is_callable). 
 * 
 * @tparam F The function
 * @tparam Args The function arguments
 */
template<class F, class...Args>
struct is_callable
{
    template<class U> static auto test(U* p) 
        -> decltype((*p)(std::declval<Args>()...), void(), std::true_type());
    template<class U> static auto test(...) 
        -> decltype(std::false_type());

    static constexpr bool value = decltype(test<F>(0))::value;
};


/// The MonitorTimer keeps track of the time when to emit monitor data
class MonitorTimer {
public:
    // -- Data types uses throughout the monitor timer-- //

    /// Data type for the clock
    using Clock = std::chrono::high_resolution_clock;

    /// Data type for a time point
    using Time = std::chrono::high_resolution_clock::time_point;

    /// Data type for the time unit
    using DurationType = std::chrono::duration<double>;

private:
    // -- Member declaration -- //
    /// The emit interval
    const DurationType _emit_interval;

    /// The starting time of the timer
    const Time _start_time;

    /// The time of the last emit
    Time _last_emit;

public:
    /// Constructor
    /** Construct a new Monitor Timer object. The _last_emit time is set to the
     * time of construction.
     * @param emit_interval The time interval that defines whether the time has
     * come to emit data. If more time than the _emit_interval has passed
     * the time_has_come function returns true.
     */
    MonitorTimer (const double emit_interval)
    :
        // Store the emit interval
        _emit_interval(emit_interval),
        // Set the starting time member
        _start_time(Clock::now()),
        // Initialize _last_emit empty, setting it to 1.1.1970, 0:00, meaning
        // that no emit has occurred yet.
        _last_emit()
    {};

    /// Check for whether the time to emit has come or not.
    /**
     * @param reset Reset the internal timer to the current time
     *        if the _emit_interval has been exceeded.
     * @return true if the internal timer has exceeded the _last_emit time.
     */
    bool time_has_come() const {
        // Calculate the time difference between now and the last emit
        const DurationType duration = Clock::now() - _last_emit;
        
        // If more time than the _emit_interval has passed, return true
        return (duration > _emit_interval);
    }

    /// Reset the timer to the current time
    void reset() {
        _last_emit = Clock::now();
    }

    /// Get the time elapsed since the start of this timer
    template<class DurationT=DurationType>
    const DurationT get_time_elapsed() const {
        return Clock::now() - _start_time;
    }

    /// Get the time elapsed since start of this timer, converted to seconds
    double get_time_elapsed_seconds() const {
        return get_time_elapsed<std::chrono::duration<double>>().count();
    }

    /// Return the emit interval
    const DurationType get_emit_interval() const {
        return _emit_interval;
    }
};


/// The MonitorManager manages the monitor entries and MonitorTimer
/** 
 * The manager performs an emission of the stored monitor data
 * if the monitor timer asserts that enough time has passed since
 * the last emit.
 */
class MonitorManager {
private:
    // -- Type definitions -- //
    /// Type of the timer
    using Timer = std::shared_ptr<MonitorTimer>;

    // -- Member declaration -- //
    /// The monitor timer
    Timer _timer;

    /// The monitor entries
    YAML::Node _entries;

    /// The flag that determines whether to collect data
    bool _emit_enabled;

    /// Counts the number of emit operations
    std::size_t _emit_counter;

    /// A prefix to the emitted string
    const std::string _emit_prefix;
    
    /// A suffix to the emitted string
    const std::string _emit_suffix;

public:
    /// Constructor
    /**
     * @param emit_interval The emit interval that specifies after how much
     *                      time to emit the monitor data.
     * @param emit_prefix   A prefix to the emitted string, default: "!!map "
     * @param emit_suffix   A suffix to the emitted string, default: "". Note
     *                      that std::endl is always appended.
     */
    MonitorManager (const double emit_interval,
                    const std::string emit_prefix="!!map ",
                    const std::string emit_suffix="")
    :
        // Create a new MonitorTimer object
        _timer(std::make_shared<MonitorTimer>(emit_interval)),
        // Create an empty MonitorEntries object for the data to be emitted
        _entries(YAML::Node()),
        // Initialially set the collect data flag to true
        _emit_enabled(true),
        _emit_counter(0),
        _emit_prefix(emit_prefix),
        _emit_suffix(emit_suffix)
    { 
        _entries.SetStyle(YAML::EmitterStyle::Flow);
    };

    /// Perform an emission of the data to the terminal, if the flag was set
    void emit_if_enabled () {
        if (_emit_enabled){
            // Emit the monitor entries to the terminal, adding a prefix
            std::cout << _emit_prefix << _entries << _emit_suffix << std::endl;

            // Take care of the counter, reset the timer and the emit flag
            _emit_counter++;
            _timer->reset();
            _emit_enabled = false;
        }
    }

    /// Checks with the timer whether the time to emit has come.
    void check_timer () {
        if (_timer->time_has_come()) {
            _emit_enabled = true;
        }
    }

    /// Returns true if the emission is enabled
    /* @detail This function can be used as a more performative way to checking
     *         whether it makes sense to collect monitor entries; it makes only
     *         sense to collect entries, if the emission will actually performed
     *         in the current time step.
     */
    bool emit_enabled() const {
        return _emit_enabled;
    }

    /// Set an entry in the monitor entries
    /** 
     * @tparam Value The type of the value that should be monitored
     * @param model_name The model name which will be prefixed to the key
     * @param key The key of the new entry
     * @param value The value of the new entry
     */
    template<typename Value>
    void set_entry( const std::string model_name, 
                    const std::string key, 
                    const Value value){
        _entries[model_name + "." + key] = value;
    }

    /// Set time- and progress-related top level entries
    /** @detail Using the given parameters, this method sets the top-level
     *          entries 'time' and 'progress'
     *
     *  @tparam Time The data type of the time
     *  @param time     The current time
     *  @param time_max The maximum time of the simulation
     */
    template<typename Time>
    void set_time_entries(const Time time, const Time time_max) {
        _entries["time"] = time;

        // Add the progress indicator and the elapsed time
        _entries["progress"] = float(time) / float(time_max);
    }

    /// Get a shared pointer to the MonitorTimer object.
    Timer& get_timer(){
        return _timer;
    }
    
    /// Return the emit interval
    auto get_emit_interval() {
        return _timer->get_emit_interval();
    }
    
    /// Return the emit interval
    auto get_emit_counter() {
        return _emit_counter;
    }

    /// Get the reference to the monitor entries object.
    YAML::Node& get_entries(){
        return _entries;
    }
};


/// The Monitor monitors entries that are emitted if a given time has passed.
class Monitor {
private:
    /// The name of the monitor
    const std::string _name;

    /// The monitor manager
    std::shared_ptr<MonitorManager> _mtr_mgr;

public:
    /// Constructor
    /** Construct a new Monitor object. 
     * 
     * @param name The name of the monitor
     * @param root_mtr_mgr The root monitor manager
     */
    Monitor(const std::string name,
            std::shared_ptr<MonitorManager> root_mtr_mgr)
    :
        _name(name),
        _mtr_mgr(root_mtr_mgr)
    {};

    /// Constructor
    /** Construct a new Monitor object. The shared pointer to the MonitorManager
     * points at the same MonitorManager as in the parent monitor object.
     * 
     * @param name The name of the monitor
     * @param parent_mtr The parent monitor
     */
    Monitor(const std::string name,
            Monitor& parent_mtr)
    :
        _name(parent_mtr.get_name() + "." + name),
        _mtr_mgr(parent_mtr.get_monitor_manager())
    {};

    /// Provide a new entry to the monitor manager.
    /** This entry is set regardless of whether the emission is enabled!
     * @tparam Func The type of the function (rvalue reference)
     * @param key The key of the new entry
     * @param value The value of the new entry
     */
    template <typename Func>
    void set_by_func(const std::string key, Func&& f){
        _mtr_mgr->set_entry(_name, key, f());
    }

    /// Provide a new entry to the monitor manager.
    /** This entry is set regardless of whether the emission is enabled!
     * @tparam Value The type of the value (lvalue reference)
     * @param key The key of the new entry
     * @param value The value of the new entry
     */
    template <typename Value>
    void set_by_value(const std::string key, Value const & v){
        _mtr_mgr->set_entry(_name, key, v);
    }
 
    /// Provide a new entry to the monitor manager.
    /** This entry is set regardless of whether the emission is enabled!
     * This function tests whether the argument is callable like a std::function
     * or a lambda excluding the operator(). Without this exclusion, types such
     * as int or double would also be classified callable.
     * @tparam Arg The type of the argument 
     * @param key The key of the new entry
     * @param arg The argument (value or function) that determines the value of 
     * the new entry
     */
    template <typename Arg>
    void set_entry(const std::string key, Arg arg) 
    {
        if constexpr (is_callable<Arg>::value){
            set_by_func(key, arg);
        }
        else{
            set_by_value(key, arg);
        }
    }

    /// Get a shared pointer to the MonitorManager.
    std::shared_ptr<MonitorManager> get_monitor_manager() const {
        return _mtr_mgr;
    }

    /// get the name of the monitor.
    std::string get_name() const {
        return _name;
    }
};

} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_DATAIO_MONITOR_HH
