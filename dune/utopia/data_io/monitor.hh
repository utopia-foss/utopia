#ifndef UTOPIA_DATAIO_MONITOR_HH
#define UTOPIA_DATAIO_MONITOR_HH

#include <vector>
#include <chrono>
#include <iostream>

#include <yaml-cpp/yaml.h>

namespace Utopia {
namespace DataIO {

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
    MonitorTimer (double emit_interval)
    :
        // Store the emit interval
        _emit_interval(emit_interval),
        // Initialize _last_emit empty, setting it to 1.1.1970, 0:00, meaning
        // that no emit has occured yet.
        _last_emit()
    {};

    /// Check for whether the time to emit has come or not.
    /**
     * @param reset Reset the internal timer to the current time
     *        if the _emit_interval has been exceeded.
     * @return true if the internal timer has exceeded the _last_emit time.
     */
    bool time_has_come(){
        // Calculate the time difference between now and the last emit
        const DurationType duration = Clock::now() - _last_emit;
        
        // If more time than the _emit_interval has passed, return true
        return (duration > _emit_interval);
    }

    /// Reset the timer to now
    void reset(){
        _last_emit = Clock::now();
    }
};


/// The MonitorEntries stores the data that is emitted to the terminal.
// TODO consider moving all this functionality directly into MonitorManager?
class MonitorEntries {
private:
    // -- Member declaration -- //
    /// The monitor data that should be emitted
    YAML::Node _data;

public:

    /// Constructor
    MonitorEntries() : _data(YAML::Node()) {
        // Specify that the emitted data is shown in a single line
        _data.SetStyle(YAML::EmitterStyle::Flow);
    };

    /// Set an entry in the monitor data
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
        _data[model_name + "." + key] = value;
    }

    /// Emit the stored data to the terminal.
    void emit(){
        std::cout << _data << std::endl;
    }
};


/// The MonitorManager manages the MonitorEntries and MonitorTimer
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

    /// The monitor data
    MonitorEntries _data;

    /// The flag that determines whether to collect data
    bool _emit_enabled;

public:
    /// Constructor
    /**
     * @param emit_interval The emit interval that specifies after how much
     * time to emit the monitor data.
     */
    MonitorManager (const double emit_interval)
    :
        // Create a new MonitorTimer object
        _timer(std::make_shared<MonitorTimer>(emit_interval)),
        // Create an empty MonitorEntries object for the data to be emitted
        _data(MonitorEntries()),
        // Initialially set the collect data flag to true
        _emit_enabled(true)
    {};

    /// Perform an emission of the data to the terminal, if the flag was set
    void emit_if_enabled () {
        if (_emit_enabled){
            _data.emit();
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
     *         whether it makes sense to collect monitor data; it makes only
     *         sense to collect data, if the emission will actually performed
     *         in the current time step.
     */
    bool emit_enabled() const {
        return _emit_enabled;
    }

    /// Get a shared pointer to the MonitorTimer object.
    Timer& get_timer(){
        return _timer;
    }

    /// Get the reference to the monitor data object.
    MonitorEntries& get_data(){
        return _data;
    }
};


/// The Monitor monitors data that is emitted if a given time has passed.
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
     * @param root_mtr The root monitor manager
     */
    Monitor(const std::string name,
            std::shared_ptr<MonitorManager> root_mtr)
    :
        _name(name),
        _mtr_mgr(root_mtr)
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

    /// Provide a new entry in the MonitorEntries.
    /** This entry is set regardless of whether the emission is enabled!
     * @tparam Function The type of the function that is called
     * @param key The key of the new entry
     * @param value The function to call to retrieve the entry value
     */
    template <typename Function>
    void set_by_func (const std::string key,  const Function f) {
        _mtr_mgr->get_data().set_entry(_name, key, f());
    }

    /// Provide a new entry in the MonitorEntries.
    /** This entry is set regardless of whether the emission is enabled!
     * @tparam Value The type of the value
     * @param key The key of the new entry
     * @param value The value of the new entry
     */
    template <typename Value>
    void set_by_value (const std::string key, const Value value) {
        _mtr_mgr->get_data().set_entry(_name, key, value);
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
