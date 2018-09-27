#ifndef UTOPIA_DATAIO_MONITOR_HH
#define UTOPIA_DATAIO_MONITOR_HH

#include <vector>
#include <chrono>
#include <iostream>

#include <yaml-cpp/yaml.h>

namespace Utopia{
namespace DataIO{

class MonitorTimer{
public:
    typedef std::chrono::high_resolution_clock Clock;

    using Time = std::__1::chrono::high_resolution_clock::time_point;
    using ChronoTimeUnit = std::chrono::milliseconds;

private:
    const std::size_t _emit_interval;

    Time _last_emit;

public:
    MonitorTimer(const std::size_t emit_interval) :
                    _emit_interval(emit_interval),
                    // Initialize the time of the last commit to current time
                    _last_emit(Clock::now()) {}

    bool time_has_come(const bool reset=false){
        // Calculate the time difference between now and the last emit
        auto now = Clock::now();
        auto duration = std::chrono::duration_cast<ChronoTimeUnit>(now - _last_emit);
        
        // If more time than the _emit_interval has passed return true
        if ((std::size_t)duration.count() > _emit_interval) {
            if (reset){
                _last_emit = now;
                std::cout << "reset" << std::endl;
            }
            return true;
        }
        else{
            return false;
        }
    }
};


class MonitorData{
private:
    YAML::Node _data;
public:
    MonitorData() : _data(YAML::Node()) {
        // Specify that the emitted data is shown in a single line
        _data.SetStyle(YAML::EmitterStyle::Flow);
    };

    template<typename Value>
    void set_entry(const std::string model_name, const std::string key, const Value value){
        _data[model_name + "." + key] = value;
    }

    void emit(){
        std::cout << _data << std::endl;
    }
};


class RootMonitor{
private:

    using Timer = std::shared_ptr<MonitorTimer>;

    const std::string _name;

    Timer _timer;

    MonitorData _data;

public:
    RootMonitor(const std::string name, 
                const std::size_t emit_interval) :
                    _name(name),
                    _timer(std::make_shared<MonitorTimer>(emit_interval)),
                    // Create an empty MonitorData object for the data to be emitted
                    _data(MonitorData()) {};

    void perform_emission(){
        if (time_has_come(true)){
            _data.emit();
        }
    }

    bool time_has_come(const bool reset=false){
        return _timer->time_has_come(reset);
    }

    Timer& get_timer(){
        return _timer;
    }

    MonitorData& get_data(){
        return _data;
    }

};

class Monitor{
private:
    const std::string _name;

    std::shared_ptr<RootMonitor> _root_mtr;
public:
    Monitor(const std::string name,
            RootMonitor root_mtr):
                _name(name),
                _root_mtr(std::make_shared<RootMonitor>(root_mtr)){};

    Monitor(const std::string name,
            Monitor& parent_mtr):
                _name(parent_mtr.get_name() + "." + name),
                _root_mtr(parent_mtr.get_root_mtr()){};

    template <typename Value>
    void set_entry(const std::string key, const Value value){
        _root_mtr->get_data().set_entry(_name, key, value);
    }

    template <typename Value>
    void set_entry_if_time_is_ripe(const std::string key, const Value value){
        if (_root_mtr->time_has_come(false)){
            set_entry(key, value);   
        }
    }

    std::shared_ptr<RootMonitor> get_root_mtr() const {
        return _root_mtr;
    }

    std::string get_name() const {
        return _name;
    }
};



} // namespace DataIO
} // namespace Utopia

#endif // UTOPIA_DATAIO_MONITOR_HH