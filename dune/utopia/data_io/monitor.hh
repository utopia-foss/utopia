#ifndef UTOPIA_DATAIO_MONITOR_HH
#define UTOPIA_DATAIO_MONITOR_HH

#include <vector>
#include <chrono>
#include <iostream>

#include <yaml-cpp/yaml.h>

namespace Utopia{
namespace DataIO{

/// The MonitorTimer keeps track of the time when to emit monitor data
class MonitorTimer{
public:
    // -- Data types uses throughout the monitor timer-- //

    /// Data type for the clock
    using Clock = std::chrono::high_resolution_clock;

    /// Data type for a time point
    using Time = std::chrono::high_resolution_clock::time_point;

    /// Data type for the time unit
    using TimeUnit = std::chrono::milliseconds;

private:
    // -- Member declaration -- //
    /// The emit interval
    const std::size_t _emit_interval;

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
    MonitorTimer(   const std::size_t emit_interval) :
                    _emit_interval(emit_interval),
                    // Initialize the time of the last commit to current time
                    _last_emit(Clock::now()) {}

    /// Check for whether the time to emit has come or not.
    /**
     * @param reset Reset the internal timer to the current time
     *        if the _emit_interval has been exceeded.
     * @return true if the internal timer has exceeded the _last_emit time.
     */
    bool time_has_come(const bool reset=false){
        // Calculate the time difference between now and the last emit
        auto now = Clock::now();
        auto duration = std::chrono::duration_cast<TimeUnit>(now - _last_emit);
        
        // If more time than the _emit_interval has passed return true
        if ((std::size_t)duration.count() > _emit_interval) {
            if (reset){
                _last_emit = now;
            }
            return true;
        }
        else{
            return false;
        }
    }
};


/// The MonitorData stores the data that is emitted to the terminal.
class MonitorData{
private:
    // -- Member declaration -- //
    /// The monitor data that should be emitted
    YAML::Node _data;

public:

    /// Constructor
    MonitorData() : _data(YAML::Node()) {
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


/// The MonitorManager manages the MonitorData and MonitorTimer
/** 
 * The manager performs an emission of the stored monitor data
 * if the monitor timer asserts that enough time has passed since
 * the last emit.
 */
class MonitorManager{
private:
    // -- Type definitions -- //
    /// Type of the timer
    using Timer = std::shared_ptr<MonitorTimer>;

    // -- Member declaration -- //
    /// The monitor timer
    Timer _timer;

    /// The monitor data
    MonitorData _data;

public:
    /// Constructor
    /**
     * @param emit_interval The emit interval that specifies after how much
     * time to emit the monitor data.
     */
    MonitorManager( const std::size_t emit_interval) :
                    // Create a new MonitorTimer object
                    _timer(std::make_shared<MonitorTimer>(emit_interval)),
                    // Create an empty MonitorData object for the data to be emitted
                    _data(MonitorData()) {};

    /// Perform an emission of the data to the terminal.
    void perform_emission(){
        if (time_has_come(true)){
            _data.emit();
        }
    }

    /// Check whether the time to emit has come.
    bool time_has_come(const bool reset=false){
        return _timer->time_has_come(reset);
    }

    /// Get a shared pointer to the MonitorTimer object.
    Timer& get_timer(){
        return _timer;
    }

    /// Get the reference to the monitor data object.
    MonitorData& get_data(){
        return _data;
    }
};


/// The Monitor monitors data that is emitted if a given time has passed.
class Monitor{
private:
    // -- Member declaration -- //
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
            MonitorManager root_mtr):
                _name(name),
                _mtr_mgr(std::make_shared<MonitorManager>(root_mtr)){};

    /// Constructor
    /** Construct a new Monitor object. The shared pointer to the MonitorManager
     * points at the same MonitorManager as in the parent monitor object.
     * 
     * @param name The name of the monitor
     * @param parent_mtr The parent monitor
     */
    Monitor(const std::string name,
            Monitor& parent_mtr):
                _name(parent_mtr.get_name() + "." + name),
                _mtr_mgr(parent_mtr.get_mtr_mgr()){};

    /// Set a new entry in the MonitorData.
    /**
     * @tparam Value The type of the value
     * @param key The key of the new entry
     * @param value The value of the new entry
     * @param wait_for_timer If the timer assures that the emit interval is
     * surpassed set the entry, otherwise nothing is written.
     */
    template <typename Value>
    void set_entry( const std::string key, 
                    const Value value, 
                    const bool wait_for_timer = true){
        if (wait_for_timer){
            if (_mtr_mgr->time_has_come(false)){
                _mtr_mgr->get_data().set_entry(_name, key, value);   
            }
        }
        else{
            _mtr_mgr->get_data().set_entry(_name, key, value);
        }
    }

    /// Get a shared pointer to the MonitorManager.
    std::shared_ptr<MonitorManager> get_mtr_mgr() const {
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